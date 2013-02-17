#ifndef Traffic_Light_h
#define Traffic_Light_h

#include <Arduino.h>

class Traffic_Light {
  private:
    byte redPIN;
    byte yellowPIN;
    byte greenPIN;
    byte light_states;
    byte last_states;
    
  public:    
    void init(byte p_redPIN, byte p_greenPIN);
    void init(byte p_redPIN, byte p_yellowPIN, byte p_greenPIN);
    
    byte signsChanged();
    
    void allOn();
    void allOff();
    
    void redOn();
    void redOff();
    
    void yellowOn();
    void yellowOff();
    
    void greenOn();
    void greenOff();
};

#endif

