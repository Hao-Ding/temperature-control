#include "../include/mdp.h"
#include "mdp_client.c"
#include "/home/dhlinux/mxml-2.7pc/mxml.h"

mdp_client_t *client_to_broker(int argc, char *argv[]) {
    int verbose = (argc > 1 && streq (argv [1], "-v"));
    mdp_client_t *session = mdp_client_new ("tcp://192.168.7.1:5555", verbose);

    return session;
}

double read_temperature_data_xml_local() {
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
    int GPIOPin=44; /* GPIO1_28 or pin 12 on the P9 header */ 

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

int main (int argc, char *argv[])
{
    int flag_lamp = 0, flag_fan = 0;
    mdp_client_t *session = client_to_broker(argc, argv);
    double min_limit = atof(argv[1]);
    double max_limit = atof(argv[2]);
    printf("min=%lf, max=%lf\n", min_limit, max_limit);

    while(1) {
        zmsg_t *request = zmsg_new();
        zmsg_pushstr (request, "GiveMeTemperatureData");
        mdp_client_send (session, "Temperature", &request);
        zmsg_t *reply = mdp_client_recv (session, NULL, NULL);
        if (reply) {
            char *buffer = zmsg_popstr(reply);
            printf("receive %s\n", buffer);
            mxml_node_t *xml = mxmlLoadString(NULL, buffer, MXML_REAL_CALLBACK);
            double data_from_server = mxmlGetReal(mxmlFindElement(xml, xml, "temperature",  NULL, NULL, MXML_DESCEND));
            double data_local = read_temperature_data_xml_local();
            printf("temperature_data_from_server = %.2lf\n", data_from_server);
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
            zmsg_destroy (&reply);
        }
        else {
            break;              //  Interrupted by Ctrl-C
        }
    }
    close_lamp();
    close_fan();

    mdp_client_destroy (&session);
    return 0;
}
