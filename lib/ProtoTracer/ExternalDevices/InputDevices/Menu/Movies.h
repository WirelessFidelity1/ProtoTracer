#pragma once
#include <Arduino.h>
#include "../../../Utils/RGBColor.h"

class Movies {
public:
    static constexpr uint8_t PackedCount = 11;
    static constexpr uint8_t Count = PackedCount + 1; // DVD and packed clips.
    static bool IsScreensaver(uint8_t) { return false; }
    static const char* Name(uint8_t selection);
    static void Start(uint8_t selection);
    static void Update();
    static RGBColor Pixel(int x, int y);
};
