#pragma once
#include "../../../Utils/RGBColor.h"

class TextScroller {
    char message[129] = {};
    uint8_t length = 0;
    bool editing = true;
    float position = 64;
    uint32_t previous = 0;
    static bool Glyph(char character, int x, int y);
public:
    void Begin();
    void Update();
    RGBColor Pixel(int x, int y, bool preview = false) const;
};
