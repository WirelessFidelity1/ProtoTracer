#pragma once
#include "Adafruit_GFX.h"
#include "Wire.h"
#define SSD1306_SWITCHCAPVCC 0
namespace Simulator { inline uint8_t oled[128*64]={}; }
// Software implementation of the drawing operations used by HeadsUpDisplay.
class Adafruit_SSD1306 {
    uint8_t buffer[128*64]={};
    bool inverted=false;
    const GFXfont* font=nullptr;
    int cursorX=0,cursorY=0;
    uint8_t color=1;
    void pixel(int x,int y,int c) { if(x>=0 && x<128 && y>=0 && y<64) buffer[y*128+x]=c!=0; }
public:
    Adafruit_SSD1306(int,int,SimulatorWire*,int) {}
    bool begin(int,int) { return true; }
    void setFont(const GFXfont* f) { font=f; }
    void setTextSize(int) {}
    void setTextColor(int c) { color=c; }
    void setCursor(int x,int y) { cursorX=x; cursorY=y; }
    void clearDisplay() { memset(buffer,0,sizeof(buffer)); }
    void invertDisplay(bool b) { inverted=b; }
    void display() { for(int i=0;i<128*64;++i) Simulator::oled[i]=buffer[i]^inverted; }
    void fillRect(int x,int y,int w,int h,int c) { for(int r=0;r<h;++r) for(int col=0;col<w;++col) pixel(x+col,y+r,c); }
    void drawBitmap(int x,int y,const uint8_t* bits,int w,int h,int c) {
        for(int r=0;r<h;++r) for(int col=0;col<w;++col)
            if(bits[r*((w+7)/8)+col/8] & (128>>(col%8))) pixel(x+col,y+r,c);
    }
    void println(const String& text) {
        if(!font) return;
        for(unsigned char c:text) {
            if(c<font->first || c>font->last) continue;
            const GFXglyph& g=font->glyph[c-font->first];
            for(int r=0;r<g.height;++r) for(int col=0;col<g.width;++col) {
                int bit=r*g.width+col;
                if(font->bitmap[g.bitmapOffset+bit/8] & (128>>(bit%8)))
                    pixel(cursorX+g.xOffset+col,cursorY+g.yOffset+r,color);
            }
            cursorX+=g.xAdvance;
        }
        cursorX=0; cursorY+=font->yAdvance;
    }
    void getTextBounds(const String& text,int x,int y,int16_t* x1,int16_t* y1,uint16_t* w,uint16_t* h) {
        int left=x,right=x,top=y,bottom=y;
        if(font) for(unsigned char c:text) {
            if(c<font->first || c>font->last) continue;
            const GFXglyph& g=font->glyph[c-font->first];
            left=min(left,x+g.xOffset); right=max(right,x+g.xOffset+g.width);
            top=min(top,y+g.yOffset); bottom=max(bottom,y+g.yOffset+g.height);
            x+=g.xAdvance;
        }
        *x1=left; *y1=top; *w=right-left; *h=bottom-top;
    }
};
