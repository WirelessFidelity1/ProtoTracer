#pragma once

#include <Arduino.h>
#include <math.h>
#include <string.h>
#include "../ButtonHandler.h"
#include "Movies.h"
#include "TextScroller.h"
#include "ArcadeGames.h"
#include "../../../Utils/RGBColor.h"

// Games use a logical 64x32 canvas fitted into the uncovered panel area.
// No allocation or blocking delays during animation.
class Games {
    static constexpr int VisibleWidth = 51; // 13 of 64 columns (~20%) stay black.

    enum Screen { Faces, Selection, Slots, Pong, MovieSelection, MoviePlayback, ScrollingText, Tetris, Flappy, Dino };
    TextScroller textScroller;
    ArcadeGames arcade;
    bool bsod = false;
    bool previousBoop = false;
    uint8_t selectedGame = 0;
    uint8_t selectedMovie = 0;
    float paddleY = 16, aiY = 16;
    float ballX = 31, ballY = 16, ballVX = 24, ballVY = 9;
    uint8_t playerScore = 0, aiScore = 0, previousMouseButtons = 0;
    bool serving = true;
    float serveDelay = 1.0f;

    void Serve(bool towardPlayer) {
        ballX = 31; ballY = 16;
        ballVX = towardPlayer ? -24 : 24;
        ballVY = random(0, 2) ? 9 : -9;
        serving = true;
        serveDelay = 1.0f;
    }
    void ResetPong() {
        paddleY = aiY = 16;
        playerScore = aiScore = 0;
        Serve(false);
    }
    void ScoreDigit(uint8_t digit, int x, RGBColor color) {
        static const uint8_t rows[8][5] = {
            {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7},
            {5,5,7,1,1}, {7,4,7,1,7}, {7,4,7,5,7}, {7,1,1,1,1}
        };
        for (int y=0; y<5; ++y) for (int bit=0; bit<3; ++bit)
            if (rows[digit][y] & (4 >> bit)) Rect(x+bit*2,y+1,2,1,color);
    }
    void UpdatePong(float dt, int mouseY, bool connected, bool launch) {
        paddleY = fmaxf(9, fminf(28, paddleY + mouseY * 0.25f));
        bool finished = playerScore == 7 || aiScore == 7;
        if (launch && connected) {
            if (finished) { ResetPong(); finished = false; }
            serving = false;
        }
        if (connected && serving && !finished) {
            serveDelay = fmaxf(0, serveDelay-dt);
            if (serveDelay <= 0) serving = false;
        }
        // Freeze the rally when the mouse is unplugged.
        if (connected && !serving && !finished) {
            // Small steps keep the ball from skipping a paddle on slow frames.
            for (float remaining = dt; remaining > 0;) {
                float step = fminf(remaining, 0.005f);
                remaining -= step;
                float target = ballVX > 0 ? ballY : 18.5f;
                aiY += fmaxf(-17*step, fminf(17*step, target-aiY));
                aiY = fmaxf(9, fminf(28, aiY));
                float oldX = ballX;
                ballX += ballVX*step; ballY += ballVY*step;
                if (ballY < 6) { ballY = 12-ballY; ballVY = fabsf(ballVY); }
                if (ballY > 31) { ballY = 62-ballY; ballVY = -fabsf(ballVY); }
                if (ballVX < 0 && oldX >= 4 && ballX < 4 && fabsf(ballY-paddleY) <= 4) {
                    ballX = 4;
                    ballVX = fminf(46, fabsf(ballVX)+2);
                    ballVY = (ballY-paddleY)*5;
                } else if (ballVX > 0 && oldX <= 59 && ballX > 59 && fabsf(ballY-aiY) <= 4) {
                    ballX = 59;
                    ballVX = -fminf(46, fabsf(ballVX)+2);
                    ballVY = (ballY-aiY)*5;
                }
                if (ballX < 0 || ballX > 63) {
                    bool missed = ballX < 0;
                    if (missed) ++aiScore; else ++playerScore;
                    Serve(missed);
                    break;
                }
            }
        }
        for (int y=7; y<32; y+=3) Rect(31,y,1,2,RGBColor(30,35,40));
        Rect(0,6,64,1,RGBColor(30,35,40));
        ScoreDigit(playerScore,20,green); ScoreDigit(aiScore,38,gold);
        Rect(2,lroundf(paddleY)-3,2,7,green);
        Rect(60,lroundf(aiY)-3,2,7,gold);
        Rect(lroundf(ballX),lroundf(ballY),1,1,white);
        if (!connected) {
            Rect(7,11,49,9,black); Text("NO MOUSE",8,12,1,gold);
        } else if (playerScore == 7 || aiScore == 7) {
            Rect(7,11,49,9,black);
            Text(playerScore == 7 ? "WIN" : "LOSE",playerScore == 7 ? 23 : 20,12,1,white);
        } else if (serving) {
            Text("PONG",20,13,1,white);
        }
    }
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
                if (xx >= 0 && xx < 64 && yy >= 0 && yy < 32)
                    pixels[yy*64 + xx*(bsod ? 64 : VisibleWidth)/64] = c;
    }
    void SymbolDraw(int name, int cx, int cy) {
        int x = cx-7, y = cy-7;
        const RGBColor red(235,45,55);
        if (name == 0) {
            // Classic BAR plaque with compact 3x5 lettering.
            Rect(x,y+3,15,9,white); Rect(x+1,y+4,13,7,black);
            static const uint8_t bar[3][5] = {
                {6,5,6,5,6}, {2,5,7,5,5}, {6,5,6,5,5}
            };
            for (int letter=0; letter<3; ++letter)
                for (int row=0; row<5; ++row) for (int col=0; col<3; ++col)
                    if (bar[letter][row] & (4 >> col))
                        Rect(x+2+letter*4+col,y+5+row,1,1,white);
        } else if (name == 1) {
            // Gold bell, flared rim, and clapper.
            Rect(x+6,y+1,3,2,gold); Rect(x+4,y+3,7,2,gold);
            Rect(x+3,y+5,9,5,gold); Rect(x+2,y+10,11,2,gold);
            Rect(x+1,y+12,13,1,gold); Rect(x+6,y+13,3,2,gold);
            Rect(x+4,y+5,1,5,white);
        } else if (name == 2) {
            // Two red cherries hanging from joined green stems.
            Rect(x+7,y+1,2,2,green); Rect(x+5,y+3,2,3,green);
            Rect(x+3,y+6,2,3,green); Rect(x+9,y+3,1,5,green);
            Rect(x+9,y+1,4,2,green);
            Rect(x+1,y+9,6,4,red); Rect(x+2,y+8,4,6,red);
            Rect(x+8,y+9,6,4,red); Rect(x+9,y+8,4,6,red);
            Rect(x+2,y+9,1,2,white); Rect(x+9,y+9,1,2,white);
        } else {
            // Lucky red seven.
            Rect(x+2,y+1,12,3,red); Rect(x+2,y+4,3,2,red);
            Rect(x+10,y+4,3,2,red); Rect(x+9,y+6,3,2,red);
            Rect(x+7,y+8,3,2,red); Rect(x+6,y+10,3,4,red);
            Rect(x+3,y+1,10,1,white);
        }
    }

    void Text(const char* text, int x, int y, int scale, RGBColor c) {
        // Five-bit rows of the original block font, plus menu glyphs.
        static const char letters[] = "WINLOSEGAM PCTURHDVBFXY2K31";
        static const uint8_t glyphs[][7] = {
            {17,17,21,21,21,27,17},{31,4,4,4,4,4,31},{17,25,25,21,19,19,17},
            {16,16,16,16,16,16,31},{14,17,17,17,17,17,14},{15,16,16,14,1,1,30},
            {31,16,16,30,16,16,31},{14,17,16,23,17,17,14},{14,17,17,31,17,17,17},
            {17,27,21,21,17,17,17},{0,0,0,0,0,0,0},{30,17,17,30,16,16,16},
            {14,17,16,16,16,17,14},{31,4,4,4,4,4,4},
            {17,17,17,17,17,17,14},{30,17,17,30,20,18,17},{17,17,17,31,17,17,17},
            {30,17,17,17,17,17,30},{17,17,17,17,17,10,4},{30,17,17,30,17,17,30},
            {31,16,16,30,16,16,16},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},
            {14,17,1,2,4,8,31},{17,18,20,24,20,18,17},{30,1,1,14,1,1,30},{4,12,4,4,4,4,14}
        };
        for (; *text; ++text, x += 6*scale) {
            const char* found = strchr(letters, *text);
            if (!found) continue;
            for (int yy=0; yy<7; ++yy) for (int xx=0; xx<5; ++xx)
                if (glyphs[found-letters][yy] & (16 >> xx)) Rect(x+xx*scale,y+yy*scale,scale,scale,c);
        }
    }
    void BSODText(const char* text, int y) {
        // Compact fixed-width type for the blue-screen message.
        static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .:_*";
        static const uint8_t glyphs[][5] = {
            {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},
            {7,4,6,4,7},{7,4,6,4,4},{3,4,5,5,3},{5,5,7,5,5},
            {7,2,2,2,7},{1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},
            {5,7,7,5,5},{5,7,7,7,5},{2,5,5,5,2},{6,5,6,4,4},
            {2,5,5,3,1},{6,5,6,5,5},{3,4,2,1,6},{7,2,2,2,2},
            {5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},
            {5,5,2,2,2},{7,1,2,4,7},
            {7,5,5,5,7},{2,6,2,2,7},{6,1,2,4,7},{6,1,2,1,6},
            {5,5,7,1,1},{7,4,6,1,6},{3,4,6,5,2},{7,1,2,2,2},
            {2,5,2,5,2},{2,5,3,1,6},
            {0,0,0,0,0},{0,0,0,0,2},{0,2,0,2,0},{0,0,0,0,7},{0,5,2,5,0}
        };
        for (int x = 1; *text; ++text, x += 4) {
            const char* found = strchr(letters, *text);
            if (!found) continue;
            for (int row = 0; row < 5; ++row) for (int col = 0; col < 3; ++col)
                if (glyphs[found-letters][row] & (4 >> col))
                    Rect(x+col,y+row,1,1,RGBColor(255,255,255));
        }
    }

    void Spin() {
        if (spinning) return;
        spinning = true; elapsed = 0; banner = 0;
        for (int i=0; i<3; ++i) {
            speed[i] = random(12000,16001)/1000.0f+i;
            stop[i] = 2.5f+i*0.75f+random(50,161)/1000.0f;
            stopped[i] = false;
        }
    }
public:
    void UpdateBoop(bool booped) {
        bool pressed = booped && !previousBoop;
        previousBoop = booped;
        if (pressed && !bsod && screen == Slots) Spin();
    }
    static Games& Instance() { static Games games; return games; }
    bool Active() const { return bsod || screen != Faces; }
    bool BSODActive() const { return bsod; }
    bool UpMenuActive() const { return screen != Faces; }
    RGBColor PreviewPixel(int x, int y) const {
        if (!bsod && screen == ScrollingText) return textScroller.Pixel(x,y,true);
        RGBColor color = Pixel(x, y);
        // Keep the white joke text legible on the monochrome preview.
        if (bsod && color.R < 128) return black;
        return color;
    }
    bool DVDActive() const { return !bsod && screen == MoviePlayback && selectedMovie == 0; }
    RGBColor Pixel(int x, int y, bool rearOnLeft = false) const {
        // The blue screen fills both physical panels, including the game margin.
        if (bsod) return x >= 0 && x < 64 && y >= 0 && y < 32 ? pixels[y*64+x] : black;
        // Text fills each panel independently, with readable glyphs on both.
        if (!bsod && screen == ScrollingText) return textScroller.Pixel(x,y);
        // Translate the same game to the front of either physical panel.
        int localX = x - (rearOnLeft ? 64-VisibleWidth : 0);
        if (localX < 0 || localX >= VisibleWidth) return black;
        if (!bsod && screen == MoviePlayback && selectedMovie != 0) return Movies::Pixel(localX, y);
        return pixels[y*64+localX];
    }
    FLASHMEM __attribute__((noinline)) void Update() {
        uint32_t now = millis();
        float dt = last ? (now-last)/1000.0f : 0;
        last = now;
        if (dt > 0.1f) dt = 0.1f;
        bool up = ButtonHandler::GetUpPressed();
        bool space = ButtonHandler::GetSpacePressed();
        bool down = ButtonHandler::GetDownPressed();
        int mouseX, mouseY;
        uint8_t mouseButtons;
        bool mouseConnected = ButtonHandler::ReadMouse(mouseX, mouseY, mouseButtons);
        bool mouseClick = (mouseButtons & 1) && !(previousMouseButtons & 1);
        previousMouseButtons = mouseButtons;
        bool toggleBSOD = ButtonHandler::GetBPressed();
        if (toggleBSOD) {
            bsod = !bsod;
            ButtonHandler::SetGamesInputActive(Active());
            up = space = false;
        }
        if (bsod) {
            ButtonHandler::GetRightPressed();
            ButtonHandler::GetLeftPressed();
            ButtonHandler::GetOKPressed();
            Rect(0,0,64,32,RGBColor(0,120,215));
            // Large :( face, like the Windows 8 crash screen.
            Rect(3,2,2,2,white);
            Rect(3,8,2,2,white);
            Rect(11,1,2,2,white);
            Rect(9,3,2,2,white);
            Rect(8,5,2,4,white);
            Rect(9,9,2,2,white);
            Rect(11,11,2,2,white);
            BSODText("YOUR PC RAN INTO",15);
            BSODText("A BOOP PROBLEM.",21);
            BSODText("B: RESTART",27);
            return;
        }
        if (up) {
            if (screen == ScrollingText) ButtonHandler::SetTextInputActive(false);
            if (screen == Faces) randomSeed(micros());
            screen = screen == Faces ? Selection : screen == MoviePlayback ? MovieSelection
                : (screen == Slots || screen == Pong || screen == MovieSelection || screen == ScrollingText || screen == Tetris || screen == Flappy || screen == Dino) ? Selection : Faces;
            ButtonHandler::SetGamesInputActive(Active());
        }
        if (!Active()) return;
        bool right = ButtonHandler::GetRightPressed();
        bool left = ButtonHandler::GetLeftPressed();
        int direction = int(right) - int(left);
        bool enter = ButtonHandler::GetOKPressed();
        if (screen == Selection && direction && !up) selectedGame = (selectedGame + 7 + direction) % 7;
        if (direction && !up && (screen == MovieSelection || screen == MoviePlayback)) {
            selectedMovie = (selectedMovie + Movies::Count + direction) % Movies::Count;
            Movies::Start(selectedMovie);
        }
        if (screen == MovieSelection && (enter || space) && !up) {
            screen = MoviePlayback;
            Movies::Start(selectedMovie);
        }
        if (screen == MoviePlayback) { Movies::Update(); return; }
        if ((space || (enter && screen == Selection)) && !up) {
            if (screen == Selection) {
                static const Screen activities[] = {Slots,Pong,MovieSelection,ScrollingText,Tetris,Flappy,Dino};
                screen = activities[selectedGame];
                if (screen == Pong) ResetPong();
                if (screen == ScrollingText) textScroller.Begin();
                if (screen == Tetris) arcade.ResetTetris();
                if (screen == Flappy) arcade.ResetFlappy();
                if (screen == Dino) arcade.ResetDino();
                // The launch key must not rotate/drop/flap on the first game frame.
                if (screen == Tetris || screen == Flappy || screen == Dino) { enter=space=down=false; direction=0; }
            } else if (screen == Slots && space) Spin();

        }
        if (screen == ScrollingText) { textScroller.Update(); return; }
        Rect(0,0,64,32,black);
        if (screen == Selection) {
            static const char* names[] = {"SLOTS","PONG","MOVIES","TEXT","TETRIS","FLAPPY","DINO"};
            uint8_t first=selectedGame/4*4;
            for (uint8_t row=0;row<4 && first+row<7;++row)
                Text(names[first+row],11,row*8,1,selectedGame==first+row ? white : RGBColor(55,60,65));
            Rect(3,2+(selectedGame%4)*8,4,3,green);
            Rect(60,first ? 24 : 0,2,6,gold);
            return;
        }
        if (screen == MovieSelection) {
            Text("MOVIES",14,0,1,gold);
            const char* name = Movies::Name(selectedMovie);
            Text(name,(64-int(strlen(name))*6)/2,12,1,white);
            Text("PLAY",20,24,1,green);
            return;
        }
        if (screen == Tetris || screen == Flappy || screen == Dino) {
            if (screen == Tetris) arcade.UpdateTetris(dt,up ? 0 : direction,!up && enter,!up && space,!up && down);
            else if (screen == Dino) arcade.UpdateDino(dt,!up && (space || enter || mouseClick));
            else arcade.UpdateFlappy(dt,!up && (space || enter || mouseClick));
            if (screen == Tetris || screen == Dino) {
                // Already laid out in the 51-column visible area; do not rescale.
                for (int y=0;y<32;++y) for (int x=0;x<VisibleWidth;++x)
                    pixels[y*64+x]=arcade.Pixel(x,y);
            } else {
                for (int y=0;y<32;++y) for (int x=0;x<64;++x) Rect(x,y,1,1,arcade.Pixel(x,y));
            }
            if (arcade.GameOver() || (screen == Flappy && arcade.Ready())) {
                Rect(0,9,64,16,black);
                Text(arcade.GameOver() ? "LOSE" : "FLAPPY",arcade.GameOver() ? 20 : 14,9,1,white);
                Text("SPACE",17,18,1,gold);
            }
            return;
        }
        if (screen == Pong) {
            UpdatePong(dt, mouseY, mouseConnected, !up && (space || mouseClick));
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
        // Blink every quarter-second, starting visible when the reels stop.
        if (banner > 0 && uint8_t((2.0f-banner)*4.0f)%2 == 0) {
            Rect(0,7,64,18,gold);
            Text(win ? "WIN" : "LOSE",win ? 14 : 8,9,2,black);
        }
    }
};
