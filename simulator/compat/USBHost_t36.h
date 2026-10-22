#pragma once
#include "Arduino.h"
namespace Simulator {
inline void (*rawPress)(uint8_t)=nullptr, (*rawRelease)(uint8_t)=nullptr;
inline void (*textPress)(int)=nullptr;
inline int mouseX=0,mouseY=0;
inline uint8_t mouseButtons=0;
inline bool mouseChanged=false;
}
struct USBHost { void begin() {} void Task() {} };
struct USBHub { USBHub(USBHost&) {} };
struct USBHIDParser { USBHIDParser(USBHost&) {} };
struct USBHIDInput { explicit operator bool() const { return true; } };
struct KeyboardController {
    KeyboardController(USBHost&) {}
    void attachPress(void(*fn)(int)) { Simulator::textPress=fn; }
    void attachRawPress(void(*fn)(uint8_t)) { Simulator::rawPress=fn; }
    void attachRawRelease(void(*fn)(uint8_t)) { Simulator::rawRelease=fn; }
};
struct MouseController:USBHIDInput {
    MouseController(USBHost&) {}
    bool available() { return Simulator::mouseChanged; }
    int getMouseX() { return Simulator::mouseX; }
    int getMouseY() { return Simulator::mouseY; }
    uint8_t getButtons() { return Simulator::mouseButtons; }
    void mouseDataClear() { Simulator::mouseChanged=false; Simulator::mouseX=Simulator::mouseY=0; }
};
