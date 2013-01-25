/*
Icinga Nagios Ampel mit Arduino
*/

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "Traffic_Light.h"

byte mac[] = { 0xDE, 0xA1, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(172, 31, 0, 177);

unsigned int localPort = 54000;      // local port to listen on

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged";       // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

Traffic_Light TL_1;
Traffic_Light TL_2;

void setup() {
  // start the Ethernet and UDP:
  Ethernet.begin(mac,ip);
  Udp.begin(localPort);

  Serial.begin(9600);
  TL_1.init(13,22,21);
  TL_2.init(24,25,26);
}


String traffic_light_number;
String command;
String parameter;


void loop() {
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if(packetSize)
  {
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remote = Udp.remoteIP();
    for (int i =0; i < 4; i++)
    {
      Serial.print(remote[i], DEC);
      if (i < 3)
      {
        Serial.print(".");
      }
    }
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    // read the packet into packetBufffer
    Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);
    Serial.println("Contents:");
    
    parameter = packetBuffer;
    parameter.trim();
    
    
    traffic_light_number = parameter.substring(0,1);
    command = parameter.substring(2,parameter.length());
    Serial.println(traffic_light_number);
    Serial.print(command);
    Serial.print(";");
    
    if(traffic_light_number.compareTo("1") == 0) 
    {
      if(command.compareTo("redOn") == 0) 
      {
        TL_1.redOn(); 
      }
      if(command.compareTo("redOff") == 0) 
      {
        TL_1.redOff(); 
      }
      if(command.compareTo("yellowOn") == 0) 
      {
        TL_1.yellowOn(); 
      }
      if(command.compareTo("yellowOff") == 0) 
      {
        TL_1.yellowOff(); 
      }
      if(command.compareTo("greenOn") == 0) 
      {
        TL_1.rOn(); 
      }
      if(command.compareTo("greenOff") == 0) 
      {
        TL_1.redOff(); 
      }
    } 


    /* send a reply, to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
    */
  }
}






