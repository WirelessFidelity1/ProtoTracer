#pragma once
#include "Arduino.h"
struct SimulatorEEPROM {
    uint8_t data[512];
    SimulatorEEPROM() { memset(data,255,sizeof(data)); }
    uint8_t read(unsigned i) { return i<sizeof(data)?data[i]:255; }
    void write(unsigned i,uint8_t n) { if(i<sizeof(data)) data[i]=n; }
    void update(unsigned i,uint8_t n) { write(i,n); }
};
inline SimulatorEEPROM EEPROM;
