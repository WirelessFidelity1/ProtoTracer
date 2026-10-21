#pragma once

#include <Arduino.h>
#include <math.h>
#include <string.h>
#include "../../../Utils/RGBColor.h"

// Bottom-up merge sort, advanced one output element at a time.
class MergeSort {
    static constexpr int Count=25;
    uint8_t values[Count] = {}, source[Count] = {};
    float heights[Count] = {}, flashes[Count] = {};
    static RGBColor* Frame() {
        DMAMEM static RGBColor pixels[51*32];
        return pixels;
    }
    int run=1, start=0, middle=0, end=0, left=0, right=0, output=0;
    int compareLeft=-1, compareRight=-1;
    bool merging=false, complete=false, paused=false;
    float clock=0, accumulator=0, celebration=0, interval=0.075f;

    FLASHMEM __attribute__((noinline)) static RGBColor Rainbow(int hue, float brightness=1) {
        hue=(hue%768+768)%768;
        int section=hue/256, offset=hue%256;
        int r=section==0 ? 255-offset : section==2 ? offset : 0;
        int g=section==0 ? offset : section==1 ? 255-offset : 0;
        int b=section==1 ? offset : section==2 ? 255-offset : 0;
        return RGBColor(uint8_t(r*brightness),uint8_t(g*brightness),uint8_t(b*brightness));
    }
    void Rect(int x,int y,int w,int h,RGBColor color) {
        RGBColor* pixels=Frame();
        for (int yy=max(0,y);yy<min(32,y+h);++yy)
            for (int xx=max(0,x);xx<min(51,x+w);++xx) pixels[yy*51+xx]=color;
    }
    FLASHMEM __attribute__((noinline)) void Step() {
        if (!merging) {
            if (start>=Count) {
                run*=2; start=0;
                if (run>=Count) { complete=true; celebration=0; return; }
            }
            middle=min(start+run,Count); end=min(start+run*2,Count);
            memcpy(source,values,sizeof(values));
            left=start; right=middle; output=start; merging=true;
        }
        compareLeft=left<middle ? left : -1;
        compareRight=right<end ? right : -1;
        if (left<middle && (right>=end || source[left]<=source[right])) values[output]=source[left++];
        else values[output]=source[right++];
        flashes[output]=1; ++output;
        if (output==end) { merging=false; start=end; }
    }

public:
    FLASHMEM __attribute__((noinline)) void Begin() {
        for (int i=0;i<Count;++i) values[i]=i+1;
        for (int i=Count-1;i>0;--i) {
            int j=random(i+1); uint8_t temp=values[i]; values[i]=values[j]; values[j]=temp;
        }
        for (int i=0;i<Count;++i) { heights[i]=values[i]; flashes[i]=0; }
        run=1; start=0; merging=complete=paused=false;
        compareLeft=compareRight=-1; clock=accumulator=celebration=0;
    }
    RGBColor Pixel(int x,int y) const { return Frame()[y*51+x]; }
    FLASHMEM __attribute__((noinline)) void Update(float dt,bool shuffle,bool pause,int direction) {
        if (shuffle) Begin();
        if (pause) paused=!paused;
        interval=fmaxf(0.025f,fminf(0.15f,interval-direction*0.015f));
        clock+=dt;
        if (!paused) {
            if (complete) {
                celebration+=dt;
                if (celebration>=3) Begin();
            } else {
                accumulator+=dt;
                while (accumulator>=interval && !complete) { accumulator-=interval; Step(); }
            }
        }
        RGBColor* pixels=Frame();
        for (int i=0;i<51*32;++i) pixels[i]=RGBColor(0,0,0);
        for (int i=0;i<Count;++i) {
            heights[i]+=(values[i]-heights[i])*fminf(1,dt*18);
            flashes[i]=fmaxf(0,flashes[i]-dt*4);
            int height=lroundf(heights[i]), x=1+i*2;
            RGBColor color=Rainbow(values[i]*24+int(clock*70));
            Rect(x,30-height,1,height,color);
            // Colored trails and bright caps emphasize each merge write.
            if (flashes[i]>0) {
                Rect(x+1,30-height,1,height,Rainbow(values[i]*24,flashes[i]*0.35f));
                Rect(x,30-height,2,1,RGBColor(255,255,255));
            }
            if (!complete && (i==compareLeft || i==compareRight))
                Rect(x,30-height,1,height,RGBColor(255,245,210));
            if (complete && fabsf(i-celebration*12)<2)
                Rect(x,30-height,2,height,RGBColor(180,255,220));
        }
        Rect(0,31,51,1,RGBColor(12,25,45));
        if (complete) {
            // Traveling rainbow crown and eight sparkling fountains.
            for (int x=0;x<51;++x) Rect(x,0,1,1,Rainbow(x*15+int(clock*300)));
            for (int i=0;i<8;++i) {
                float phase=fmodf(celebration+i*0.17f,1.2f);
                int x=int(25+sinf(i*2.4f)*phase*22);
                int y=int(28-55*phase+35*phase*phase);
                Rect(x,y,1,2,Rainbow(i*96+int(clock*200)));
            }
        } else {
            // Color-coded runs show the current merge width at the top.
            for (int i=0;i<Count;++i)
                Rect(1+i*2,0,1,1,Rainbow((i/run)*130,0.65f));
            if (merging) Rect(start*2,2,(end-start)*2,1,RGBColor(40,130,200));
        }
        if (paused) { Rect(46,2,1,4,RGBColor(255,200,40)); Rect(49,2,1,4,RGBColor(255,200,40)); }
    }
};
