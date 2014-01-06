#include "/home/dhlinux/mxml-2.7pc/mxml.h"
#include "../include/mdp.h"
#include "mdp_worker.c"

// Woker registers to broker with service "Temperature"
mdp_worker_t *worker_to_broker(int argc, char *argv[]) {
    int verbose = (argc > 1 && streq (argv [1], "-v"));
    mdp_worker_t *session = mdp_worker_new (
        "tcp://192.168.9.1:5555", "Temperature", verbose);
    
    return session;
}

double get_temperature() {
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


char *get_temperature_xml() {
    mxml_node_t *xml;
    mxml_node_t *data;
    mxml_node_t *node;
    
    char buffer[1024];

    xml = mxmlNewXML("1.0");
    data = mxmlNewElement(xml, "data");
    node = mxmlNewElement(data, "temperature");
    
    double temperature_data = get_temperature();
    mxmlNewReal(node, temperature_data);

    mxmlSaveString(xml, buffer, sizeof(buffer), MXML_NO_CALLBACK);
    
    return buffer;
}

int main (int argc, char *argv [])
{
    // register to broker
    mdp_worker_t *session = worker_to_broker(argc, argv);

    while (1) {
        // Get request from the broker
        zframe_t *reply_to;
        zmsg_t *request = mdp_worker_recv (session, &reply_to);

        // Prepare data
        zmsg_t *temperature = zmsg_new();
        char *data_to_send = get_temperature_xml();
        zmsg_pushstr(temperature, data_to_send);
        if (request == NULL)
            break;              //  Worker was interrupted
        //  Echo message
        mdp_worker_send (session, &temperature, reply_to);
        sleep(1);
        zframe_destroy (&reply_to);
    }
    mdp_worker_destroy (&session);
    return 0;
}
