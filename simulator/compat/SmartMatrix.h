#pragma once
#include "Arduino.h"
#define SM_PANELTYPE_HUB75_32ROW_MOD16SCAN 0
#define SM_HUB75_OPTIONS_NONE 0
#define SM_BACKGROUND_OPTIONS_NONE 0
#define SM_APA102_OPTIONS_COLOR_ORDER_BGR 0
struct rgb24 {
    uint8_t red=0,green=0,blue=0;
    rgb24()=default;
    rgb24(int r,int g,int b):red(r),green(g),blue(b) {}
};
namespace Simulator { inline rgb24 panels[64*64], accents[176]; }
template<int W,int H> struct SimulatorLayer {
    void drawPixel(int x,int y,rgb24 c) {
        if(x>=0 && x<W && y>=0 && y<H) {
            if constexpr(H==1) Simulator::accents[x]=c;
            else Simulator::panels[y*W+x]=c;
        }
    }
    void swapBuffers(bool=true) {} void setBrightness(int) {}
};
struct SimulatorMatrix {
    template<class T> void addLayer(T*) {}
    void begin() {} void setRefreshRate(int) {} void setBrightness(int) {}
};
#define SMARTMATRIX_ALLOCATE_BUFFERS(name,...) SimulatorMatrix name
#define SMARTMATRIX_APA_ALLOCATE_BUFFERS(name,...) SimulatorMatrix name
#define SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(name,w,h,...) SimulatorLayer<w,h> name
