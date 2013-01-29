#include "Traffic_Light.h"


void Traffic_Light::init(byte p_redPIN, byte p_yellowPIN, byte p_greenPIN) {
 redPIN=p_redPIN;
 pinMode(redPIN, OUTPUT);
 
 yellowPIN=p_yellowPIN;
 pinMode(yellowPIN, OUTPUT);
 
 greenPIN=p_greenPIN;
 pinMode(greenPIN, OUTPUT); 
}

void Traffic_Light::redOn() {
  digitalWrite(redPIN, HIGH);
}

void Traffic_Light::redOff() {
  digitalWrite(redPIN, LOW);
}


void Traffic_Light::yellowOn() {
  digitalWrite(yellowPIN, HIGH);
}

void Traffic_Light::yellowOff() {
  digitalWrite(yellowPIN, LOW);
}


void Traffic_Light::greenOn() {
  digitalWrite(greenPIN, HIGH);
}

void Traffic_Light::greenOff() {
  digitalWrite(greenPIN, LOW);
}

void Traffic_Light::allOn() {
  redOn();
  yellowOn();
  greenOn(); 
}

void Traffic_Light::allOff() {
  redOff();
  yellowOff();
  greenOff(); 
}

