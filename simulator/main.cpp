#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <fstream>
#include <sstream>
#include "Arduino.h"
#include "SmartMatrix.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_NeoPixel.h"
#include "Adafruit_APDS9960.h"
#include "USBHost_t36.h"
#include "EEPROM.h"
#include "Examples/Protogen/ProtogenHUB75Project.h"

namespace Simulator { extern float audioLevel; extern bool deterministic; extern uint32_t simulatedMicros; }
static ProtogenHUB75Project project;
static bool running=true, audioEnabled=false;
static int previousMouseY=-1;
static double frameMs=0;
static constexpr int Width=1050, Height=690;

static uint8_t HID(WPARAM key) {
    switch(key) {
        case VK_UP:return 0x52; case VK_DOWN:return 0x51;
        case VK_LEFT:return 0x50; case VK_RIGHT:return 0x4f;
        case VK_RETURN:return 0x28; case VK_SPACE:return 0x2c;
        case VK_ESCAPE:return 0x29; case 'B':return 0x05; case 'M':return 0x10;
        default:return 0;
    }
}
static void Key(WPARAM key,bool down) {
    uint8_t code=HID(key);
    if(code && down && Simulator::rawPress) Simulator::rawPress(code);
    if(code && !down && Simulator::rawRelease) Simulator::rawRelease(code);
}
static void Tick() {
    IntervalTimer::Service();
    ButtonHandler::UpdateUSB();
    Games::Instance().Update();
    project.Animate(float(millis()%5000)/5000);
    project.Render();
    project.Display();
}
static void Label(HDC dc,int x,int y,const char* text,COLORREF color=RGB(185,202,225)) {
    SetTextColor(dc,color); SetBkMode(dc,TRANSPARENT);
    TextOutA(dc,x,y,text,int(strlen(text)));
}
static void Blit(HDC dc,int x,int y,int w,int h,int scale,const uint32_t* pixels) {
    BITMAPINFO info={}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=w; info.bmiHeader.biHeight=-h;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    SetStretchBltMode(dc,COLORONCOLOR);
    StretchDIBits(dc,x,y,w*scale,h*scale,0,0,w,h,pixels,&info,DIB_RGB_COLORS,SRCCOPY);
}
static void Draw(HDC dc) {
    RECT area={0,0,Width,Height}; HBRUSH bg=CreateSolidBrush(RGB(12,17,27)); FillRect(dc,&area,bg); DeleteObject(bg);
    auto font=CreateFontA(18,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
    auto old=SelectObject(dc,font);
    Label(dc,28,18,"PROTOTRACER / WINDOWS SIMULATOR",RGB(80,225,235));
    Label(dc,28,48,"Shared firmware renderer, menus, games and packed movies. No Teensy required.");
    uint32_t panel[2048];
    for(int p=0;p<2;++p) {
        Label(dc,28+p*490,85,p==0 ? "LEFT PANEL / 64 x 32" : "RIGHT PANEL / 64 x 32");
        for(int i=0;i<2048;++i) { auto c=Simulator::panels[p*2048+i]; panel[i]=(c.red<<16)|(c.green<<8)|c.blue; }
        Blit(dc,28+p*490,112,64,32,7,panel);
    }
    Label(dc,28,350,"OLED / 128 x 64");
    uint32_t oled[8192];
    for(int i=0;i<8192;++i) oled[i]=Simulator::oled[i] ? 0xbeeaff : 0x020609;
    Blit(dc,28,378,128,64,3,oled);
    Label(dc,450,350,"ACCENT LIGHTS / 176 LEDs");
    uint32_t accent[176];
    for(int i=0;i<176;++i) { auto c=Simulator::accents[i]; accent[i]=(c.red<<16)|(c.green<<8)|c.blue; }
    Blit(dc,450,380,176,1,3,accent);
    Label(dc,450,410,"ANTENNA / 5 LEDs");
    for(int i=0;i<5;++i) {
        uint32_t c=Simulator::antenna[i]; int b=Simulator::antennaBrightness;
        RECT r={450+i*34,438,474+i*34,462};
        HBRUSH brush=CreateSolidBrush(RGB(((c>>16)&255)*b/255,((c>>8)&255)*b/255,(c&255)*b/255));
        FillRect(dc,&r,brush); DeleteObject(brush);
    }
    Label(dc,450,480,Simulator::boop ? "BOOP: pressed" : "F2 / right mouse: hold to boop",Simulator::boop ? RGB(255,190,80) : RGB(185,202,225));
    char status[100]; snprintf(status,sizeof(status),"F3: synthetic audio %s   Wheel: level %.0f%%",audioEnabled ? "ON" : "OFF",Simulator::audioLevel*100);
    Label(dc,450,510,status);
    snprintf(status,sizeof(status),"Frame %.1f ms   /   BOOP setting: %s",frameMs,Menu::UseBoopSensor() ? "ON" : "OFF"); Label(dc,450,540,status);
    Label(dc,28,593,"Up: activities / back    Left / Right: select    Enter: settings / launch    Space: action");
    Label(dc,28,620,"Mouse: Pong paddle    B: blue screen    M: Matrix face effect    Text: type normally");
    Label(dc,28,647,"Enable BOOP in settings for sensor actions. F2 works while typing. Close window to quit.");
    SelectObject(dc,old); DeleteObject(font);
}
static void Screenshot(const char* path) {
    HDC screen=GetDC(nullptr), dc=CreateCompatibleDC(screen);
    BITMAPINFO info={}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth=Width;
    info.bmiHeader.biHeight=-Height; info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32;
    void* data=nullptr; HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&data,nullptr,0);
    auto old=SelectObject(dc,bitmap); Draw(dc); GdiFlush();
    BITMAPFILEHEADER file={}; file.bfType=0x4d42; file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);
    file.bfSize=file.bfOffBits+Width*Height*4;
    std::ofstream out(path,std::ios::binary); out.write(reinterpret_cast<char*>(&file),sizeof(file));
    out.write(reinterpret_cast<char*>(&info.bmiHeader),sizeof(BITMAPINFOHEADER)); out.write(static_cast<char*>(data),Width*Height*4);
    SelectObject(dc,old); DeleteObject(bitmap); DeleteDC(dc); ReleaseDC(nullptr,screen);
}
static LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM key,LPARAM param) {
    switch(message) {
        case WM_ERASEBKGND:return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint; HDC dc=BeginPaint(window,&paint);
            HDC memory=CreateCompatibleDC(dc); HBITMAP buffer=CreateCompatibleBitmap(dc,Width,Height);
            auto old=SelectObject(memory,buffer); Draw(memory);
            BitBlt(dc,0,0,Width,Height,memory,0,0,SRCCOPY);
            SelectObject(memory,old); DeleteObject(buffer); DeleteDC(memory); EndPaint(window,&paint); return 0;
        }
        case WM_KEYDOWN:
            if(key==VK_F2) { Simulator::boop=true; return 0; }
            if(!(param & (1L<<30))) {
                if(key==VK_F3) { audioEnabled=!audioEnabled; Simulator::audioLevel=audioEnabled ? 0.65f : 0; }
                else Key(key,true);
            }
            return 0;
        case WM_KEYUP:
            if(key==VK_F2) Simulator::boop=false; else Key(key,false); return 0;
        case WM_CHAR:
            if(Simulator::textPress && key>=8 && key<=126) Simulator::textPress(int(key)); return 0;
        case WM_MOUSEMOVE: {
            int y=GET_Y_LPARAM(param);
            if(previousMouseY>=0) Simulator::mouseY+=y-previousMouseY;
            previousMouseY=y; Simulator::mouseChanged=true; return 0;
        }
        case WM_LBUTTONDOWN:
            Simulator::mouseButtons=1; Simulator::mouseChanged=true; SetCapture(window); return 0;
        case WM_LBUTTONUP:
            Simulator::mouseButtons=0; Simulator::mouseChanged=true; ReleaseCapture(); return 0;
        case WM_RBUTTONDOWN:Simulator::boop=true; SetCapture(window); return 0;
        case WM_RBUTTONUP:Simulator::boop=false; ReleaseCapture(); return 0;
        case WM_MOUSEWHEEL:
            Simulator::audioLevel=constrain(Simulator::audioLevel+GET_WHEEL_DELTA_WPARAM(key)/1200.0f,0.0f,1.0f);
            audioEnabled=Simulator::audioLevel>0; return 0;
        case WM_KILLFOCUS:
            Simulator::boop=false; Simulator::mouseButtons=0; Simulator::mouseChanged=true; previousMouseY=-1;
            if(Simulator::rawRelease) { Simulator::rawRelease(0x05); Simulator::rawRelease(0x10); } return 0;
        case WM_DESTROY:running=false; PostQuitMessage(0); return 0;
    }
    return DefWindowProcA(window,message,key,param);
}
static void Frames(int n) { for(int i=0;i<n;++i) { Simulator::simulatedMicros+=16667; Tick(); } }
static void Press(WPARAM key) { Key(key,true); Frames(2); Key(key,false); }
static int SelfTest() {
    std::ofstream report("self-test.txt");
    auto check=[&](bool ok,const char* label) { report<<(ok ? "PASS " : "FAIL ")<<label<<"\n"; report.flush(); if(!ok) throw std::runtime_error(label); };
    try {
        Frames(360);
        check(std::any_of(std::begin(Simulator::panels),std::end(Simulator::panels),[](rgb24 c){return c.red||c.green||c.blue;}),"face renderer produces pixels");
        check(std::any_of(std::begin(Simulator::oled),std::end(Simulator::oled),[](uint8_t c){return c!=0;}),"OLED renders firmware HUD");
        Screenshot("simulator-face.bmp");
        Press('M'); Frames(60);
        check(project.MatrixEnabled(),"M enables Matrix face effect");
        Screenshot("simulator-matrix.bmp");
        Key('M',true); Frames(1); Key('M',true); Frames(1);
        check(!project.MatrixEnabled(),"held M does not repeatedly toggle Matrix");
        Key('M',false);
        for(int i=0;i<9;++i) { Press(VK_RIGHT); Frames(10); }
        check(Menu::GetFaceState()==9,"Wavy face is selectable");
        Frames(120); Screenshot("simulator-wavy.bmp");
        bool wavyNeverBlinks = true;
        for(int i=0;i<600;++i) { Frames(1); wavyNeverBlinks &= project.WavyEyeIsOpen(); }
        check(wavyNeverBlinks,"Wavy eye stays open through the blink cycle");
        Press(VK_RIGHT); Frames(10);
        check(Menu::GetFaceState()==0,"face selection wraps through all ten faces");
        Press(VK_RETURN); check(Menu::GetCurrentMenu()==1,"Enter advances firmware settings");
        for(int i=0;i<12;++i) Press(VK_RETURN);
        check(Menu::GetCurrentMenu()==0,"settings navigation wraps to faces");
        Press(VK_UP); Frames(30); check(Games::Instance().Active(),"Up opens activity picker"); Screenshot("simulator-menu.bmp");
        Press(VK_RETURN); Press(VK_SPACE); Frames(90); Screenshot("simulator-slots.bmp");
        Press(VK_UP); Press(VK_RIGHT); Press(VK_RETURN);
        Simulator::mouseY=24; Simulator::mouseButtons=1; Simulator::mouseChanged=true;
        Frames(2); Simulator::mouseButtons=0; Simulator::mouseChanged=true; Frames(60);
        Screenshot("simulator-pong.bmp"); Press(VK_UP);
        Press(VK_LEFT); // return to first entry before opening Movies
        Press(VK_RIGHT); Press(VK_RIGHT); Press(VK_RETURN); Frames(20);
        for(int i=0;i<Movies::Count;++i) {
            Press(VK_RETURN); Frames(20); Press(VK_RIGHT);
        }
        check(Games::Instance().Active(),"every movie selection and playback wraps safely");
        Screenshot("simulator-movies.bmp");
        Press(VK_UP); Press(VK_UP); // movie picker -> activities
        Press(VK_RIGHT); Press(VK_RETURN); // Text
        for(char c:std::string("HELLO WINDOWS")) Simulator::textPress(c);
        Simulator::textPress(13); Frames(90); Screenshot("simulator-text.bmp");
        Press(VK_UP); Press(VK_RIGHT); Press(VK_RETURN); Frames(30); Press(VK_SPACE); Frames(30); // Tetris
        Press(VK_UP); Press(VK_RIGHT); Press(VK_RETURN); Press(VK_SPACE); Frames(90); // Flappy
        Press(VK_UP); Press(VK_RIGHT); Press(VK_RETURN); Frames(5);
        check(Games::Instance().DinoActive(),"Dino launches through shared keyboard path");
        auto dinoTop=[]() {
            for(int y=0;y<28;++y) for(int x=3;x<15;++x) {
                RGBColor c=Games::Instance().Pixel(x,y);
                if(c.R==225 && c.G==225 && c.B==225) return y;
            }
            return 28;
        };
        int standing=dinoTop();
        Simulator::boop=true; Frames(8);
        check(dinoTop()<standing,"boop starts Dino and jumps"); Screenshot("simulator-dino.bmp");
        Frames(60); check(dinoTop()==standing,"held boop does not repeat jumps");
        Simulator::boop=false; Frames(2);
        Press(VK_UP); Press(VK_RIGHT); Press(VK_RETURN); Frames(180); Screenshot("simulator-merge.bmp");
        Press('B'); check(Games::Instance().BSODActive(),"B opens blue screen"); Press('B');
        check(!Games::Instance().BSODActive(),"B closes blue screen");
        report<<"All simulator smoke checks passed.\n"; return 0;
    } catch(const std::exception& error) { report<<error.what()<<"\n"; return 1; }
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE,LPSTR args,int show) {
    bool test=strstr(args,"--self-test")!=nullptr;
    Simulator::deterministic=test;
    char executable[MAX_PATH]; GetModuleFileNameA(nullptr,executable,MAX_PATH);
    std::string settings=executable; settings=settings.substr(0,settings.find_last_of("\\/"))+"/simulator-settings.bin";
    if(!test) {
        std::ifstream saved(settings,std::ios::binary);
        uint8_t bytes[sizeof(EEPROM.data)];
        if(saved.read(reinterpret_cast<char*>(bytes),sizeof(bytes))) memcpy(EEPROM.data,bytes,sizeof(bytes));
    }
    ButtonHandler::BeginUSB(); project.Initialize();
    if(test) return SelfTest();
    timeBeginPeriod(1);
    WNDCLASSA cls={}; cls.lpfnWndProc=WindowProc; cls.hInstance=instance;
    cls.lpszClassName="ProtoTracerSimulator"; cls.hCursor=LoadCursor(nullptr,IDC_ARROW); RegisterClassA(&cls);
    RECT rect={0,0,Width,Height}; DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
    AdjustWindowRect(&rect,style,FALSE);
    HWND window=CreateWindowA(cls.lpszClassName,"ProtoTracer Simulator",style,CW_USEDEFAULT,CW_USEDEFAULT,
        rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,instance,nullptr);
    if(!window) return 1;
    ShowWindow(window,show);
    auto next=std::chrono::steady_clock::now();
    while(running) {
        MSG message;
        while(PeekMessage(&message,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&message); DispatchMessage(&message); }
        if(!running) break;
        auto start=std::chrono::steady_clock::now();
        Tick();
        frameMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        InvalidateRect(window,nullptr,FALSE);
        next+=std::chrono::microseconds(16667);
        if(next<start) next=std::chrono::steady_clock::now();
        std::this_thread::sleep_until(next);
    }
    std::ofstream saved(settings,std::ios::binary); saved.write(reinterpret_cast<char*>(EEPROM.data),sizeof(EEPROM.data));
    timeEndPeriod(1); return 0;
}
