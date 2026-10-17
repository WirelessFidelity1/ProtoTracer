#pragma once

#include <Arduino.h>
#include <math.h>
#include <string.h>
#include "../ButtonHandler.h"
#include "../../../Utils/RGBColor.h"

// A native 64x32 canvas. No allocation or blocking delays during animation.
class Games {
    enum Screen { Faces, Selection, Slots };
    Screen screen = Faces;
    RGBColor pixels[64 * 32];
    float pos[3] = {0, 3, 6};
    float speed[3] = {};
    float stop[3] = {};
    bool stopped[3] = {true, true, true};
    bool spinning = false, win = false;
    float elapsed = 0, banner = 0;
    uint32_t last = 0;

    const RGBColor black = RGBColor(0,0,0), white = RGBColor(240,250,250);
    const RGBColor gold = RGBColor(245,195,45), green = RGBColor(65,220,100);

    int Symbol(int reel, int index) const {
        static const uint8_t reels[3][12] = {
            {0,1,2,3,0,2,1,3,0,1,2,3},
            {1,2,0,3,2,1,3,0,1,0,2,3},
            {2,0,3,1,0,2,1,3,2,1,0,3}
        };
        return reels[reel][(index % 12 + 12) % 12];
    }
    void Rect(int x, int y, int w, int h, RGBColor c) {
        for (int yy = y; yy < y+h; ++yy)
            for (int xx = x; xx < x+w; ++xx)
                if (xx >= 0 && xx < 64 && yy >= 0 && yy < 32) pixels[yy*64+xx] = c;
    }
    void SymbolDraw(int name, int cx, int cy) {
        int x = cx-7, y = cy-7;
        RGBColor c = name == 0 ? RGBColor(65,145,240) : name == 1 ? RGBColor(60,220,230) : name == 2 ? RGBColor(235,60,60) : gold;
        if (name == 0) {
            Rect(x+5,y+7,6,6,c); Rect(x+3,y+9,2,3,c); Rect(x+11,y+9,2,3,c);
            Rect(x+1,y+3,3,4,c); Rect(x+5,y+1,3,4,c); Rect(x+10,y+2,3,4,c);
            Rect(x+12,y+6,2,3,c); Rect(x+7,y+8,1,1,white);
        } else if (name == 1) {
            Rect(x+1,y+1,13,10,c); Rect(x+3,y+3,9,6,black); Rect(x+4,y+4,5,1,white);
            Rect(x+4,y+6,3,1,green); Rect(x+10,y+7,1,1,white);
            Rect(x+6,y+11,3,2,c); Rect(x+3,y+13,9,2,c);
        } else if (name == 2) {
            Rect(x+4,y+4,8,9,c); Rect(x+2,y+6,12,5,c); Rect(x+5,y+2,5,2,c);
            Rect(x+8,y,2,3,green); Rect(x+10,y+1,4,2,green);
            Rect(x+4,y+6,1,1,white); Rect(x+6,y+13,4,1,c);
        } else {
            Rect(x+3,y+3,9,2,gold); Rect(x+2,y+5,11,6,gold); Rect(x,y+11,15,3,gold);
            Rect(x+4,y+6,7,3,RGBColor(245,135,40)); Rect(x+5,y+6,5,1,white); Rect(x+2,y+12,1,1,white);
        }
    }
    void Text(const char* text, int x, int y, int scale, RGBColor c) {
        // Five-bit rows of the original block font, plus menu glyphs.
        static const char letters[] = "WINLOSEGAM PCT";
        static const uint8_t glyphs[][7] = {
            {17,17,21,21,21,27,17},{31,4,4,4,4,4,31},{17,25,25,21,19,19,17},
            {16,16,16,16,16,16,31},{14,17,17,17,17,17,14},{15,16,16,14,1,1,30},
            {31,16,16,30,16,16,31},{14,17,16,23,17,17,14},{14,17,17,31,17,17,17},
            {17,27,21,21,17,17,17},{0,0,0,0,0,0,0},{30,17,17,30,16,16,16},
            {14,17,16,16,16,17,14},{31,4,4,4,4,4,4}
        };
        for (; *text; ++text, x += 6*scale) {
            const char* found = strchr(letters, *text);
            if (!found) continue;
            for (int yy=0; yy<7; ++yy) for (int xx=0; xx<5; ++xx)
                if (glyphs[found-letters][yy] & (16 >> xx)) Rect(x+xx*scale,y+yy*scale,scale,scale,c);
        }
    }
    void Spin() {
        if (spinning) return;
        spinning = true; elapsed = 0; banner = 0;
        for (int i=0; i<3; ++i) {
            speed[i] = random(12000,16001)/1000.0f+i;
            stop[i] = 0.85f+i*0.48f+random(50,161)/1000.0f;
            stopped[i] = false;
        }
    }
public:
    static Games& Instance() { static Games games; return games; }
    bool Active() const { return screen != Faces; }
    RGBColor Pixel(int x, int y) const { return pixels[y*64+x]; }
    void Update() {
        uint32_t now = millis();
        float dt = last ? (now-last)/1000.0f : 0;
        last = now;
        if (dt > 0.1f) dt = 0.1f;
        bool up = ButtonHandler::GetUpPressed();
        bool space = ButtonHandler::GetSpacePressed();
        if (up) {
            if (screen == Faces) randomSeed(micros());
            screen = screen == Faces ? Selection : screen == Slots ? Selection : Faces;
            ButtonHandler::SetGamesInputActive(Active());
        }
        if (!Active()) return;
        ButtonHandler::GetRightPressed(); ButtonHandler::GetOKPressed();
        if (space && !up) {
            if (screen == Selection) screen = Slots;
            else Spin();
        }
        Rect(0,0,64,32,black);
        if (screen == Selection) {
            Text("GAMES",17,1,1,gold); Text("SLOTS",17,12,1,white); Text("SPACE",17,24,1,green);
            return;
        }
        banner = fmaxf(0, banner-dt);
        if (spinning) {
            elapsed += dt;
            for (int i=0; i<3; ++i) if (!stopped[i]) {
                pos[i] += speed[i]*dt;
                if (elapsed >= stop[i]) {
                    speed[i] = fmaxf(1.1f,speed[i]-20*dt);
                    if (speed[i] <= 1.15f) { pos[i] = roundf(pos[i]); speed[i]=0; stopped[i]=true; }
                }
                pos[i] = fmodf(pos[i],12.0f);
            }
            if (stopped[0] && stopped[1] && stopped[2]) {
                spinning=false; banner=2;
                win = Symbol(0,lroundf(pos[0])) == Symbol(1,lroundf(pos[1])) && Symbol(1,lroundf(pos[1])) == Symbol(2,lroundf(pos[2]));
            }
        }
        for (int i=0; i<3; ++i) {
            int nearest = lroundf(pos[i]);
            for (int d=-1; d<=1; ++d) SymbolDraw(Symbol(i,nearest+d),11+i*21,int(16+d*18-(pos[i]-nearest)*18));
        }
        for (int y=0; y<32; y+=2) { Rect(21,y,1,1,RGBColor(35,35,40)); Rect(42,y,1,1,RGBColor(35,35,40)); }
        if (banner > 0) { Rect(0,7,64,18,gold); Text(win ? "WIN" : "LOSE",win ? 14 : 8,9,2,black); }
    }
};
