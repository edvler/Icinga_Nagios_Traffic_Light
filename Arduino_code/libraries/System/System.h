#ifndef System_h
#define System_h

#include <Arduino.h>

/**
* System Class.
* In dieser Klasse sind Systemnahe Funktionen gespeichert.
*
* @author Matthias Maderer
* @version 1.1.7
*/
class System {
public:
  /**
  * Gibt die Uptime des Arduions zurück.
  * Format: TAGE:STUNDE:MINUTEN:SEKUNDEN
  * Bsp.: 1:20:23:50 = 1 Tag, 20 Stunden, 23 Minuten und 50 Sekunden
  * @return char *: Pointer!
  */
  char * uptime();
  
  /**
  * Gibt den freien Ram zurück!
  * @return int: Freier Ram
  */ 
  int ramFree();
  
  /**
  * Gibt die größe des Ram-Speichers zurück
  * @return int: Ram gesamt
  */ 
  int ramSize();
  
private:
  char retval[250];
};

#endif

