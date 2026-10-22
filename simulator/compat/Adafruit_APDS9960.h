#pragma once
#include "Arduino.h"
namespace Simulator { inline bool boop=false; }
struct Adafruit_APDS9960 {
    bool begin() { return true; }
    void enableProximity(bool=true) {} void enableColor(bool=true) {}
    uint8_t readProximity() { return Simulator::boop ? 255 : 0; }
    bool colorDataReady() { return true; }
    void getColorData(uint16_t* r,uint16_t* g,uint16_t* b,uint16_t* c) { *r=*g=*b=*c=0; }
};
