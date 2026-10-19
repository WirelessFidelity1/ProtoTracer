#pragma once
#include <Arduino.h>
#include "../../../Utils/RGBColor.h"

class Movies {
public:
    static constexpr uint8_t Count = 5; // DVD plus PS2, Rick, Gamecube, and VaporWave.
    static const char* Name(uint8_t selection);
    static void Start(uint8_t selection);
    static void Update();
    static RGBColor Pixel(int x, int y);
};
