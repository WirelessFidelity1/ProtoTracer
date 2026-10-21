#pragma once

#include <Arduino.h>
#include <math.h>
#include "../../../Utils/RGBColor.h"

// Native pixel animations for the uncovered 51x32 panel area.
class RetroScreensavers {
    struct Flyer { float x, y, z, phase; };
    Flyer flyers[6];
    uint32_t last=0;
    float time=0;

    static RGBColor* Frame();
    void Rect(int x,int y,int w,int h,RGBColor color);
    void DrawToaster(const Flyer& flyer,int index);

public:
    void Begin();
    void Update();
    RGBColor Pixel(int x,int y) const;
};
