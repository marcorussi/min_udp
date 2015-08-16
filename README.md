# minUDP
A minimal UDP/IP stack implemented on top of an Ethernet layer for PIC32MX.
This UDP stack has been added to the existing PIC32 framework.

This demo project runs over PIC32 Ethernet Starter Kit.
The UDP application implemented in app_udp.c file controls state of 2 LEDs with
2 related UDP sockets. For an easy test just run from ubuntu console following
commands considering x.x.x.x as the obtained IP address:

    $ sudo sendip -p ipv4 -is 10.42.0.1 -p udp -us 1010 -ud 2020 -v x.x.x.x -d "led on"

for turning LED 1 ON;

    $ sudo sendip -p ipv4 -is 10.42.0.1 -p udp -us 1010 -ud 2020 -v x.x.x.x -d "led off"

for turning LED 1 OFF;

    $ sudo sendip -p ipv4 -is 10.42.0.1 -p udp -us 1010 -ud 2020 -v x.x.x.x -d "led blink"

for starting LED 1 blink;

Do the same with -us 3030 -ud 4040 for controlling LED 2.
Each LED state request is followed by a ACK packet. You can use Wireshark for
checking the obtained IP address and requests/ACK packets.
The demo application is completely configurable and it needs my PIC32 framework
which has been included in this project. Anyway you can easily port the demo
application or the entire UDP stack on another RTOS or it can run in a simple
while loop.

Known issues:
 - Ethernet layer remains stacked until an Ethernet cable is connected.
   It is necessary to add a timeout in order to exit the related infinite loop
   in eth.c file.
