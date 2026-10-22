#pragma once
#include "Arduino.h"
#define NEO_GRB 0
#define NEO_KHZ800 0
namespace Simulator { inline uint32_t antenna[5]={}; inline uint8_t antennaBrightness=255; }
struct Adafruit_NeoPixel {
    Adafruit_NeoPixel(int,int,int) {}
    void begin() {} void show() {} void clear() { memset(Simulator::antenna,0,sizeof(Simulator::antenna)); }
    void setBrightness(uint8_t n) { Simulator::antennaBrightness=n; }
    uint16_t numPixels() { return 5; }
    uint32_t getPixelColor(unsigned i) { return i<5 ? Simulator::antenna[i] : 0; }
    static uint32_t Color(uint8_t r,uint8_t g,uint8_t b) { return (r<<16)|(g<<8)|b; }
    void setPixelColor(unsigned i,uint32_t c) { if(i<5) Simulator::antenna[i]=c; }
    void setPixelColor(unsigned i,uint8_t r,uint8_t g,uint8_t b) { setPixelColor(i,Color(r,g,b)); }
};
