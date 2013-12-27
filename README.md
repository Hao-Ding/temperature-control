The project implements a distributed indoor temperature control system using two Beaglebone Blacks as two spots. 

Since the trouble to utilize Bluetooth, two BBBs are connected through one laptop with cables. One is responsible for temperature data collection, while the other is in charge of control of lamps and fan according to the first one's data.

1. The first choice is TCP communication using OpenSSL and Mini-XML. Its server and client code can be found in folder tcp_ssl_xml.

If the certificate and private key are out of date, please use the following command to generate new ones. 
    openssl genrsa -out privkey.pem 2048
    openssl req -new -x509 -key privkey.pem -out cacert.pem -days 1095
Of course, you should have installed OpenSSL and Mini-XML for PC and ARM.

Move into tcp_ssl_xml, and type 
    gcc -o tcp_ssl_xml_client tcp_ssl_xml_client.c -lssl -lcrypto -lmxml -lpthread
    gcc -o tcp_ssl_xml_server tcp_ssl_xml_server.c -lssl -lcrypto -lmxml -lpthread
to generate binary files for PC, and change gcc to arm-linux-gnueabi-gcc, for instance, to generate binary files for BBBs. Maybe you should also point out the path of Mini-XML library for ARM, when cross-compiling. Here is /home/dhlinux/mxml-2.7ARM. 
So it should be like arm-linux-gnueabi-gcc -o tcp_ssl_xml_server tcp_ssl_xml_server.c -L/home/dhlinux/mxml-2.7ARM -lssl -lcrypto -lmxml -lpthread

Type ./tcp_ssl_xml_server 8080 5 cacert.pem privkey.pem
     ./tcp_ssl_xml_client 127.0.0.1 8080 25 30
to run them on PC, and change 127.0.0.1 to the ip address of server. Here, 25 and 30 are the Low and High limits of temperature correspondingly.


