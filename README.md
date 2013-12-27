The project implements a distributed indoor temperature control system using two Beaglebone Blacks as two spots. 

Since the trouble to utilize Bluetooth, two BBBs are connected through one laptop with cables. One is responsible for temperature data collection, while the other is in charge of control of lamps and fan according to the first one's data.

1. The first choice is TCP communication using OpenSSL and Mini-XML. Its server and client code can be found in folder tcp_ssl_xml. Please see the README in tcp_ssl_xml for more details.




