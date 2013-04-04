/*
Icinga Nagios Traffic Light

GitHub: https://github.com/edvler/Icinga_Nagios_Traffic_Light
Web-Blog: http://www.edvler-blog.de/icinga_nagios_traffic_light_ampel

Author: Matthias Maderer
Date: 29.01.2013



This is a project for icinga (https://www.icinga.org/) or nagios (http://www.nagios.org/).
Both are monitoring systems for IT infrastructures. Almost everything can be monitored, from a simple host to a complex services.
But at the end it's realy simple. In icinga an nagios are 3 major possible states avaliabe.
Each service or host could be in one of the following states:

- good (green)
- warning (orange)
- red (critical)

This is where my traffic light comes in.
It is used to bring the actual overall state of the monitoring system to a traffic light.

Four traffic lights available. One and two have 3 signs, three and four have 2 signs.

The traffic light is controlled over network! It's possible to configure the network settings over webinterface.
The default IP for the Webinterface is 192.168.0.111.

It's possible to send commands via UDP or per Webrequest over a browser or wget.



Setup:
Build your traffic light according to the traffic_light_schema.fzz (open it with fritzing.org).

Prepare your scetch:
- The traffic light is playing sounds if enabled. See #define PIEZO_PIN
- Decide if you send your commands over web or UDP. See #define USE_UDP
- Check where the reset PIN for the webduino interface is connected. See #define RESET_PIN



Upload this scetch to your Arduino with a Ethernet Shield.
Now use your browser to open the URL http://192.168.0.111/.
You will see a webinterface to configure the network settings.
For more informations about the webinterface look at www.edvler-blog.de/arduino_networksetup_webinterface_with_eeprom

The control_traffic_light.pl reads the actual state from the ndoutils or idoutils database and sends the state to the traffic light.
The only thing you need to do is setup up the IP address and the port of your arduino and your database settings in the control_traffic_light.pl.


Then call it with crontab. 
Now you have your own icinga / nagios traffic light.




It's also possible to send commands manual, to set a individual state if needed. 

How it work's with UDP:
1. On the Icinga server or the Nagios server is the script control_traffic_light.pl called with crontab.
2. The script sends messages with the UDP protocol to the Arduino and changes the traffic light corresponding to the overall status of icinga or nagios.
3. It is also possible to turn on and off the traffic lights from the console:
   - control_traffic_light.pl 1redOn       #to turn the red LED of the traffic light one on.
   - control_traffic_light.pl 1redOff      #to turn the red LED of the traffic light one off.
 
   - control_traffic_light.pl 2yellowOn    #to turn the yellow LED of traffic the light TWO on.
   - control_traffic_light.pl 2yellowOff   #to turn the yellow LED of traffic the light TWO off.
  
   - control_traffic_light.pl 2greenOn     #to turn the green LED of traffic the light TWO on.
   - control_traffic_light.pl 1greenOn     #to turn the grenn LED of traffic the light ONE off.

   - control_traffic_light.pl 3greenOn     #to turn the green LED of traffic the light TWO on.
   - control_traffic_light.pl 4greenOn     #to turn the grenn LED of traffic the light ONE off.

... and so on.
   
   - control_traffic_light.pl allOn        #to turn all LED's of the traffic light on
   - control_traffic_light.pl allOff       #to turn all LED's of the traffic light on
   
commands are case-sensitive!



How it work's with WEB/HTTP requests:
Open your Browser and type one of the following URL's:
http://<ARDUINO_IP>/tl.html?tl2=greenOn - Turn on the green sign of traffic light TWO
http://<ARDUINO_IP>/tl.html?tl2=greenOff - Turn off the green sign of traffic light TWO

http://<ARDUINO_IP>/tl.html?tl1=redOn - Turn on the red sign of traffic light ONE
http://<ARDUINO_IP>/tl.html?tl1=redOff - Turn off the red sign of traffic light ONE

http://<ARDUINO_IP>/tl.html?tl1=yellowOn - Turn on the yellow sign of traffic light ONE
http://<ARDUINO_IP>/tl.html?tl3=yellowOn - Turn on the yellow sign of traffic light THREE
....

you could use "wget --spider http://<ARDUINO_IP>/tl.html?tl3=yellowOn" on your linux console
Please replace <ARDUINO_IP> with the IP of your Arduino


Lizenz:
http://creativecommons.org
"Creative Commons Namensnennung - Nicht-kommerziell - Weitergabe unter gleichen Bedingungen 3.0 Unported Lizenz"
*/

#define WEBDUINO_FAVICON_DATA "" // no favicon
//#define DEBUG  //uncomment for serial debug output
#define USE_SYSTEM_LIBRARY //comment out if you want to save some space (about 1 Byte). You wouldn't see uptime and free RAM if it's commented out.
#define SERIAL_BAUD 9600


#define PIEZO_PIN 5     //Comment out to use a piezo speaker to announced sign changes
#define USE_UDP			//Comment out if don't use UDP. If commented out the only way to send commands is over WEB requests.


#include "SPI.h" // new include
#include "avr/pgmspace.h" // new include
#include "Ethernet.h"
#include "WebServer.h"


/* #############################################################################################################################################################
* Create instances of the traffic light class
*/

#define MAX_COMMAND_LENGTH 11    //Max length for commands         

#include "Traffic_Light.h"
Traffic_Light TL_1;
Traffic_Light TL_2;
Traffic_Light TL_3;
Traffic_Light TL_4;
// #############################################################################################################################################################





/* #############################################################################################################################################################
* Code for the EEPROM related things
* 
*/
#include <EEPROM.h>
#include "EEPROMAnything.h"

#define RESET_PIN 40	//Connect a button to this PIN. If the button is hold, an the device is turned on the default ethernet settings are restored.

/* structure which is stored in the eeprom. 
* Look at "EEPROMAnything.h" for the functions storing and reading the struct
*/
struct config_t
{
    
    byte config_set;
    byte use_dhcp;
    byte dhcp_refresh_minutes;
    byte mac[6];
    byte ip[4];
    byte gateway[4];
    byte subnet[4];
    byte dns_server[4];
    unsigned int webserverPort;
    byte play_sound;
    unsigned int UDPport;
} eeprom_config;

/** 
* set_EEPROM_Default() function
*
* The default settings. 
* This settings are used when no config is present or the reset button is pressed.
*/
void set_EEPROM_Default() {
    eeprom_config.config_set=1;  // dont change! It's used to check if the config is already set
  
    eeprom_config.use_dhcp=0; // use DHCP per default
    eeprom_config.dhcp_refresh_minutes=60; // refresh the DHCP every 60 minutes
    
    eeprom_config.play_sound=0;
    eeprom_config.UDPport=54000;
    
  
    // set the default MAC address. In this case its DE:AD:BE:EF:FE:ED
    eeprom_config.mac[0]=0xDE;  
    eeprom_config.mac[1]=0xAD;
    eeprom_config.mac[2]=0xBE;
    eeprom_config.mac[3]=0xEF;
    eeprom_config.mac[4]=0x11;
    eeprom_config.mac[5]=0x22;
    
    // set the default IP address for the arduino. In this case its 192.168.0.111
    eeprom_config.ip[0]=192;
    eeprom_config.ip[1]=168;
    eeprom_config.ip[2]=0;
    eeprom_config.ip[3]=111;
  
    // set the default GATEWAY. In this case its 192.168.0.254
    eeprom_config.gateway[0]=192;
    eeprom_config.gateway[1]=168;
    eeprom_config.gateway[2]=0;
    eeprom_config.gateway[3]=254;
    
    // set the default SUBNET. In this case its 255.255.255.0
    eeprom_config.subnet[0]=255;
    eeprom_config.subnet[1]=255;
    eeprom_config.subnet[2]=255;
    eeprom_config.subnet[3]=0;

    // set the default DNS SERVER. In this case its 192.168.0.254
    eeprom_config.dns_server[0]=192;
    eeprom_config.dns_server[1]=168;
    eeprom_config.dns_server[2]=0;
    eeprom_config.dns_server[3]=254;

    // set the default Webserver Port. In this case its Port 80
    eeprom_config.webserverPort=80;
    
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
  
  // read the current config
  EEPROM_readAnything(0, eeprom_config);
  
  // check if config is present or if reset button is pressed
  if (eeprom_config.config_set != 1 || digitalRead(RESET_PIN) == LOW) {
    // set default values
    set_EEPROM_Default();
    
    // write the config to eeprom
    EEPROM_writeAnything(0, eeprom_config);
  } 
}

/**
* print_EEPROM_Settings() function
*
* This function is used for debugging the configuration.
* It prints the actual configuration to the serial port.
*/
#ifdef DEBUG
void print_EEPROM_Settings() {
    Serial.print("IP: ");
    for(int i = 0; i<4; i++) {
      Serial.print(eeprom_config.ip[i]);
      if (i<3) {
        Serial.print('.');
      }
    }
    Serial.println();
  
    Serial.print("Subnet: ");
    for(int i = 0; i<4; i++) {
      Serial.print(eeprom_config.subnet[i]);
      if (i<3) {
        Serial.print('.');
      }
    }
    Serial.println();
    
    Serial.print("Gateway: ");
    for(int i = 0; i<4; i++) {
      Serial.print(eeprom_config.gateway[i]);
      if (i<3) {
        Serial.print('.');
      }
    }
    Serial.println();

    Serial.print("DNS Server: ");
    for(int i = 0; i<4; i++) {
      Serial.print(eeprom_config.dns_server[i]);
      if (i<3) {
        Serial.print('.');
      }
    }
    Serial.println();
    
    Serial.print("MAC: ");
    for (int a=0;a<6;a++) {
      Serial.print(eeprom_config.mac[a],HEX);
      if(a<5) {
        Serial.print(":");
      }
    }
    Serial.println();
    
    Serial.print("Webserver Port: ");
    Serial.println(eeprom_config.webserverPort);
    
    Serial.print("USE DHCP: ");
    Serial.println(eeprom_config.use_dhcp);
    
    Serial.print(" DHCP renew every ");
    Serial.print(eeprom_config.dhcp_refresh_minutes);
    Serial.println(" minutes");
    
    Serial.print("Config Set: ");
    Serial.println(eeprom_config.config_set);

}
#endif

// #############################################################################################################################################################


/* START Network section #######################################################################################################################################
* Code for setting up network connection
*/
unsigned long last_dhcp_renew;
byte dhcp_state;

#ifdef USE_UDP
  EthernetUDP Udp;                                       // An EthernetUDP instance to let us send and receive packets over UDP
  char packetBuffer[UDP_TX_PACKET_MAX_SIZE];             // buffer to hold incoming udp packets
#endif

/**
* renewDHCP() function
* Renew the DHCP relase in a given interval.
* 
* Overview:
* - Check if interval = 0 and set it to 1
* - Check if renew interval is reached and renew the lease
*/
void renewDHCP(int interval) {
  unsigned long interval_millis = interval * 60000;

  if (interval == 0 ) {
     interval = 1; 
  }
  if (eeprom_config.use_dhcp==1) {
    if((millis() - last_dhcp_renew) >  interval_millis) {
      last_dhcp_renew=millis();
      dhcp_state = Ethernet.maintain();
    }
  }
}


/**
* setupNetwork() function
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
  
  #ifdef DEBUG
   print_EEPROM_Settings();
  #endif

 // byte mac[] = { eeprom_config.mac[0], eeprom_config.mac[1], eeprom_config.mac[2], eeprom_config.mac[3], eeprom_config.mac[4], eeprom_config.mac[5] };  
  
  if (eeprom_config.use_dhcp != 1) {
    IPAddress ip(eeprom_config.ip[0], eeprom_config.ip[1], eeprom_config.ip[2], eeprom_config.ip[3]);                                               
    IPAddress gateway (eeprom_config.gateway[0],eeprom_config.gateway[1],eeprom_config.gateway[2],eeprom_config.gateway[3]);                      
    IPAddress subnet  (eeprom_config.subnet[0], eeprom_config.subnet[1], eeprom_config.subnet[2], eeprom_config.subnet[3]);  
    IPAddress dns_server  (eeprom_config.dns_server[0], eeprom_config.dns_server[1], eeprom_config.dns_server[2], eeprom_config.dns_server[3]);
    Ethernet.begin(eeprom_config.mac, ip, dns_server, gateway, subnet);
  } else {
    if (Ethernet.begin(eeprom_config.mac) == 0) {
      Serial.print("Failed to configure Ethernet using DHCP");
    }
    Serial.println(Ethernet.localIP());
  }
}
// END Network section #########################################################################################################################################


/* WEB-Server section #######################################################################################################################################
* Webserver Code
*/

#ifdef USE_SYSTEM_LIBRARY
#include <System.h>
System sys;
#endif

/* Store all string in the FLASH storage to free SRAM.
The P() is a function from Webduino.
*/
P(Page_start) = "<html><head><title>Web_EEPROM_Setup</title></head><body>\n";
P(Page_end) = "</body></html>";

P(Http400) = "HTTP 400 - BAD REQUEST";
P(Index) = "<h1>index.html</h1><br>Welcome to Nagios and Icinga traffic light! Please set up your Netzwork: <a href=\"setupNet.html\">NETWORK SETUP</a>";

P(Form_eth_start) = "<FORM action=\"setupNet.html\" method=\"get\">";
P(Form_end) = "<FORM>";
P(Form_input_send) = "<INPUT type=\"submit\" value=\"Set config\">";

P(Form_input_text_start) = "<input type=\"text\" name=\"";
P(Form_input_value)  = "\" value=\"";
P(Form_input_size2) = "\" maxlength=\"2\" size=\"2";
P(Form_input_size3) = "\" maxlength=\"3\" size=\"3";
P(Form_input_end) = "\">\n";

P(MAC) = "MAC address: ";
P(IP) = "IP address: ";
P(SUBNET) = "Subnet: ";
P(GW) = "GW address: ";
P(DNS_SERVER) = "DNS server: ";
P(WEB_PORT) = "Webserver port (1-65535): ";
P(DHCP_ACTIVE) = "Use DHCP: ";
P(DHCP_REFRESH) = "Renew interval for DHCP in minutes (1 - 255): ";

P(Form_cb) = "<input type=\"radio\" name=\"23\" value=\"";
P(Form_cb_checked) = " checked ";
P(Form_cb_on) = ">On";
P(Form_cb_off) = ">Off";

P(br) = "<br>\n";

P(table_start) = "<table>";
P(table_tr_start) = "<tr>";
P(table_tr_end) = "</tr>";
P(table_td_start) = "<td>";
P(table_td_end) = "</td>";
P(table_end) = "</table>";

P(Config_set) = "<font size=\"6\" color=\"red\">New configuration stored! <br>Please turn off and on your Arduino or use the reset button!</font><br>";

P(DHCP_STATE_TIME) = "DHCP last renew timestamp (sec)";
P(DHCP_STATE) = "DHCP renew return code (sec)";

P(UDP_PORT) = "UDP Port for receiving commands:";
P(SOUND) = "Play sounds (1=on, 0=0ff):";


P(UPTIME) = "Uptime: ";

#ifdef USE_SYSTEM_LIBRARY
P(RAM_1) = "RAM (byte): ";
P(RAM_2) = " free of ";
#endif

/* This creates an pointer to instance of the webserver. */
WebServer * webserver;


/**
* indexHTML() function
* This function is used to send the index.html to the client.
*/
void indexHTML(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  /* this line sends the standard "we're all OK" headers back to the
     browser */
  server.httpSuccess();

  /* if we're handling a GET or POST, we can output our data here.
     For a HEAD request, we just stop after outputting headers. */
  if (type == WebServer::HEAD)
    return;
    
  server.printP(Page_start);
  
  server.printP(Index);
  
  server.printP(Page_end);
    
}

/**
* setupNetHTML() function
* This function is used to send the setupNet.html to the client.
*
* Overview:
* - Send a HTTP 200 OK Header
* - If get parameters exists assign them to the corresponding variable in the eeprom_config struct
* - Print the configuration
*
* Parameters are simple numbers. The name of the parameter is converted to an int with the atoi function.
* This saves some code for setting the MAC and IP addresses.
*/
#define NAMELEN 5
#define VALUELEN 7
void setupNetHTML(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  URLPARAM_RESULT rc;
  char name[NAMELEN];
  char value[VALUELEN];
  boolean params_present = false;
  byte param_number = 0;

  /* this line sends the standard "we're all OK" headers back to the
     browser */
  server.httpSuccess();

  /* if we're handling a GET or POST, we can output our data here.
     For a HEAD request, we just stop after outputting headers. */
  if (type == WebServer::HEAD)
    return;

  server.printP(Page_start);

  // check for parameters
  if (strlen(url_tail)) {
    while (strlen(url_tail)) {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc != URLPARAM_EOS) {
        params_present=true;
        // debug output for parameters
        #ifdef DEBUG
        Serial.print(name);
        server.print(name);
        Serial.print(" - "); 
        server.print(" - ");
        Serial.println(value);
        server.print(value);
        server.print("<br>");
        #endif
        
        
        param_number = atoi(name);
 
        // read MAC address
        if (param_number >=0 && param_number <=5) {
          eeprom_config.mac[param_number]=strtol(value,NULL,16);
        }
    
        // read IP address
        if (param_number >=6 && param_number <=9) {
          eeprom_config.ip[param_number-6]=atoi(value);
        }
    
        // read SUBNET
        if (param_number >=10 && param_number <=13) {
          eeprom_config.subnet[param_number-10]=atoi(value);
        }
    
        // read GATEWAY
        if (param_number >=14 && param_number <=17) {
          eeprom_config.gateway[param_number-14]=atoi(value);
        }
    
        // read DNS-SERVER
        if (param_number >=18 && param_number <=21) {
          eeprom_config.dns_server[param_number-18]=atoi(value);
        }
        
        // read WEBServer port
        if (param_number == 22) {
          eeprom_config.webserverPort=atoi(value);
        }
        
        // read DHCP ON/OFF
        if (param_number == 23) {
          eeprom_config.use_dhcp=atoi(value);
        }
    
        // read DHCP renew interval
        if (param_number == 24) {
          eeprom_config.dhcp_refresh_minutes=atoi(value);
        }

        // read DHCP renew interval
        if (param_number == 25) {
          eeprom_config.UDPport=atoi(value);
        } 

        // read DHCP renew interval
        if (param_number == 26) {
          eeprom_config.play_sound=atoi(value);
        } 

      }
    }
    EEPROM_writeAnything(0, eeprom_config);
  }

  //print the form
  server.printP(Form_eth_start);
  
  if(params_present==true) {
     server.printP(Config_set);
  }
    
  server.printP(table_start);
  
  // print the current MAC
  server.printP(table_tr_start);
  server.printP(table_td_start);
  server.printP(MAC);
  server.printP(table_td_end);
  server.printP(table_td_start);
  for (int a=0;a<6;a++) {
    server.printP(Form_input_text_start);
    server.print(a);
    server.printP(Form_input_value);
    server.print(eeprom_config.mac[a],HEX);
    server.printP(Form_input_size2);    
    server.printP(Form_input_end);
  }
  server.printP(table_td_end);
  server.printP(table_tr_end);

  // print the current IP
  server.printP(table_tr_start);
  server.printP(table_td_start);
  server.printP(IP);
  server.printP(table_td_end);
  server.printP(table_td_start);    
  for (int a=0;a<4;a++) {
    server.printP(Form_input_text_start);
    server.print(a+6);
    server.printP(Form_input_value);
    server.print(eeprom_config.ip[a]);
    server.printP(Form_input_size3);
    server.printP(Form_input_end);
  }
  server.printP(table_td_end);
  server.printP(table_tr_end);
  

  // print the current SUBNET
  server.printP(table_tr_start);
  server.printP(table_td_start);
  server.printP(SUBNET);
  server.printP(table_td_end);
  server.printP(table_td_start); 
  for (int a=0;a<4;a++) {
    server.printP(Form_input_text_start);
    server.print(a+10);
    server.printP(Form_input_value);
    server.print(eeprom_config.subnet[a]);
    server.printP(Form_input_size3);
    server.printP(Form_input_end);
  }
  server.printP(table_td_end);
  server.printP(table_tr_end);

  // print the current GATEWAY
  server.printP(table_tr_start);
  server.printP(table_td_start);
  server.printP(GW);
  server.printP(table_td_end);
  server.printP(table_td_start); 
  for (int a=0;a<4;a++) {
    server.printP(Form_input_text_start);
    server.print(a+14);
    server.printP(Form_input_value);
    server.print(eeprom_config.gateway[a]);
    server.printP(Form_input_size3);
    server.printP(Form_input_end);
  }
  server.printP(table_td_end);
  server.printP(table_tr_end);

  // print the current DNS-SERVER
  server.printP(table_tr_start);
  server.printP(table_td_start);
  server.printP(DNS_SERVER);
  server.printP(table_td_end);
  server.printP(table_td_start); 
  for (int a=0;a<4;a++) {
    server.printP(Form_input_text_start);
    server.print(a+18);
    server.printP(Form_input_value);
    server.print(eeprom_config.dns_server[a]);
    server.printP(Form_input_size3);
    server.printP(Form_input_end);
  }
  server.printP(table_td_end);
  server.printP(table_tr_end);

  
  // print the current webserver port
  server.printP(table_tr_start);
  server.printP(table_td_start);
  server.printP(WEB_PORT);
  server.printP(table_td_end);
  server.printP(table_td_start);
  server.printP(Form_input_text_start);
  server.print(22);
  server.printP(Form_input_value);
  server.print(eeprom_config.webserverPort);
  server.printP(Form_input_end);
  server.printP(table_td_end);
  server.printP(table_tr_end);
  
  //print the current DHCP config
  server.printP(table_tr_start);
  server.printP(table_td_start);
  server.printP(DHCP_ACTIVE);
  server.printP(table_td_end);
  server.printP(table_td_start);
  server.printP(Form_cb);
  server.print("0\"");
   if(eeprom_config.use_dhcp != 1) {
    server.printP(Form_cb_checked);
  }
  server.printP(Form_cb_off);   
  
  server.printP(Form_cb);
  server.print("1\"");
  if(eeprom_config.use_dhcp == 1) {
    server.printP(Form_cb_checked);
  }
  server.printP(Form_cb_on);   
  server.printP(table_td_end);
  server.printP(table_tr_end);
  
  //print the current DHCP renew time
  server.printP(table_tr_start);
  server.printP(table_td_start);
  server.printP(DHCP_REFRESH);
  server.printP(table_td_end);
  server.printP(table_td_start);
  server.printP(Form_input_text_start);
  server.print(24);
  server.printP(Form_input_value);
  server.print(eeprom_config.dhcp_refresh_minutes);
  server.printP(Form_input_size3);
  server.printP(Form_input_end);
  server.printP(table_td_end);
  server.printP(table_tr_end);

  //print DHCP status
  if(eeprom_config.use_dhcp == 1) {
    server.printP(table_tr_start);
    server.printP(table_td_start);	
    server.printP(DHCP_STATE);
    server.printP(table_td_end);
    server.printP(table_td_start);
    server.print(dhcp_state);
    server.printP(table_td_end);
    server.printP(table_tr_end);
	 
    server.printP(table_tr_start);
    server.printP(table_td_start);	
    server.printP(DHCP_STATE_TIME);
    server.printP(table_td_end);
    server.printP(table_td_start);
    server.print(last_dhcp_renew/1000);
    server.printP(table_td_end);
    server.printP(table_tr_end);
  }
  
  #ifdef USE_SYSTEM_LIBRARY
  //print uptime
  server.printP(table_tr_start);
  server.printP(table_td_start);	
  server.printP(UPTIME);
  server.printP(table_td_end);
  server.printP(table_td_start);
  server.print(sys.uptime());
  server.printP(table_td_end);
  server.printP(table_tr_end); 

  server.printP(table_tr_start);
  server.printP(table_td_start);
  server.printP(RAM_1);	
  server.print(sys.ramFree());
  server.printP(RAM_2);
  server.print(sys.ramSize());
  server.printP(table_td_end);
  server.printP(table_tr_end); 
  #endif

  // print the current webserver port
  server.printP(table_tr_start);
  server.printP(table_td_start);
  server.printP(UDP_PORT);
  server.printP(table_td_end);
  server.printP(table_td_start);
  server.printP(Form_input_text_start);
  server.print(25);
  server.printP(Form_input_value);
  server.print(eeprom_config.UDPport);
  server.printP(Form_input_end);
  server.printP(table_td_end);
  server.printP(table_tr_end);

  // print the current webserver port
  server.printP(table_tr_start);
  server.printP(table_td_start);
  server.printP(SOUND);
  server.printP(table_td_end);
  server.printP(table_td_start);
  server.printP(Form_input_text_start);
  server.print(26);
  server.printP(Form_input_value);
  server.print(eeprom_config.play_sound);
  server.printP(Form_input_end);
  server.printP(table_td_end);
  server.printP(table_tr_end);

  
  server.printP(table_end);
  
  //print the send button
  server.printP(Form_input_send);    
  server.printP(Form_end);
  

  
  server.printP(Page_end);

}

/**
* errorHTML() function
* This function is called whenever a non extisting page is called.
* It sends a HTTP 400 Bad Request header and the same as text.
*/
void errorHTML(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  /* this line sends the standard "HTTP 400 Bad Request" headers back to the
     browser */
  server.httpFail();

  /* if we're handling a GET or POST, we can output our data here.
     For a HEAD request, we just stop after outputting headers. */
  if (type == WebServer::HEAD)
    return;
    
  server.printP(Http400);
  
  server.printP(Page_end);
}
// END WEBCODE ######################################################################################################################################################

#define NAMELEN_TL 5
#define VALUELEN_TL 12
void tlHTML(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  URLPARAM_RESULT rc;
  char name[NAMELEN_TL];
  char value[VALUELEN_TL];
  boolean params_present = false;
  int param_number = 0;

  /* this line sends the standard "we're all OK" headers back to the
     browser */
  server.httpSuccess();

  /* if we're handling a GET or POST, we can output our data here.
     For a HEAD request, we just stop after outputting headers. */
 // if (type == WebServer::HEAD)
 //   return;

  // check for parameters
  if (strlen(url_tail)) {
    while (strlen(url_tail)) {
		rc = server.nextURLparam(&url_tail, name, NAMELEN_TL, value, VALUELEN_TL);
		if (rc != URLPARAM_EOS) {
                        params_present=true;
                        // debug output for parameters
                        #ifdef DEBUG
                        Serial.print(name);
                        server.print(name);
                        Serial.print(" - "); 
                        server.print(" - ");
                        Serial.println(value);
                        server.print(value);
                        server.print("<br>");
                        #endif
  
			param_number = atoi(&name[2]);
                        Serial.println(param_number);
			trafficLight(param_number,&String(value));
		}
    }
  }

  server.print("OK");

}

#ifdef PIEZO_PIN
/* #############################################################################################################################################################
* Melody
* Play short Melodys with this Module
*/

// storage 
byte sign1_state;
byte sign2_state;
byte sign3_state;
byte sign4_state;


#include "pitches.h"
#define NOTE_COUNT 8  //Max avaliable notes

int melody[NOTE_COUNT];
int noteDurations[NOTE_COUNT];
int notes_to_play=0;

void setMelody(byte nr) {
  if (nr == 0) {
    melody[0] = NOTE_C4;
    melody[1] = NOTE_C3;
    melody[2] = NOTE_C2;
    melody[3] = NOTE_C1;

    noteDurations[0] = 3;
    noteDurations[1] = 3;
    noteDurations[2] = 3;
    noteDurations[3] = 1;
    
    notes_to_play = 4;
  }
  

  if (nr == 1) {
    melody[0] = NOTE_C4;
    melody[1] = NOTE_G3;
    melody[2] = NOTE_G3;
    melody[3] = NOTE_A3;
    melody[4] = NOTE_G3;
    melody[5] = 0;
    melody[6] = NOTE_B3;
    melody[7] = NOTE_C4;
    
    noteDurations[0] = 4;
    noteDurations[1] = 8;
    noteDurations[2] = 8;
    noteDurations[3] = 4;
    noteDurations[4] = 4;
    noteDurations[5] = 4;
    noteDurations[6] = 4;
    noteDurations[7] = 4;
    
    notes_to_play = 8;
    
  }  
}

void playMelody() {

  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < notes_to_play; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000/noteDurations[thisNote];
    tone(PIEZO_PIN, melody[thisNote],noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(PIEZO_PIN);
  } 
}
// ###############################################################################################################################################################
#endif






/**
* setup() function
* This function is called whenever the arduino is turned on.
*/
void setup()
{
  Serial.begin(SERIAL_BAUD);
  
  /* initialize the Ethernet adapter with the settings from eeprom */
  delay(200); // some time to settle
  setupNetwork();
  delay(200); // some time to settle
  
  #define PREFIX ""
  webserver = new WebServer(PREFIX, eeprom_config.webserverPort);

  /* setup our default command that will be run when the user accesses
   * the root page on the server */
  webserver->setDefaultCommand(&indexHTML);

  /* setup our default command that will be run when the user accesses
   * a page NOT on the server */
  webserver->setFailureCommand(&errorHTML);

  /* run the same command if you try to load /index.html, a common
   * default page name */
  webserver->addCommand("index.html", &indexHTML);

  /* display a network setup form. The configuration is stored in eeprom */
  webserver->addCommand("setupNet.html", &setupNetHTML);

  /* page for traffic light commands. */
  webserver->addCommand("tl.html", &tlHTML);
  
  /* start the webserver */
  webserver->begin();
  
#ifdef USE_UDP
  //set the UDP Port for receiving the commands
  Udp.begin(eeprom_config.UDPport);
#endif

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

/**
* loop() function
* Runs forver ....
* 
* Overview:
* - Renew the DHCP lease
* - Serve web clients
*
*/
void loop()
{
  // renew DHCP lease
  renewDHCP(eeprom_config.dhcp_refresh_minutes);

  char buff[200];
  int len = 200;

  /* process incoming connections one at a time forever */
  webserver->processConnection(buff, &len);
  
#ifdef USE_UDP
  //control traffic signs with UDP protocol
  trafficLightUDP();
#endif
  
  
#ifdef PIEZO_PIN
  //play sounds on sign changes
  sign1_state = TL_1.signsChanged();
  sign2_state = TL_2.signsChanged();
  sign3_state = TL_3.signsChanged();
  sign4_state = TL_4.signsChanged();
    
  if (eeprom_config.play_sound == 1) {
    //if only the green sign is on play a "good" melody
    if (sign1_state == 4 || sign2_state == 4 || sign3_state == 4 || sign4_state == 4) {
      setMelody(1);
      playMelody();
    }
  
    //if the red or yellow sign is on play a "good" melody
    if ((sign1_state <=3 && sign1_state >0) || (sign2_state <=3 && sign2_state >0) || (sign3_state <=3 && sign3_state >0) || (sign4_state <=3 && sign4_state >0) ) {
      setMelody(0);
      playMelody();
    }
  }
#endif

}

//counter for received commands
#ifdef DEBUG 
long cmd_count = 0;
#endif

void trafficLight(byte traffic_light_number, String * command) {
  
    #ifdef DEBUG 
      cmd_count++;
      Serial.print("Commands count: ");
      Serial.println(cmd_count);
      Serial.print("RAM: ");
      Serial.print(sys.ramFree());
      Serial.print(" byte free of ");
      Serial.println(sys.ramSize());
      Serial.print("Sekunden: ");
      Serial.println(millis()/1000);
      Serial.println();
    #endif
    
    // if the command is for all Traffic Lights
    if(traffic_light_number == 0) {
      if(command->compareTo("allOn") == 0) 
      {
        TL_1.allOn(); 
        TL_2.allOn(); 
        TL_3.allOn(); 
        TL_4.allOn(); 
      }
      if(command->compareTo("allOff") == 0) 
      {
        TL_1.allOff(); 
        TL_2.allOff(); 
        TL_3.allOff(); 
        TL_4.allOff();

      }             
    }

    //Traffic Light 1
    if(traffic_light_number == 1) 
    {
      if(command->compareTo("redOn") == 0) 
      {
        TL_1.redOn(); 
      }
      if(command->compareTo("redOff") == 0) 
      {
        TL_1.redOff(); 
      }
      if(command->compareTo("yellowOn") == 0) 
      {
        TL_1.yellowOn(); 
      }
      if(command->compareTo("yellowOff") == 0) 
      {
        TL_1.yellowOff(); 
      }
      if(command->compareTo("greenOn") == 0) 
      {
        TL_1.greenOn(); 
      }
      if(command->compareTo("greenOff") == 0) 
      {
        TL_1.greenOff(); 
      }
    } 
 
    //Traffic Light 2
    if(traffic_light_number == 2) 
    {
      if(command->compareTo("redOn") == 0) 
      {
        TL_2.redOn(); 
      }
      if(command->compareTo("redOff") == 0) 
      {
        TL_2.redOff(); 
      }
      if(command->compareTo("yellowOn") == 0) 
      {
        TL_2.yellowOn(); 
      }
      if(command->compareTo("yellowOff") == 0) 
      {
        TL_2.yellowOff(); 
      }
      if(command->compareTo("greenOn") == 0) 
      {
        TL_2.greenOn(); 
      }
      if(command->compareTo("greenOff") == 0) 
      {
        TL_2.greenOff(); 
      }
    } 

    //Traffic Light 3 - only red and green sign
    if(traffic_light_number == 3) 
    {
      if(command->compareTo("redOn") == 0) 
      {
        TL_3.redOn(); 
      }
      if(command->compareTo("redOff") == 0) 
      {
        TL_3.redOff(); 
      }
      if(command->compareTo("greenOn") == 0) 
      {
        TL_3.greenOn(); 
      }
      if(command->compareTo("greenOff") == 0) 
      {
        TL_3.greenOff(); 
      }
    } 

    //Traffic Light 4 - only red and green sign
    if(traffic_light_number == 4) 
    {
      if(command->compareTo("redOn") == 0) 
      {
        TL_4.redOn(); 
      }
      if(command->compareTo("redOff") == 0) 
      {
        TL_4.redOff(); 
      }
      if(command->compareTo("greenOn") == 0) 
      {
        TL_4.greenOn(); 
      }
      if(command->compareTo("greenOff") == 0) 
      {
        TL_4.greenOff(); 
      }
    } 	
  }


#ifdef USE_UDP
void trafficLightUDP() {
  String traffic_light_number;
  String command;

  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if(packetSize)
  {
    Udp.read(packetBuffer,MAX_COMMAND_LENGTH);

    // get traffic light number
    traffic_light_number += packetBuffer[0];
    traffic_light_number += '\0';

    // get command
    for (int i=1;i<packetSize;i++) {
      command += packetBuffer[i];
    }
    command += '\0';

    // some DEBUG output to serial console
    #ifdef DEBUG
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
      
      Serial.println("Contents:");
      for (int pc=0;pc<packetSize;pc++) {
        Serial.print(packetBuffer[pc]);
      }
      Serial.println();

      // some DEBUG output to serial console
      Serial.print("Traffic Light: \"");
      Serial.print(traffic_light_number);
      Serial.println("\";");
      Serial.print("Command: \"");
      Serial.print(command);
      Serial.println("\";");
    #endif
    
    if (command.compareTo("llOn") == 0) {
      command = "allOn\0";
    } else if (command.compareTo("llOff") == 0) {
      command = "allOff\0";
    }
    
    int lnr = atoi(&traffic_light_number[0]);
    
    if (lnr >= 0 && lnr <=4) {
      trafficLight(lnr,&command);
    }
  }
}
#endif
