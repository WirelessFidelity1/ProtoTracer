#pragma once
#include <Arduino.h>
#include <math.h>
#include <string.h>
#include "../../../Utils/RGBColor.h"

// Small, allocation-free games for a 64x32 logical canvas.
class ArcadeGames {
    uint8_t board[15][10] = {};
    uint8_t shape = 0, rotation = 0;
    int pieceX = 3, pieceY = 0;
    float fallTime = 0;
    bool dead = false, ready = false;
    uint16_t score = 0;
    float birdY = 15, birdV = 0;
    float pipeX[3] = {};
    uint8_t gapY[3] = {};
    bool passed[3] = {};
    float dinoY = 0, dinoV = 0, distance = 0;
    float cactusX[2] = {};
    uint8_t cactusHeight[2] = {};
    RGBColor pixels[64*32];

    static bool Cell(uint8_t shape, uint8_t rotation, int x, int y) {
        static const uint16_t masks[] = {0x0F00,0x6600,0x4E00,0x8E00,0x2E00,0x6C00,0xC600};
        if (shape == 1) rotation = 0;
        for (uint8_t i=0;i<rotation;++i) { int oldX=x; x=y; y=3-oldX; }
        return masks[shape] & (0x8000u >> (y*4+x));
    }
    bool Fits(int px, int py, uint8_t turn) const {
        for (int y=0;y<4;++y) for (int x=0;x<4;++x) if (Cell(shape,turn,x,y)) {
            int bx=px+x, by=py+y;
            if (bx<0 || bx>=10 || by>=15 || (by>=0 && board[by][bx])) return false;
        }
        return true;
    }
    void Spawn() {
        shape=random(7); rotation=0; pieceX=3; pieceY=0; fallTime=0;
        if (!Fits(pieceX,pieceY,rotation)) dead=true;
    }
    void Lock() {
        for (int y=0;y<4;++y) for (int x=0;x<4;++x) if (Cell(shape,rotation,x,y)) {
            if (pieceY+y<0) { dead=true; continue; }
            board[pieceY+y][pieceX+x]=shape+1;
        }
        if (dead) return;
        uint8_t lines=0;
        for (int y=14;y>=0;--y) {
            bool full=true;
            for (int x=0;x<10;++x) if (!board[y][x]) full=false;
            if (full) {
                for (int row=y;row>0;--row) memcpy(board[row],board[row-1],10);
                memset(board[0],0,10); ++lines; ++y;
            }
        }
        static const uint16_t points[] = {0,100,300,500,800};
        score=min(9999,int(score)+points[lines]);
        Spawn();
    }
    void Rect(int x,int y,int w,int h,RGBColor color) {
        for (int row=max(0,y);row<min(32,y+h);++row)
            for (int col=max(0,x);col<min(64,x+w);++col) pixels[row*64+col]=color;
    }
    void Score(int originX=1) {
        static const uint8_t digits[10][5] = {
            {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
            {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}};
        uint16_t value=score;
        for (int column=3;column>=0;--column) {
            uint8_t digit=value%10; value/=10;
            for (int y=0;y<5;++y) for (int x=0;x<3;++x)
                if (digits[digit][y]&(4>>x)) Rect(originX+column*4+x,1+y,1,1,RGBColor(240,240,240));
        }
    }
    static RGBColor Block(uint8_t index) {
        static const uint8_t colors[7][3] = {
            {20,220,240},{240,220,40},{180,60,220},{50,100,255},
            {255,140,20},{60,220,70},{240,50,60}};
        return RGBColor(colors[index][0],colors[index][1],colors[index][2]);
    }
    void TetrisBlock(int x,int y,RGBColor color) {
        if (y<0 || y>=15) return;
        // Native panel pixels: 15 full-height rows plus a one-pixel border.
        Rect(15+x*2,1+y*2,2,2,color);
    }

public:
    void ResetDino() {
        dinoY=dinoV=distance=0; score=0; dead=false; ready=true;
        cactusX[0]=60; cactusX[1]=100;
        cactusHeight[0]=6; cactusHeight[1]=8;
    }
    FLASHMEM __attribute__((noinline)) void UpdateDino(float dt,bool jump) {
        if (dead && jump) ResetDino();
        // Reach ~13 pixels and stay above the tallest cactus long enough
        // for its full collision width to pass at the starting speed.
        if (jump && dinoY<=0) { ready=false; dinoV=52; }
        if (!dead && !ready) {
            // Small steps prevent fast obstacles skipping the dinosaur.
            for (float remaining=dt;remaining>0 && !dead;) {
                float step=fminf(remaining,0.005f); remaining-=step;
                float speed=fminf(34,18+distance/350);
                distance+=speed*step;
                score=min(9999,int(distance/5));
                dinoV-=105*step; dinoY+=dinoV*step;
                if (dinoY<0) { dinoY=0; dinoV=0; }
                for (int i=0;i<2;++i) {
                    cactusX[i]-=speed*step;
                    if (cactusX[i]<-5) {
                        // Leave enough time to land and jump again at every speed.
                        int minimumGap=max(32,int(ceilf(speed*1.25f)));
                        cactusX[i]=fmaxf(51,cactusX[1-i]+random(minimumGap,minimumGap+15));
                        cactusHeight[i]=random(5,9);
                    }
                    // Inset body bounds allow the tail and snout to overhang.
                    if (cactusX[i]<15 && cactusX[i]+4>9 && dinoY<cactusHeight[i]-1)
                        dead=true;
                }
            }
        }
        for (auto& pixel:pixels) pixel=RGBColor(0,0,0);
        const RGBColor white(225,225,225), gray(75,75,75);
        Rect(0,28,51,1,white);
        for (int i=0;i<7;++i) {
            int x=((i*11-int(distance))%77+77)%77;
            Rect(x,30+i%2,2+i%3,1,gray);
        }
        // Faint clouds drift behind the monochrome runner.
        for (int i=0;i<2;++i) {
            int x=((i*37+24-int(distance/4))%74+74)%74;
            Rect(x,10+i*3,7,1,gray); Rect(x+2,9+i*3,3,1,gray);
        }
        for (int i=0;i<2;++i) {
            int x=int(floorf(cactusX[i])), top=28-cactusHeight[i];
            Rect(x+2,top,2,cactusHeight[i],white);
            Rect(x,top+2,1,3,white); Rect(x,top+4,3,1,white);
            Rect(x+5,top+1,1,3,white); Rect(x+3,top+3,3,1,white);
        }
        static const char* dinosaur[] = {
            "       ####### ", "      ## ##### ", "      ######## ",
            "      ######## ", "      ####     ", "      ######   ",
            "#    #####     ", "#   #######    ", "## ######## #  ",
            " ##########    ", "  #########    ", "   #######     ",
            "    #####      ", "    ## ##      ", "    ##  ##     "
        };
        int top=13-lroundf(dinoY);
        for (int y=0;y<15;++y) for (int x=0;x<15;++x) {
            if (dinosaur[y][x]!='#') continue;
            // Alternate feet while running; hold the pose in the air.
            if (y>=13 && !ready && !dead && dinoY==0
                && ((int(distance/3)&1) ? x<7 : x>=7)) continue;
            Rect(3+x,top+y,1,1,white);
        }
        Score(35);
    }
    bool GameOver() const { return dead; }
    bool Ready() const { return ready; }
    const RGBColor& Pixel(int x,int y) const { return pixels[y*64+x]; }
    void ResetTetris() {
        memset(board,0,sizeof(board)); score=0; dead=ready=false; Spawn();
    }
    FLASHMEM __attribute__((noinline)) void UpdateTetris(float dt,int direction,bool rotate,bool drop,bool softDrop) {
        if (dead) { if (rotate || drop) ResetTetris(); }
        else {
            if (direction && Fits(pieceX+direction,pieceY,rotation)) pieceX+=direction;
            if (rotate) {
                uint8_t turn=(rotation+1)%4;
                static const int kicks[] = {0,-1,1,-2,2};
                for (int kick:kicks) if (Fits(pieceX+kick,pieceY,turn)) {
                    pieceX+=kick; rotation=turn; break;
                }
            }
            fallTime+=dt;
            float interval=fmaxf(0.12f,0.65f-score/10000.0f);
            if (drop) {
                while (Fits(pieceX,pieceY+1,rotation)) ++pieceY;
                Lock();
            } else if (softDrop || fallTime>=interval) {
                fallTime=0;
                if (Fits(pieceX,pieceY+1,rotation)) ++pieceY; else Lock();
            }
        }
        for (auto& pixel:pixels) pixel=RGBColor(0,0,0);
        const RGBColor border(150,170,190);
        Rect(14,0,1,32,border); Rect(35,0,1,32,border);
        Rect(14,0,22,1,border); Rect(14,31,22,1,border);
        for (int y=0;y<15;++y) for (int x=0;x<10;++x)
            if (board[y][x]) TetrisBlock(x,y,Block(board[y][x]-1));
        if (!dead) for (int y=0;y<4;++y) for (int x=0;x<4;++x)
            if (Cell(shape,rotation,x,y)) TetrisBlock(pieceX+x,pieceY+y,Block(shape));
        Score(36);
    }
    void ResetFlappy() {
        birdY=15; birdV=0; score=0; dead=false; ready=true;
        for (int i=0;i<3;++i) { pipeX[i]=70+i*36; gapY[i]=random(10,23); passed[i]=false; }
    }
    FLASHMEM __attribute__((noinline)) void UpdateFlappy(float dt,bool flap) {
        if (dead && flap) ResetFlappy();
        if (flap) { ready=false; birdV=-13; }
        if (!dead && !ready) {
            // Substeps keep collisions reliable during slower display frames.
            for (float remaining=dt;remaining>0 && !dead;) {
                float step=fminf(remaining,0.01f); remaining-=step;
                birdV+=32*step; birdY+=birdV*step;
                if (birdY<0 || birdY+6>30) dead=true;
                for (int i=0;i<3;++i) {
                    pipeX[i]-=14*step;
                    if (pipeX[i]+7<0) {
                        float farthest=fmaxf(pipeX[0],fmaxf(pipeX[1],pipeX[2]));
                        pipeX[i]=farthest+36; gapY[i]=random(10,23); passed[i]=false;
                    }
                    if (!passed[i] && pipeX[i]+7<12) { score=min(9999,int(score)+1); passed[i]=true; }
                    if (pipeX[i]<20 && pipeX[i]+7>12
                        && (birdY+1<gapY[i]-6 || birdY+5>gapY[i]+6)) dead=true;
                }
            }
        }
        for (auto& pixel:pixels) pixel=RGBColor(0,0,0);
        for (int i=0;i<3;++i) {
            int x=int(floorf(pipeX[i]));
            Rect(x,0,7,gapY[i]-6,RGBColor(30,170,55));
            Rect(x,gapY[i]+6,7,30-gapY[i]-6,RGBColor(30,170,55));
        }
        Rect(0,30,64,2,RGBColor(120,85,30));
        static const char* bird[] = {
            "  YYYY  ", " YYYWK  ", "YYYYWKOO",
            "YOYYOOOO", " YOYYY  ", "  YYY   "
        };
        int top=lroundf(birdY);
        for (int y=0;y<6;++y) for (int x=0;x<8;++x) {
            char pixel=bird[y][x];
            if (pixel==' ') continue;
            RGBColor color = pixel=='Y' ? RGBColor(255,220,40)
                : pixel=='W' ? RGBColor(255,255,255)
                : pixel=='O' ? RGBColor(245,110,20) : RGBColor(0,0,0);
            Rect(12+x,top+y,1,1,color);
        }
        // A little wing lift when flapping, lowered during descent.
        Rect(13,top+(birdV<0 ? 2 : 4),2,1,RGBColor(210,140,20));
        Score();
    }
};
