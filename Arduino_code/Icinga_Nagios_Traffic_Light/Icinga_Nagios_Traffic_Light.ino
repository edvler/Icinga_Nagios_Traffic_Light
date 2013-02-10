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


#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>

#include <EEPROM.h>
#include "EEPROMAnything.h"



/* #############################################################################################################################################################
* Settings section.
* Some values may be changed
*/
//#define DEBUG					//Display debug output to the serial console
// #############################################################################################################################################################




/* #############################################################################################################################################################
* Code for the EEPROM related things
* 
*/
#define RESET_PIN 40	//Connect a button to this PIN. If the button is hold, an the device is turned on the default ethernet settings are restored.

/*structure which is stored in the eeprom. 
* Look at "EEPROMAnything.h" for the functions storing and reading the struct
*/
struct config_t
{
    byte use_dhcp;
    byte mac[6];
    byte ip[4];
    byte gateway[4];
    byte subnet[4];
    byte dns_server[4];
    unsigned int localPort;
    unsigned int webserverPort;
    byte config_set;
} eeprom_config;

/*The default Ethernet settings. 
* This settings are used when no config is present or the reset button is used.
*/
void set_EEPROM_Default() {
    eeprom_config.config_set=255;
  
    eeprom_config.use_dhcp = 0;
  
    eeprom_config.mac[0]=0xA1;
    eeprom_config.mac[1]=0xA2;
    eeprom_config.mac[2]=0xA3;
    eeprom_config.mac[3]=0xA4;
    eeprom_config.mac[4]=0xA5;
    eeprom_config.mac[5]=0xA0;
    
    eeprom_config.ip[0]=172;
    eeprom_config.ip[1]=31;
    eeprom_config.ip[2]=0;
    eeprom_config.ip[3]=123;
  
    eeprom_config.gateway[0]=172;
    eeprom_config.gateway[1]=31;
    eeprom_config.gateway[2]=0;
    eeprom_config.gateway[3]=254;
    
    eeprom_config.subnet[0]=255;
    eeprom_config.subnet[1]=255;
    eeprom_config.subnet[2]=255;
    eeprom_config.subnet[3]=0;

    eeprom_config.dns_server[0]=172;
    eeprom_config.dns_server[1]=31;
    eeprom_config.dns_server[2]=0;
    eeprom_config.dns_server[3]=254;

    eeprom_config.localPort=54000;
    eeprom_config.webserverPort=80;
    
    EEPROM_writeAnything(0, eeprom_config);
    
    #ifdef DEBUG
      Serial.println("Config reset");
    #endif 
}


/**
* read_EEPROM_Settings function
* This function is used to read the EEPROM settings at startup
*
* Overview:
* - Set the PIN for the RESET-button to input and activate pullups
* - Load the stored data from EEPROM into the eeprom_config struct
* - Check if a config is stored or the reset button is pressed. If one of the conditions is ture, set the defaults
*/
void read_EEPROM_Settings() {
  pinMode(RESET_PIN, INPUT);
  digitalWrite(RESET_PIN, HIGH);
  
  EEPROM_readAnything(0, eeprom_config);
  
  if (eeprom_config.config_set != 255 || digitalRead(RESET_PIN) == LOW) {
    set_EEPROM_Default();
  } 
}
// #############################################################################################################################################################



/* #############################################################################################################################################################
* System functions
* Runtime and RAM informations
*/
#include <System.h> //Quellcode: Look at [Sketchbook Speicherort]\libraries\System ([Sketchbook Speicherort] is configured in the Arduino-IDE: Files -> Settings)
System Sys;
// #############################################################################################################################################################



/* START Webserver section #####################################################################################################################################
* Webserver Code
*/
#define NAMELEN 5                               //Possible parameter name length (webserver)
#define VALUELEN 11                             //Possible value length (webserver)


/*
* Store large strings in FLASH
*/
P(Page_start) = "<html><head><title>Icinga/Nagios traffic light</title></head><body>\n";
P(Page_end) = "</body></html>";
P(Get_head) = "<h1>GET from ";
P(Post_head) = "<h1>POST to ";
P(Unknown_head) = "<h1>UNKNOWN request for ";
P(Default_head) = "unidentified URL requested.</h1><br>\n";
P(Parsed_head) = "parsed.html requested.</h1><br>\n";
P(Good_tail_begin) = "URL tail = '";
P(Bad_tail_begin) = "INCOMPLETE URL tail = '";
P(Tail_end) = "'<br>\n";
P(Parsed_tail_begin) = "URL parameters:<br>\n";
P(Parsed_item_separator) = " = '";
P(Params_end) = "End of parameters<br>\n";
P(Post_params_begin) = "Parameters sent by POST:<br>\n";
P(Line_break) = "<br>\n";

P(DHCP) = "<b>USE DHCP: </b>";
P(MAC) = "<b>MAC address: </b>";
P(IP) = "<b>IP address: </b>";
P(GW) = "<b>Gateway address: </b>";
P(DNS) = "<b>DNS Server address: </b>";
P(NM) = "<b>Netmask: </b>";
P(WSP) = "<b>Webserver-port: ";
P(LP) = "<b>UDP port to listen on: </b>";

P(Form_start) = "<FORM action=\"setupEth.html\" method=\"post\">";
P(Form_end) = "</FORM>";

P(Form_input_text_start) = "<input type=\"text\" name=\"";
P(Form_input_cb_start) = "<input type=\"checkbox\" name=\"";
P(Form_input_value)  = "\" value=\"";
P(Form_input_end) = "\">";
P(Form_input_send) = "<INPUT type=\"submit\" value=\"Send\">";

P(br) = "<br>";


/**
* setupEth function (webpage)
* This function is used to display a HTML-site for changing the ethernet parameters.
*
* Overview:
* - First of all send a success header to the client.
* - If a POST request is done parse the parameters and store it in the EEPROM
* - Create a simple html form with the actual parameters from eeprom
*/
void setupEth(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  URLPARAM_RESULT rc;
  char name[NAMELEN];
  char value[VALUELEN];

  // this line sends the standard "we're all OK" headers back to the browser
  server.httpSuccess();

  // if we're handling a GET or POST, we can output our data here. For a HEAD request, we just stop after outputting headers.
  if (type == WebServer::HEAD) {
    return;
  }

  // print html body
  server.printP(Page_start);
  
  // debug output! Print some infos about the request
  #ifdef DEBUG
    switch (type)
      {
      case WebServer::GET:
          server.printP(Get_head);
          break;
      case WebServer::POST:
          server.printP(Post_head);
          break;
      default:
          server.printP(Unknown_head);
      }


    server.printP(Parsed_head);
    server.printP(tail_complete ? Good_tail_begin : Bad_tail_begin);
    server.print(url_tail);
    server.printP(Tail_end);
  #endif

  // if a POST request is sent, read all parameters
  if (type == WebServer::POST)
  {
    #ifdef DEBUG
      server.printP(Post_params_begin);
    #endif
    
    while (server.readPOSTparam(name, NAMELEN, value, VALUELEN))
    {
      #ifdef DEBUG
        server.print(name);
        server.printP(Parsed_item_separator);
        server.print(value);
        server.printP(Tail_end);
      #endif
 

      if(strcmp(name,"ma0")==0) {
         eeprom_config.mac[0]=strtol(value,NULL,16);
      }
      if(strcmp(name,"ma1")==0) {
         eeprom_config.mac[1]=strtol(value,NULL,16);
      }
      if(strcmp(name,"ma2")==0) {
         eeprom_config.mac[2]=strtol(value,NULL,16);
      }
      if(strcmp(name,"ma3")==0) {
         eeprom_config.mac[3]=strtol(value,NULL,16);
      }
      if(strcmp(name,"ma4")==0) {
         eeprom_config.mac[4]=strtol(value,NULL,16);
      }
      if(strcmp(name,"ma5")==0) {
         eeprom_config.mac[5]=strtol(value,NULL,16);
      }
      
      
      if(strcmp(name,"ip0")==0) {
         eeprom_config.ip[0]=atoi(value);
      }
      if(strcmp(name,"ip1")==0) {
         eeprom_config.ip[1]=atoi(value);
      }
      if(strcmp(name,"ip2")==0) {
         eeprom_config.ip[2]=atoi(value);
      }           
      if(strcmp(name,"ip3")==0) {
         eeprom_config.ip[3]=atoi(value);
      }


      if(strcmp(name,"gw0")==0) {
         eeprom_config.gateway[0]=atoi(value);
      }
      if(strcmp(name,"gw1")==0) {
         eeprom_config.gateway[1]=atoi(value);
      }
      if(strcmp(name,"gw2")==0) {
         eeprom_config.gateway[2]=atoi(value);
      }           
      if(strcmp(name,"gw3")==0) {
         eeprom_config.gateway[3]=atoi(value);
      }


      if(strcmp(name,"nm0")==0) {
         eeprom_config.subnet[0]=atoi(value);
      }
      if(strcmp(name,"nm1")==0) {
         eeprom_config.subnet[1]=atoi(value);
      }
      if(strcmp(name,"nm2")==0) {
         eeprom_config.subnet[2]=atoi(value);
      }           
      if(strcmp(name,"nm3")==0) {
         eeprom_config.subnet[3]=atoi(value);
      }

      if(strcmp(name,"dn0")==0) {
         eeprom_config.dns_server[0]=atoi(value);
      }
      if(strcmp(name,"dn1")==0) {
         eeprom_config.dns_server[1]=atoi(value);
      }
      if(strcmp(name,"dn2")==0) {
         eeprom_config.dns_server[2]=atoi(value);
      }           
      if(strcmp(name,"dn3")==0) {
         eeprom_config.dns_server[3]=atoi(value);
      }


      if(strcmp(name,"lp")==0) {
         eeprom_config.localPort=atoi(value);
      }

      if(strcmp(name,"wp")==0) {
         eeprom_config.webserverPort=atoi(value);
      }

      if(strcmp(name,"dh")==0) {
         eeprom_config.use_dhcp=255;
      }
    
    }
    
    EEPROM_writeAnything(0, eeprom_config);
  }

  server.printP(Form_start);

  server.printP(MAC);
  for (int a=0;a<6;a++) {
    server.printP(Form_input_text_start);
    server.print("ma");
    server.print(a);
    server.printP(Form_input_value);
    server.print(eeprom_config.mac[a],HEX);
    server.printP(Form_input_end);
  }
  server.printP(br);
 
  server.printP(IP);
  for (int a=0;a<4;a++) {
    server.printP(Form_input_text_start);
    server.print("ip");
    server.print(a);
    server.printP(Form_input_value);
    server.print(eeprom_config.ip[a]);
    server.printP(Form_input_end);
  }
  server.printP(br);

  server.printP(GW);
  for (int a=0;a<4;a++) {
    server.printP(Form_input_text_start);
    server.print("gw");
    server.print(a);
    server.printP(Form_input_value);
    server.print(eeprom_config.gateway[a]);
    server.printP(Form_input_end);
  }
  server.printP(br);
  
  server.printP(NM);
  for (int a=0;a<4;a++) {
    server.printP(Form_input_text_start);
    server.print("nm");
    server.print(a);
    server.printP(Form_input_value);
    server.print(eeprom_config.subnet[a]);
    server.printP(Form_input_end);
  }
  server.printP(br);
  
  server.printP(DNS);
  for (int a=0;a<4;a++) {
    server.printP(Form_input_text_start);
    server.print("dn");
    server.print(a);
    server.printP(Form_input_value);
    server.print(eeprom_config.dns_server[a]);
    server.printP(Form_input_end);
  }
  server.printP(br);
  
  server.printP(WSP);
  server.printP(Form_input_text_start);
  server.print("wp");
  server.printP(Form_input_value);
  server.print(eeprom_config.webserverPort);
  server.printP(Form_input_end);
  server.printP(br);

  server.printP(LP);
  server.printP(Form_input_text_start);
  server.print("lp");
  server.printP(Form_input_value);
  server.print(eeprom_config.localPort);
  server.printP(Form_input_end);
  server.printP(br);

  server.printP(DHCP);
  server.printP(Form_input_cb_start);
  server.print("dh");
  server.printP(Form_input_value);
  server.print("1");
  if (eeprom_config.use_dhcp == 255) {
    server.printP("\" \"checked");
  }
  server.printP(Form_input_end);
  server.printP(br);

  server.printP(Form_input_send);
  server.printP(br);

  server.printP(Form_end);
  server.printP(br);
  
  server.printP(Page_end);

}


/**
* my_failCmd function (webpage)
* This function is used to display a HTML-site for failed requests
*
* Overview:
* - send a httpFail header to the client.
*/
void my_failCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  //send fail header and display EPIC FAIL
  server.httpFail();
}


/**
* indexHtml function (webpage)
* This function is used to display the index.html (default page)
*
* Overview:
* - First of all send a success header to the client.
* - Display a link to the ethernet setup
*/
void indexHtml(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  // this line sends the standard "we're all OK" headers back to the browser
   server.httpSuccess();

  // if we're handling a GET or POST, we can output our data here. For a HEAD request, we just stop after outputting headers.
  if (type == WebServer::HEAD) {
    return;
  }
    
  server.print("<h1>Icinga/Nagios Traffic Light</h1><br>");
  server.print("<a href=\"setupEth.html\">Ethernet Setup</a>");
  server.printP(Page_end);
}
// END Webserver section #######################################################################################################################################



/* START Network section #######################################################################################################################################
* Webserver Code
*/
#define DHCP_RENEW_INTERVAL 60 //DHCP renew time in Minutes

char packetBuffer[UDP_TX_PACKET_MAX_SIZE];             // buffer to hold incoming udp packets
EthernetUDP Udp;                                       // An EthernetUDP instance to let us send and receive packets over UDP

/* This creates an instance of the webserver->  By specifying a prefix
* of "", all pages will be at the root of the server. 
*/
#define PREFIX ""
WebServer  * webserver;


/**
* setupNetwork function (webpage)
* This function is used to setupup the network according to the values stored in the eeprom
*
* Overview:
* - First of all read the EEPROM settings
* - Display a link to the ethernet setup
* - Check if DHCP should be used, if not create instaces of IPAddress for ip, gateway, subnet and dns_server
* - Invoke Ethernet.begin with all parameters if no dhcp is active (Ethernet.begin(mac, ip, dns_server, gateway, subnet);). 
* - If DHCP is used invoke only with mac (Ethernet.begin(mac);) and display the ip on the serial console.
*/
void setupNetwork() {
  read_EEPROM_Settings();
  

  byte mac[] = { eeprom_config.mac[0], eeprom_config.mac[1], eeprom_config.mac[2], eeprom_config.mac[3], eeprom_config.mac[4], eeprom_config.mac[5] };  
  
  if (eeprom_config.use_dhcp != 255) {
    IPAddress ip(eeprom_config.ip[0], eeprom_config.ip[1], eeprom_config.ip[2], eeprom_config.ip[3]);                                               
    IPAddress gateway (eeprom_config.gateway[0],eeprom_config.gateway[1],eeprom_config.gateway[2],eeprom_config.gateway[3]);                      
    IPAddress subnet  (eeprom_config.subnet[0], eeprom_config.subnet[1], eeprom_config.subnet[2], eeprom_config.subnet[3]);  
    IPAddress dns_server  (eeprom_config.dns_server[0], eeprom_config.dns_server[1], eeprom_config.dns_server[2], eeprom_config.dns_server[3]);
    Ethernet.begin(mac, ip, dns_server, gateway, subnet);
     
  #ifdef DEBUG
    Serial.print("IP: ");
    Serial.println(ip);
    Serial.print("Gateway: ");
    Serial.println(gateway);
    Serial.print("Subnet: ");
    Serial.println(subnet);
    Serial.print("DNS Server: ");
    Serial.println(dns_server);
    Serial.print("MAC: ");
    for (int a=0;a<6;a++) {
      Serial.print(mac[a],HEX);
      if(a<5) {
        Serial.print(":");
      }
    }
	Serial.println();
    Serial.print("UDP Port to listen on: ");
    Serial.println(eeprom_config.localPort);
    Serial.print("Webserver Port: ");
    Serial.println(eeprom_config.webserverPort);
    Serial.print("USE DHCP: ");
    Serial.println(eeprom_config.use_dhcp);
    Serial.print("Config Set: ");
    Serial.println(eeprom_config.config_set);
  #endif
  } else {
    if (Ethernet.begin(mac) == 0) {
      Serial.print("Failed to configure Ethernet using DHCP");
        // print your local IP address:
    }
    Serial.println(Ethernet.localIP());
  }
}
// END Network section #########################################################################################################################################



/* #############################################################################################################################################################
* Create instances of the traffic light class
*/

#define MAX_COMMAND_LENGTH 11    //Max length for commands sent per UDP               

//initialize string objects for the trafficLight() function
String traffic_light_number = "0";
String command = "00000000000";
String parameter = "00000000000";

#include "Traffic_Light.h"
Traffic_Light TL_1;
Traffic_Light TL_2;
Traffic_Light TL_3;
Traffic_Light TL_4;
// #############################################################################################################################################################


/**
* setup function called once at startup
*/
void setup()
{
  Serial.begin(57000);

  /* ##############################################################################################
  * Network section
  */
  //call the setupNetwork function, which reads the EEPROM values and starts the ethernet
  setupNetwork();
  
  //set the UDP Port for receiving the commands
  Udp.begin(eeprom_config.localPort);
 
  webserver = new WebServer(PREFIX, eeprom_config.webserverPort);

  //setup our default command that will be run when the user accesses the root page on the server
  webserver->setDefaultCommand(&indexHtml);

  //run the same command if you try to load /index.html, a common default page name
  webserver->addCommand("index.html", &indexHtml);

  //setup our default command that will be run when the user accesses a page NOT on the server
  webserver->setFailureCommand(&my_failCmd);

  //run the same setupEth command if you try to load setupEth.html
  webserver->addCommand("setupEth.html", &setupEth);
  
  //start the webserver
  webserver->begin();
  // ##############################################################################################
  

  /* ##############################################################################################
  * Traffic light section
  */
  //set the PIN's on the Arduino Board for each Traffic Light
  //.init(RED,YELLOW,GREEN)
  TL_1.init(22,23,25);
  TL_2.init(31,29,28);
  
  //TL_3 and TL_4 only has a red and green sign
  TL_3.init(24,30);
  TL_4.init(26,27);
  
  //test all signs on startup
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
  // ##############################################################################################
}


void trafficLight() {
 // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if(packetSize)
  {
    #ifdef DEBUG
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
    #endif


    // read the packet into packetBufffer
    Udp.read(packetBuffer,MAX_COMMAND_LENGTH);
    
    // make a string of the char array
    parameter = packetBuffer;
    parameter.trim();
    
    // determine Traffic Light number and command
    traffic_light_number = parameter.substring(0,1);
    command = String("");
    command = parameter.substring(1,packetSize);
    
    #ifdef DEBUG
      // some DEBUG output to serial console
      Serial.println("Contents:");
      Serial.println(traffic_light_number);
      Serial.print(command);
      Serial.println(";");
    #endif
    
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



void loop()
{
  char buff[64];
  int len = 64;

  trafficLight();
  
  /* process incoming connections one at a time forever */
  webserver->processConnection(buff, &len);
}
