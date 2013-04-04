#include "Traffic_Light.h"

void Traffic_Light::init(byte p_redPIN, byte p_greenPIN) {
 redPIN=p_redPIN;
 pinMode(redPIN, OUTPUT);
 
 greenPIN=p_greenPIN;
 pinMode(greenPIN, OUTPUT); 
}

void Traffic_Light::init(byte p_redPIN, byte p_yellowPIN, byte p_greenPIN) {
 init(p_redPIN,p_greenPIN);
 
 yellowPIN=p_yellowPIN;
 pinMode(yellowPIN, OUTPUT);
}

byte Traffic_Light::signsChanged() {
  int retval = -1;
  
  if (light_states != last_states) {
    retval = light_states;
    last_states=light_states;
  }
  
  return retval;
}

byte Traffic_Light::signsState() {
  return light_states;
}

void Traffic_Light::redOn() {
  digitalWrite(redPIN, HIGH);
  bitWrite(light_states,0,1);
}

void Traffic_Light::redOff() {
  digitalWrite(redPIN, LOW);
  bitWrite(light_states,0,0);
}


void Traffic_Light::yellowOn() {
  digitalWrite(yellowPIN, HIGH);
  bitWrite(light_states,1,1);
}

void Traffic_Light::yellowOff() {
  digitalWrite(yellowPIN, LOW);
  bitWrite(light_states,1,0);
}


void Traffic_Light::greenOn() {
  digitalWrite(greenPIN, HIGH);
  bitWrite(light_states,2,1);
}

void Traffic_Light::greenOff() {
  digitalWrite(greenPIN, LOW);
  bitWrite(light_states,2,0);
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


