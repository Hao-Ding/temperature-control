#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "/home/dhlinux/mxml-2.7pc/mxml.h"

#define MAXBUF 1024

SSL_CTX *my_SSL_init(int argc, char **argv) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method());

    if (ctx == NULL) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    if (SSL_CTX_use_certificate_file(ctx, argv[3], SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, argv[4], SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    return ctx;
}

int my_tcp_init(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in my_addr;
    unsigned int myport, lisnum;

    if (argv[1])
        myport = atoi(argv[1]);
    else
        myport = 7838;

    if (argv[2])
        lisnum = atoi(argv[2]);
    else
        lisnum = 2;

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    } else
        printf("socket created\n");

    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = PF_INET;
    my_addr.sin_port = htons(myport);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr))
== -1) {
        perror("bind");
        exit(1);
    } else
        printf("binded\n");

    if (listen(sockfd, lisnum) == -1) {
        perror("listen");
        exit(1);
    } else
        printf("begin listen\n");
    
    return sockfd;
}

int get_SSL_fd(int sockfd, int *new_fd) {
    struct sockaddr_in their_addr;
    socklen_t len;
    len = sizeof(struct sockaddr);

    if ((*new_fd =
        accept(sockfd, (struct sockaddr *) &their_addr,
        &len)) == -1) {
        perror("accept");
        exit(errno);
    } else
        printf("server: get connection from %s, port %d, socket %d\n",
            inet_ntoa(their_addr.sin_addr),
            ntohs(their_addr.sin_port), *new_fd);

    return 0;
}

int send_temperature_data_xml(int fd, SSL *ssl) {
    lseek(fd, 0, 0);
    char buffer[1024];
    int ret = read(fd, buffer, sizeof(buffer));
    if (ret != -1) {
        buffer[ret] = '\0';
    }

    int value = atoi(buffer);
    double temp = (value / 4096.0) * 180 - 50;
    char buffer2[1024];
    //gcvt(temp, 4, buffer2);
    mxml_node_t *xml;
    mxml_node_t *data;
    mxml_node_t *node;
    xml = mxmlNewXML("1.0");
    data = mxmlNewElement(xml, "data");
    node = mxmlNewElement(data, "temperature");
    mxmlNewReal(node, temp);
    mxmlSaveString(xml, buffer2, sizeof(buffer2), MXML_NO_CALLBACK);
    int len;
    len = SSL_write(ssl, buffer2, strlen(buffer2));

    if (len <= 0) {
        printf
            ("Fail to send message '%s'! Error code is %d, Error message is'%s'\n",
            buffer2, errno, strerror(errno));
    } else
        printf("Succeed in sending message '%s', Length = %d!\n",
            buffer2, len);

    return 0;
}

int main(int argc, char **argv) {
    int sockfd, new_fd;
    SSL_CTX *ctx = my_SSL_init(argc, argv);

    sockfd = my_tcp_init(argc, argv);
    while (1) {
        SSL *ssl;
        
        get_SSL_fd(sockfd, &new_fd);
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, new_fd);

        if (SSL_accept(ssl) == -1) {
            perror("accept");
            close(new_fd);
            break;
        }
        
        //int fd = open("/sys/bus/iio/devices/iio\:device0/in_voltage0_raw", O_RDONLY);
        int fd = open("data", O_RDONLY);
        while(1) {
            send_temperature_data_xml(fd, ssl);
            sleep(1);
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(new_fd);
        close(fd);
    }
    close(sockfd);
    SSL_CTX_free(ctx);
    return 0;
}
