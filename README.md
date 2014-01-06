The project implements a distributed indoor temperature control system using two Beaglebone Blacks as two spots. 

Since the trouble to utilize Bluetooth, two BBBs are connected through one laptop with cables. One is responsible for temperature data collection, while the other is in charge of control of lamps and fan and temperature data collection as well. The decisions will be given according to average temperature degree of the data on both sides.

-------------------------------------------------------------------------

TMP36 is the temperature sensor type chosen in the project. And one example can be found from http://changetheworldwithyourpassion.blogspot.dk/2013/06/make-tmp36-work-on-beaglebone-black.html 

Besides, the connection to lamps and fan is based on GPIO, which can be found on the official Beaglebone black datasheet. 

-------------------------------------------------------------------------

1. The first choice is TCP communication using OpenSSL and Mini-XML. Its server and client code can be found in folder tcp_ssl_xml. Please see the README in tcp_ssl_xml for more details.

2. The second one chooses service-oriented communication, where zeromq is seen as the ideal candidate library. In zeromq, there are various kinds of protocols for network communication implementation, and majordomo, our choice, targets service-oriented style. More implementation details can be found in folder tcp_zmq_majordomo. And more information about zeromq can be found here http://zeromq.org/

