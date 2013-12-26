#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "/home/dhlinux/mxml-2.7pc/mxml.h"

#define MAXBUF 1024

void show_crts(SSL * ssl)
{
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL) {
        printf("Digit Crt Information:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Crt: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    } else {
        printf("No Crt Information!\n");
    }
}

SSL_CTX *my_SSL_init(int argc, char **argv) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());

    if (ctx == NULL) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    
    return ctx;
}

int my_tcp_init(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in dest;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket");
        exit(errno);
    }
    printf("socket created\n");

    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(atoi(argv[2]));
    if (inet_aton(argv[1], (struct in_addr *) &dest.sin_addr.s_addr) == 0) {
        perror(argv[1]);
        exit(errno);
    }
    printf("address created\n");

    if (connect(sockfd, (struct sockaddr *) &dest, sizeof(dest)) != 0) {
        perror("Connect ");
        exit(errno);
    }
    printf("server connected\n");
    
    return sockfd;
}

double read_temperature_data_xml(SSL *ssl, int *flag_end) {
    int len;
    char buffer[MAXBUF + 1];
    bzero(buffer, MAXBUF + 1);
    len = SSL_read(ssl, buffer, MAXBUF);
    if (len > 0) {
        printf("Receive:'%s'，Length = %d\n",
            buffer, len);
    }
    else {
        printf("Fail to receive data. Error code = %d, Error message is '%s'\n",
            errno, strerror(errno));
        *flag_end = 1;
    }
    mxml_node_t *xml = mxmlLoadString(NULL, buffer, MXML_REAL_CALLBACK);
    double temp = mxmlGetReal(mxmlFindElement(xml, xml, "temperature",  NULL, NULL, MXML_DESCEND));

    return temp;
}

void open_lamp() {
}

void open_fan() {
}

void close_lamp() {
}

void close_fan() {
}

int main(int argc, char **argv)
{
    int sockfd;
    SSL *ssl;

    if (argc != 5) {
        printf("Need more arguments, the correct format is: \n\t\t%s IPaddress Port LowTemperature HighTemperature\n\tFor example:\t%s 192.168.9.2 7838\n",
        argv[0], argv[0]);
        exit(0);
    }

    SSL_CTX *ctx = my_SSL_init(argc, argv);
    
    sockfd = my_tcp_init(argc, argv);

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    if (SSL_connect(ssl) == -1) {
        ERR_print_errors_fp(stderr);
    }
    else {
        printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
        //show_crts(ssl);
    }

    int flag_end = 0;
    while (flag_end == 0) {
        double data_from_server = read_temperature_data_xml(ssl, &flag_end);
        if (flag_end == 0) {
            printf("temperature_data_from_server = %.2lf\n", data_from_server);
            if (data_from_server < atof(argv[3])) {
                open_lamp();
            }
            else if (data_from_server > atof(argv[4])) {
                open_fan();
            }
            else {
                close_lamp();
                close_fan();
            }
        }
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);
    return 0;
}
