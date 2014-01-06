#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "/home/dhlinux/mxml-2.7pc/mxml.h"

#define MAXBUF 1024

void show_crts(SSL * ssl) {
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

double read_temperature_data_xml_from_server(SSL *ssl, int *flag_end) {
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

double read_temperature_data_local() {
    int len;
    int fd = open("/sys/bus/iio/devices/iio\:device0/in_voltage0_raw", O_RDONLY);
    lseek(fd, 0, 0);
    char buffer[1024];
    int ret = read(fd, buffer, sizeof(buffer));
    if (ret != -1) {
        buffer[ret] = '\0';
    }
    int value = atoi(buffer);
    double temp = (value / 4096.0) * 180 - 50;
    return temp;
}

int open_GPIO(int GPIOPin) {
    FILE *myOutputHandle = NULL;
    char setValue[4], GPIOString[4], GPIOValue[64], GPIODirection[64];
    sprintf(GPIOString, "%d", GPIOPin);
    sprintf(GPIOValue, "/sys/class/gpio/gpio%d/value", GPIOPin);
    sprintf(GPIODirection, "/sys/class/gpio/gpio%d/direction", GPIOPin);

    // Export the pin
    if ((myOutputHandle = fopen("/sys/class/gpio/export", "ab")) ==
NULL){
      	printf("Unable to export GPIO pin\n");
        return 1;
    }
    strcpy(setValue, GPIOString);
    fwrite(&setValue, sizeof(char), 2, myOutputHandle);
    fclose(myOutputHandle);

    // Set direction of the pin to an output
    if ((myOutputHandle = fopen(GPIODirection, "rb+")) == NULL){
        printf("Unable to open direction handle\n");
        return 1;
    }
    strcpy(setValue,"out");
    fwrite(&setValue, sizeof(char), 3, myOutputHandle);
    fclose(myOutputHandle);

    // Set output to high
    if ((myOutputHandle = fopen(GPIOValue, "rb+")) == NULL){
        printf("Unable to open value handle\n");
        return 1;
    }
    strcpy(setValue, "1"); // Set value high
    fwrite(&setValue, sizeof(char), 1, myOutputHandle);
    fclose(myOutputHandle);
    return 0;
}

int open_lamp() {
    int GPIOPin=44; /* GPIO1_28 or pin 12 on the P9 header */

    printf("\nStarting Opening Lamp\n");
    open_GPIO(GPIOPin);
    return 0;
}

int open_fan() {
    int GPIOPin=61; /* GPIO1_28 or pin 12 on the P9 header */ 

    printf("\nStarting Opening Fan\n");
    open_GPIO(GPIOPin);
    return 0;
}

int close_GPIO(int GPIOPin) {
    FILE *myOutputHandle = NULL;
    char setValue[4], GPIOString[4], GPIOValue[64], GPIODirection[64];
    sprintf(GPIOString, "%d", GPIOPin);
    sprintf(GPIOValue, "/sys/class/gpio/gpio%d/value", GPIOPin);
    sprintf(GPIODirection, "/sys/class/gpio/gpio%d/direction", GPIOPin);

    // Export the pin
    if ((myOutputHandle = fopen("/sys/class/gpio/export", "ab")) ==
NULL){
      	printf("Unable to export GPIO pin\n");
        return 1;
    }
    strcpy(setValue, GPIOString);
    fwrite(&setValue, sizeof(char), 2, myOutputHandle);
    fclose(myOutputHandle);

    // Set direction of the pin to an output
    if ((myOutputHandle = fopen(GPIODirection, "rb+")) == NULL){
        printf("Unable to open direction handle\n");
        return 1;
    }
    strcpy(setValue,"out");
    fwrite(&setValue, sizeof(char), 3, myOutputHandle);
    fclose(myOutputHandle);

    // Set output to low
    if ((myOutputHandle = fopen(GPIOValue, "rb+")) == NULL){
        printf("Unable to open value handle\n");
        return 1;
    }
    strcpy(setValue, "0"); // Set value high
    fwrite(&setValue, sizeof(char), 1, myOutputHandle);
    fclose(myOutputHandle);

    // Unexport the pin
    if ((myOutputHandle = fopen("/sys/class/gpio/unexport", "ab")) ==
NULL) {
       	printf("Unable to unexport GPIO pin\n");
        return 1;
    }
    strcpy(setValue, GPIOString);
    fwrite(&setValue, sizeof(char), 2, myOutputHandle);
    fclose(myOutputHandle);

    return 0;
}

int close_lamp() {
    int GPIOPin=45; /* GPIO1_28 or pin 12 on the P9 header */ 

    printf("\nStarting Closing Lamp\n");
    close_GPIO(GPIOPin);
    return 0;
}

int close_fan() {
    int GPIOPin=61; /* GPIO1_28 or pin 12 on the P9 header */ 

    printf("\nStarting Closing Fan\n");
    close_GPIO(GPIOPin);
    return 0;
}

int main(int argc, char **argv) {
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

    int flag_end = 0, flag_lamp = 0, flag_fan = 0;
    double min_limit = atof(argv[3]);
    double max_limit = atof(argv[4]);
    printf("min_limit = %0.2lf, max_limit = %.2lf\n", min_limit, max_limit);
    close_lamp();
    close_fan();
    while (flag_end == 0) {
        double data_from_server = read_temperature_data_xml_from_server(ssl, &flag_end);
        double data_local = read_temperature_data_local();
        if (flag_end == 0) {
            printf("temperature_data_from_server = %.3lf\n", data_from_server);
            printf("temperature_data_local = %.2lf\n", data_local);
            double data_ave = (data_from_server + data_local) / 2;
            printf("temperature_average = %.2lf\n", data_ave);
            if (data_ave < min_limit && flag_lamp == 0) {
                open_lamp();
                flag_lamp = 1;
            }
            else if (data_ave > max_limit && flag_fan == 0) {
                open_fan();
                flag_fan = 1;
            }
            else if (data_ave >= min_limit && data_ave <= max_limit){
                if (flag_lamp == 1) {
                    close_lamp();
                    flag_lamp = 0;
                }
                if (flag_fan == 1) {
                    close_fan();
                    flag_fan = 0;
                }
            }
        }
    }
    close_lamp();
    close_fan();

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);
    return 0;
}
