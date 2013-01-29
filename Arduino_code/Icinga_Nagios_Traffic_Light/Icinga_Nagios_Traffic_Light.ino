/*
Icinga Nagios Ampel mit Arduino
GitHub: https://github.com/edvler/Icinga_Nagios_Traffic_Light
Web-Blog: http://www.edvler-blog.de/icinga_nagios_traffic_light_ampel

Author: Matthias Maderer
Date: 29.01.2013

Info: Look at the Readme.txt or the Web-Blog

Lizenz:
http://creativecommons.org
"Creative Commons Namensnennung - Nicht-kommerziell - Weitergabe unter gleichen Bedingungen 3.0 Unported Lizenz"

Possible commands to send per Ethernet UDP:
1redOn - Turn on the red sign of traffic light ONE
2yellowOn - Turn on the yellow sign of traffic light TWO
3greenOn - Turn on the green sign of traffic light THREE
...

1redOff - Turn off the red sign of traffic light ONE
2yellowOff - Turn off the yellow sign of traffic light TWO
...

allOn - Turn on all signs
allOff - Turn off all signs

commands are case-sensitive!

*/

//include needed libraries
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "Traffic_Light.h"

byte mac[] = { 0xDE, 0xA1, 0xBE, 0xEF, 0xFE, 0xED };   // choose MAC address (free which one)
IPAddress ip(172, 31, 0, 177);                         // choose IP address
IPAddress gateway (172,31,0,254);                      // choose IP gateway
IPAddress subnet  (255, 255, 255, 0);                  // choose subnet
unsigned int localPort = 54000;                        // local port to listen 

char packetBuffer[UDP_TX_PACKET_MAX_SIZE];             // buffer to hold incoming packet


EthernetUDP Udp;                                       // An EthernetUDP instance to let us send and receive packets over UDP

//Instances of the Traffic_Light class for each Traffic Light
Traffic_Light TL_1;
Traffic_Light TL_2;
Traffic_Light TL_3;
Traffic_Light TL_4;

void setup() {
  // start the Serial-Port
  Serial.begin(9600);
  
  // start the Ethernet and UDP:
  Ethernet.begin(mac, ip, gateway, subnet);
  Udp.begin(localPort);

  // set the PIN's on the Arduino Board for each Traffic Light
  // .init(RED,YELLOW,GREEN)
  TL_1.init(31,28,29);
  TL_2.init(25,22,23);
  //TL_3 and TL_4 only has a red and green sign
  TL_3.init(30,40,24);
  TL_4.init(27,40,26);
  
  // test all signs on startup
  TL_1.allOn();
  delay(500);
  TL_1.allOff();
  TL_2.allOn();
  delay(500);
  TL_2.allOff();
  TL_3.allOn();
  delay(500);
  TL_3.allOff();
  TL_4.allOn();
  delay(500);
  TL_4.allOff();
}


//string objects for the loop() function
String traffic_light_number;
String command;
String parameter;


void loop() {
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if(packetSize)
  {
    // some DEBUG output to serial console
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
    
    // make a string of the char array
    parameter = packetBuffer;
    parameter.trim();
    
    // determine Traffic Light number and command
    traffic_light_number = parameter.substring(0,1);
    command = String("");
    command = parameter.substring(1,packetSize);
    
    // some DEBUG output to serial console
    Serial.println("Contents:");
    Serial.println(traffic_light_number);
    Serial.print(command);
    Serial.println(";");
    
    // if the command is for all Traffic Lights
    if(traffic_light_number.compareTo("a") == 0) {
      if(command.compareTo("llOn") == 0) 
      {
        TL_1.allOn(); 
        TL_2.allOn(); 
        TL_3.allOn(); 
        TL_4.allOn(); 
      }
      if(command.compareTo("llOff") == 0) 
      {
        TL_1.allOff(); 
        TL_2.allOff(); 
        TL_3.allOff(); 
        TL_4.allOff();
      }             
    }

    
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

    //Traffic Light 3 - only red and green sign
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

    //Traffic Light 4 - only red and green sign
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
  }
}






