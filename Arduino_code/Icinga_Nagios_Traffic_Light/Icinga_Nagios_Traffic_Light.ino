/*
Icinga Nagios Ampel mit Arduino
*/

//http://svg-edit.googlecode.com/svn/branches/2.6/editor/svg-editor.html#delete

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
Traffic_Light TL_3;
Traffic_Light TL_4;

void setup() {
  // start the Ethernet and UDP:
  Ethernet.begin(mac,ip);
  Udp.begin(localPort);

  Serial.begin(9600);
  TL_1.init(31,28,29);
  TL_2.init(25,22,23);
  
  TL_3.init(30,40,24);
  TL_4.init(27,40,26);
  
  TL_1.allOn();
  TL_2.allOn();
  TL_3.allOn();
  TL_4.allOn();
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

   // memcpy(packetBuffer,"A",UDP_TX_PACKET_MAX_SIZE);

    // read the packet into packetBufffer
    Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);
    Serial.println("Contents:");
    
    parameter = packetBuffer;
    parameter.trim();
    
    
    traffic_light_number = parameter.substring(0,1);
    command = String("");
    command = parameter.substring(1,packetSize);
    
    
    Serial.println(traffic_light_number);
    Serial.print(command);
    Serial.print(";");
    
	//Traffic Light 1
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
        TL_1.greenOn(); 
      }
      if(command.compareTo("greenOff") == 0) 
      {
        TL_1.greenOff(); 
      }
    } 

	//Traffic Light 2
    if(traffic_light_number.compareTo("2") == 0) 
    {
      if(command.compareTo("redOn") == 0) 
      {
        TL_2.redOn(); 
      }
      if(command.compareTo("redOff") == 0) 
      {
        TL_2.redOff(); 
      }
      if(command.compareTo("yellowOn") == 0) 
      {
        TL_2.yellowOn(); 
      }
      if(command.compareTo("yellowOff") == 0) 
      {
        TL_2.yellowOff(); 
      }
      if(command.compareTo("greenOn") == 0) 
      {
        TL_2.greenOn(); 
      }
      if(command.compareTo("greenOff") == 0) 
      {
        TL_2.greenOff(); 
      }
    } 

    if(traffic_light_number.compareTo("3") == 0) 
    {
      if(command.compareTo("redOn") == 0) 
      {
        TL_3.redOn(); 
      }
      if(command.compareTo("redOff") == 0) 
      {
        TL_3.redOff(); 
      }
      if(command.compareTo("greenOn") == 0) 
      {
        TL_3.greenOn(); 
      }
      if(command.compareTo("greenOff") == 0) 
      {
        TL_3.greenOff(); 
      }
    } 

    if(traffic_light_number.compareTo("4") == 0) 
    {
      if(command.compareTo("redOn") == 0) 
      {
        TL_4.redOn(); 
      }
      if(command.compareTo("redOff") == 0) 
      {
        TL_4.redOff(); 
      }
      if(command.compareTo("greenOn") == 0) 
      {
        TL_4.greenOn(); 
      }
      if(command.compareTo("greenOff") == 0) 
      {
        TL_4.greenOff(); 
      }
    } 	
	
    /* send a reply, to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
    */
  }
}






