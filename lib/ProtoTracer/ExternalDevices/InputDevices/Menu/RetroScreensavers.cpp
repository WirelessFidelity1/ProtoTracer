#include "RetroScreensavers.h"

FLASHMEM __attribute__((noinline)) RGBColor* RetroScreensavers::Frame() {
        DMAMEM static RGBColor pixels[51*32];
        return pixels;
    }

FLASHMEM __attribute__((noinline)) void RetroScreensavers::Rect(int x,int y,int w,int h,RGBColor color) {
        RGBColor* pixels=Frame();
        for (int row=max(0,y);row<min(32,y+h);++row)
            for (int col=max(0,x);col<min(51,x+w);++col) pixels[row*51+col]=color;
    }

FLASHMEM __attribute__((noinline)) void RetroScreensavers::DrawToaster(const Flyer& flyer,int index) {
        int x=lroundf(flyer.x), y=lroundf(flyer.y);
        const RGBColor edge(110,65,20), gold(235,155,45), silver(165,175,185), white(245,245,235);
        if (index%3==2) {
            // Toast: browned crust, a pale center, and toasted speckles.
            Rect(x+1,y,3,1,gold); Rect(x,y+1,5,3,gold); Rect(x+1,y+4,3,1,edge);
            Rect(x+1,y+1,3,3,RGBColor(255,215,125));
            Rect(x+2,y+2,1,1,RGBColor(175,105,35));
            return;
        }
        int flap=lroundf(sinf(time*9+flyer.phase)*3);
        // Feathered wings lift and fall independently of the toaster body.
        for (int i=0;i<4;++i) {
            int wingY=y+1+flap*i/3;
            Rect(x-1-i,wingY,1,3-i/2,white);
            Rect(x+7+i,wingY,1,3-i/2,silver);
        }
        Rect(x+1,y,5,1,white); Rect(x,y+1,7,4,silver);
        Rect(x+1,y+5,5,1,edge); Rect(x+1,y+1,4,1,white);
        Rect(x+1,y+2,4,1,RGBColor(35,30,35));
        Rect(x+2,y+1,3,1,gold); // Bread peeking out of the slot.
        Rect(x+5,y+3,2,1,RGBColor(90,100,110));
        Rect(x+6,y+2,1,1,RGBColor(245,90,45));
        Rect(x+1,y+4,1,1,white);
    }

FLASHMEM __attribute__((noinline)) void RetroScreensavers::Begin() {
    last=millis(); time=0;
    for (auto& flyer:flyers) {
        flyer.x=random(-8,60); flyer.y=random(-6,34);
        flyer.z=random(70,131)/100.0f;
        flyer.phase=random(628)/100.0f;
    }
}

FLASHMEM __attribute__((noinline)) void RetroScreensavers::Update() {
        uint32_t now=millis();
        float dt=fminf(0.1f,(now-last)/1000.0f); last=now; time+=dt;
        RGBColor* pixels=Frame();
        for (int i=0;i<51*32;++i) pixels[i]=RGBColor(0,0,0);
            for (int i=0;i<6;++i) {
                Flyer& flyer=flyers[i];
                flyer.x-=9*flyer.z*dt; flyer.y+=4*flyer.z*dt;
                if (flyer.x<-12 || flyer.y>36) {
                    // Enter from either the top or the right, never pop in mid-screen.
                    if (random(2)) { flyer.x=58; flyer.y=random(-10,24); }
                    else { flyer.x=random(10,65); flyer.y=-10; }
                    flyer.z=random(70,131)/100.0f;
                }
                DrawToaster(flyer,i);
            }

    }

FLASHMEM __attribute__((noinline)) RGBColor RetroScreensavers::Pixel(int x,int y) const { return Frame()[y*51+x]; }
