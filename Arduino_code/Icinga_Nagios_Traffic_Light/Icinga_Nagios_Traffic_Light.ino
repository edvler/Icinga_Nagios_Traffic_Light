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




/* #############################################################################################################################################################
* Settings section.
* Some values may be changed
*/
//#define DEBUG					//Display debug output to the serial console
#define PIEZO_PIN 5     //Comment out to use a piezo speaker to announced sign changes
// #############################################################################################################################################################







/* #############################################################################################################################################################
* Code for the EEPROM related things
* 
*/
#include <EEPROM.h>
#include "EEPROMAnything.h"

#define RESET_PIN 40	//Connect a button to this PIN. If the button is hold, an the device is turned on the default ethernet settings are restored.

/*structure which is stored in the eeprom. 
* Look at "EEPROMAnything.h" for the functions storing and reading the struct
*/
struct config_t
{
    byte use_dhcp;
    byte dhcp_refresh_minutes;
    byte mac[6];
    byte ip[4];
    byte gateway[4];
    byte subnet[4];
    byte dns_server[4];
    unsigned int localPort;
    unsigned int webserverPort;
    byte config_set;
    byte play_sound;
} eeprom_config;

/*The default Ethernet settings. 
* This settings are used when no config is present or the reset button is pressed.
*/
void set_EEPROM_Default() {
    eeprom_config.config_set=1;
  
    eeprom_config.use_dhcp=0;
    eeprom_config.dhcp_refresh_minutes=60;
    
    eeprom_config.play_sound=1;
  
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
  
  if (eeprom_config.config_set != 1 || digitalRead(RESET_PIN) == LOW) {
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







/* START Network section #######################################################################################################################################
* Webserver Code
*/
long last_dhcp_renew;
byte dhcp_state;

char packetBuffer[UDP_TX_PACKET_MAX_SIZE];             // buffer to hold incoming udp packets
EthernetUDP Udp;                                       // An EthernetUDP instance to let us send and receive packets over UDP


/**
renewDHCP() function
Renew the DHCP relase in a given interval.
*/
void renewDHCP(int interval) {
  if (interval == 0 ) {
     interval = 1; 
  }
  if (eeprom_config.use_dhcp==1) {
    if((millis() - last_dhcp_renew) >  (interval*60*1000)) {
      last_dhcp_renew=millis();
      dhcp_state = Ethernet.maintain();
    }
  }
}


/**
* setupNetwork() function (webpage)
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
  
  if (eeprom_config.use_dhcp != 1) {
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

/* START Webserver section #####################################################################################################################################
* Webserver Code
*/

//Networkservice  for webserver -- For an example please see http://arduino.cc/en/Tutorial/WebServer
#define WEBSERVER_PORT 80 //HTTP Port for webserver (default 80)

EthernetServer * Webserver;
EthernetClient client;
String request; //String to store requests

String Page_start = "<html><head><title>Icinga/Nagios traffic light</title></head><body>\n";
String Page_end = "</body></html>";
String Get_head = "<h1>GET from ";
String Post_head = "<h1>POST to ";
String Unknown_head = "<h1>UNKNOWN request for ";
String Default_head = "unidentified URL requested.</h1><br>\n";
String Parsed_head = "parsed.html requested.</h1><br>\n";
String Good_tail_begin = "URL tail = '";
String Bad_tail_begin = "INCOMPLETE URL tail = '";
String Tail_end = "'<br>\n";
String Parsed_tail_begin = "URL parameters:<br>\n";
String Parsed_item_separator = " = '";
String Params_end = "End of parameters<br>\n";
String Post_params_begin = "Parameters sent by POST:<br>\n";
String Line_break = "<br>\n";

String DHCP = "<b>USE DHCP: </b>";
String MAC = "<b>MAC address: </b>";
String IP = "<b>IP address: </b>";
String GW = "<b>Gateway address: </b>";
String DNS = "<b>DNS Server address: </b>";
String NM = "<b>Netmask: </b>";
String WSP = "<b>Webserver-port: </b>";
String LP = "<b>UDP port to listen on: </b>";
String DHCP_REFRESH = "<b>DHCP renew time in minutes: </b>";
String PLAY_SOUND = "<b>Play sounds on sign change: </b>";

String Form_start = "<FORM action=\"setupEth.html\" method=\"get\">";
String Form_end = "</FORM>";

String Form_input_text_start = "<input type=\"text\" name=\"";
String Form_input_cb_start = "<input type=\"checkbox\" name=\"";
String Form_input_value  = "\" value=\"";
String Form_input_end = "\">";
String Form_input_send = "<INPUT type=\"submit\" value=\"Send\">";

String br = "<br>";

/**
* serveClient()
* This function checks if a client has connected.
* If a new client is connected, each character is read into the request string and the string is searched for parameters.
* If parameters present, call the parseURL() function to get the parameter names and values.
*/
void serveWebClient() {
  client = Webserver->available(); 
  if (client) { //Is a new client avaliable?
    boolean currentLineIsBlank = true;
    byte currentLine = 0;
    byte currentChar = 0;
    
    boolean requestEnd = false;
    
    request = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();     
        
        //Eine Anfrage ist komplett wenn der Client ein \r\n\r\n (leere Zeile) schickt!        
        if (c == '\n' && currentLineIsBlank) {
          //Wenn das aktuelle Zeichen \n (Newline) ist UND currentLineIsBlank=true (dies bedeutet: auch das vorhergehende Zeichen war ein \n oder \r\n) ist ...
          //... dann ist die Anfrage komplett und kann verarbeitet werden.
          Serial.println(request);
          
          
          if (request.indexOf("?") != -1) {
            //Prï¿½fen ob ein ? im request vorhanden ist. Dies deutet auf Parameter hin.
            printHeader200();
            parseURL();
            indexHTML();
          }
          else if (request.compareTo("/")==0 || request.compareTo("/index.html") == 0){
            //Prï¿½fen ob die Grundseite aufgerufen wurde
            printHeader200();
            indexHTML();
          } 
          else if (request.compareTo("/setupEth.html")==0){
            //Prï¿½fen ob die Grundseite aufgerufen wurde
            printHeader200();
            Serial.println(request);
            setupEthHTML();
          } 
          else {
            //Alles andere ergibt einen ERROR404.
            printHeader404();  
          }
          break;
        }
        
        if (c == '\n') {
          // Es beginnt eine neue Zeile
          currentLineIsBlank = true;

          currentLine++;
          currentChar = 0;
        } 
        else if (c != '\r') {
          // wenn ungleich \r (Carriage Return) kann es sich nur um Zeichen handeln.
          currentLineIsBlank = false;

          //Mit den folgenden Beiden if Bedingungen wird der GET String ausgelesen aus der ersten Zeile des Requests ausgelesen.
          if (currentLine == 0 && currentChar >4 && c == ' ') {
            //Wenn es sich um die erste Zeile handelt UND das aktuelle Zeichen an der Position >4 ist UND das Zeichen ein Leerzeichen ist...
            //entspricht dies dem Ende der angeforderten Seite inkl. Parameter
            requestEnd = true;
          }

          if (currentLine == 0 && currentChar >3 && requestEnd == false) {
            //Wenn es sich um die erste Zeile handelt UND das aktuelle Zeichen an der Position >3 ist UND das Ende der angeforderten URL noch nicht erreich ist...
            //das Zeichen zum request Buffer hinzufï¿½gen.
            request.concat(c);
          }

          //Kleine Sicherheit um bei URL's lï¿½nger 250 Zeichen eine grobe Fehlfunktion zu vermeiden
          if (currentChar < 250) {
            currentChar++;    
          }          
        }
      } //if client.avaliable
    } //while
    client.stop();
    //Serial.println("client disonnected");
  }
}

void setupEthHTML() {
  client.println(Page_start);
  
  client.print(Form_start);

  client.print(MAC);
  for (int a=0;a<6;a++) {
    client.print(Form_input_text_start);
    client.print(a);
    client.print(Form_input_value);
    client.print(eeprom_config.mac[a],HEX);
    client.println(Form_input_end);
  }
  client.println(br);
 
  client.print(IP);
  for (int a=0;a<4;a++) {
    client.print(Form_input_text_start);
    client.print(a+6);
    client.print(Form_input_value);
    client.print(eeprom_config.ip[a]);
    client.println(Form_input_end);
  }
  client.println(br);

  client.print(GW);
  for (int a=0;a<4;a++) {
    client.print(Form_input_text_start);
    client.print(a+10);
    client.print(Form_input_value);
    client.print(eeprom_config.gateway[a]);
    client.println(Form_input_end);
  }
  client.println(br);
  
  client.print(NM);
  for (int a=0;a<4;a++) {
    client.print(Form_input_text_start);
    client.print(a+14);
    client.print(Form_input_value);
    client.print(eeprom_config.subnet[a]);
    client.println(Form_input_end);
  }
  client.println(br);
  
  client.print(DNS);
  for (int a=0;a<4;a++) {
    client.print(Form_input_text_start);
    client.print(a+18);
    client.print(Form_input_value);
    client.print(eeprom_config.dns_server[a]);
    client.println(Form_input_end);
  }
  client.println(br);
  
  client.print(WSP);
  client.print(Form_input_text_start);
  client.print(22);
  client.print(Form_input_value);
  client.print(eeprom_config.webserverPort);
  client.println(Form_input_end);
  client.println(br);

  client.print(LP);
  client.print(Form_input_text_start);
  client.print(23);
  client.print(Form_input_value);
  client.print(eeprom_config.localPort);
  client.println(Form_input_end);
  client.println(br);

  client.print(DHCP);
  client.print(Form_input_cb_start);
  client.print(24);
  client.print(Form_input_value);
  client.print("1\"");
  if (eeprom_config.use_dhcp == 1) {
    client.print("checked");
  }
  client.println(">");

  client.print(DHCP_REFRESH);
  client.print(Form_input_text_start);
  client.print(25);
  client.print(Form_input_value);
  client.print(eeprom_config.dhcp_refresh_minutes);
  client.println(Form_input_end);
  client.println(br);

  client.print(PLAY_SOUND);
  client.print(Form_input_cb_start);
  client.print(26);
  client.print(Form_input_value);
  client.print("1\"");
  if (eeprom_config.play_sound == 1) {
    client.print("checked");
  }
  client.println(">");

  client.print(Form_input_send);
  client.print(Form_end);

  //HTMl Tags schlieÃŸen
  client.println(Page_end);
}


/**
* Diese Funktion wertet die Parameter, welche per URL ï¿½bergeben wurden aus.
* Es kï¿½nnen nur Parameter mit Zahlen ï¿½bergeben werden!
* 
* NICHT zulï¿½ssig ist z.B.: http://heizung/index.html?a=1&0=asdf
* NICHT zulï¿½ssiger Parameter a = 1
* NICHT zulï¿½ssiger Parameter 0 = asfd
* 
* Zulï¿½ssig ist z.B. http://heizung/index.html?0=5&1=5
* Parameter 0 = 5
* Parameter 1 = 5
* 
* Ablauf:
* Die URL wird wird das vorkommen des Zeichens & durchsucht. 
* Die Zeichen bis zum & werden in einen eigenen String param gespeichert.
* Der String wird nach der Postition des Zeichens = durchsucht.
* Anschlieï¿½end wird ein char Array jeweils mit den Zeichen vor dem = und nach dem = gefï¿½llt.
* Der Inhalt wird in einen Int umgewandelt.
* 
* Das Ergebnis wird im var_array abgespeichert.
* Hier ist das Zeichen vor dem = das Array-Element und das Zeichen nach dem = der Wert.
* In der automatic() Funktion wird das var_array wieder ausgelesen und Relay's, Room's Timer, etc.. auf den entsprechenden Wert gesetzt.
*/
void parseURL() {
  String param = "";
  int char_pos = 0;
  boolean more_params = true;
  
  char param_name[3];
  char param_value[10];
  byte param_number;
  
  char_pos = request.indexOf("?");
  
  //Sind ï¿½berhaupt Parameter-Paare vorhanden?
  if (char_pos != -1) {
    //Abschneiden was nicht gebraucht wird. Dies ist alles vor dem ? inkl dem ?
    request = request.substring(char_pos+1);


    eeprom_config.use_dhcp=0;
    eeprom_config.play_sound=0;

    //So lange den String abarbeiten bis Ende erreicht
    while(more_params == true) {
      //Position des nï¿½chsten & festellen
      char_pos = request.indexOf("&");
      
      //Sind weitere Parameter vorhanden ist char_pos ungleich -1 oder handelt es sich um das letzte Parameter-Paar
      if (char_pos != -1) {
        //Weitere Parameter-Paare vorhanden
        param = request.substring(0,char_pos); //das aktuelle Parameter-Paar in einen eigenen String speichern
        request = request.substring(char_pos+1); // url_str um das aktuelle Parameter-Paar kï¿½rzen
      } else {
        //Keine Parameter-Paar mehr vorhanden
        param = request; //das aktuelle Parameter-Paar ist gleich dem url_str
        more_params = false; //Bedingung fï¿½r while Schleifenende setzten
      }
 
      //Position des = Zeichens festellen im Parameter-Paar
      char_pos = param.indexOf("=");


      //Alle Zeichen bis zum = Zeichen in ein char Array speichern
      for (int i = 0; i<char_pos;i++) {
        param_name[i] = param.charAt(i); 
	if (i==(char_pos-1)) {
	  param_name[i+1]='\0';
	}
      }
	  
     
      //Alle Zeichen nach dem = Zeichen in ein char Array speichern
      for (int i = char_pos+1;i < param.length();i++) {
        param_value[i-char_pos-1] = param.charAt(i);
	if (i==(param.length()-1)) {
	  param_value[i-char_pos]='\0';
	}
      }

      param_number = atoi(param_name);  

      #ifdef DEBUG
        client.println(param_number);
        client.println(":");
        client.println(param_value);
        client.println(br);
      #endif
      
      if (param_number >=0 && param_number <=5) {
        eeprom_config.mac[param_number]=strtol(param_value,NULL,16);
      }

      if (param_number >=6 && param_number <=9) {
        eeprom_config.ip[param_number-6]=atoi(param_value);
      }

      if (param_number >=10 && param_number <=13) {
        eeprom_config.gateway[param_number-10]=atoi(param_value);
      }

      if (param_number >=14 && param_number <=17) {
        eeprom_config.subnet[param_number-14]=atoi(param_value);
      }

      if (param_number >=18 && param_number <=21) {
        eeprom_config.dns_server[param_number-18]=atoi(param_value);
      }
      
      if (param_number == 22) {
        eeprom_config.webserverPort=atoi(param_value);
      }
      
      if (param_number == 23) {
        eeprom_config.localPort=atoi(param_value);
      }

      if (param_number == 24) {
        eeprom_config.use_dhcp=atoi(param_value);
      }

      if (param_number == 25) {
        eeprom_config.dhcp_refresh_minutes=atoi(param_value);
      }
      
      if (param_number == 26) {
        eeprom_config.play_sound=atoi(param_value);
      }
      
    }
    
    EEPROM_writeAnything(0, eeprom_config);
  } 
}




/**
Diese Funktionen gibt einen HTML Status 200 OK Header an den Client zurï¿½ck
Wichtig ist Connection: close zu ï¿½bermitteln um die Verbindung zu beenden.
*/
void printHeader200() {
  // send a 200 standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connnection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");  
}

/**
Diese Funktionen gibt einen HTML Status 404 Not Found Header an den Client zurï¿½ck
Wichtig ist Connection: close zu ï¿½bermitteln um die Verbindung zu beenden.
*/
void printHeader404() {
  // send a 200 standard http response header
  client.println("HTTP/1.q 404 Not Found");
  client.println("Content-Type: text/html");
  client.println("Connnection: close");
  client.println();
  client.println("unknown page!");

}

void indexHTML() {
  //Ãœberschrift
  client.println(Page_start);

  client.print("<h1>Icinga/Nagios Traffic Light</h1><br>");
  client.print("<a href=\"setupEth.html\">Ethernet Setup</a>");
  
  client.print("<br><br>RAM: ");
  client.print(Sys.ramFree());
  client.print(" byte free of ");
  client.print(Sys.ramSize());
  client.println(" byte<br><br>");
  client.println("Uptime: ");
  client.print(Sys.uptime());
  
  if (eeprom_config.use_dhcp==1) {
  client.print("<br>DHCP Status: ");
  client.println(dhcp_state);
  }
  

  //HTMl Tags schlieÃŸen
  client.println(Page_end);
}
// ###############################################################################################################################################################


/* #############################################################################################################################################################
* Create instances of the traffic light class
*/

#define MAX_COMMAND_LENGTH 11    //Max length for commands sent per UDP               

#include "Traffic_Light.h"
Traffic_Light TL_1;
Traffic_Light TL_2;
Traffic_Light TL_3;
Traffic_Light TL_4;
// #############################################################################################################################################################


#ifdef PIEZO_PIN
/* #############################################################################################################################################################
* Melody
* Play short Melodys with this Module
*/

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
 
  //Webserver initialisieren und starten
  Webserver = new EthernetServer(eeprom_config.webserverPort);
  Webserver->begin();
  
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

byte sign1_state;
byte sign2_state;
byte sign3_state;
byte sign4_state;

void loop()
{
  //renew DHCP lease
  renewDHCP(eeprom_config.dhcp_refresh_minutes);
  
  //control traffic signs
  trafficLight();
  
  //play sounds on sign changes
  #ifdef PIEZO_PIN
  if (eeprom_config.play_sound == 1) {
    sign1_state = TL_1.signsChanged();
    sign2_state = TL_2.signsChanged();
    sign3_state = TL_3.signsChanged();
    sign4_state = TL_4.signsChanged();
    
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
  
  // check for new webclients
  serveWebClient();
}

void trafficLight() {
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




