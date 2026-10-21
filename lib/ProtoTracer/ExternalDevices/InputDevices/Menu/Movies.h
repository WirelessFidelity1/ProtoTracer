#pragma once
#include <Arduino.h>
#include "../../../Utils/RGBColor.h"

class Movies {
public:
    static constexpr uint8_t PackedCount = 4;
    static constexpr uint8_t Count = PackedCount + 2; // DVD, packed clips, Toasters.
    static bool IsScreensaver(uint8_t selection) { return selection> PackedCount && selection<Count; }
    static const char* Name(uint8_t selection);
    static void Start(uint8_t selection);
    static void Update();
    static RGBColor Pixel(int x, int y);
};
