#include "Movies.h"
#include "../../../Assets/Textures/Animated/Utils/MoviesData.h"

static_assert(sizeof(MoviesData::clips) / sizeof(MoviesData::clips[0]) + 1 == Movies::Count,
    "Regenerate MoviesData.h when changing the movie list");

namespace {
    uint8_t selected = 0;
    uint8_t framePixels[51 * 32];
    uint32_t started = 0;
    uint16_t previousFrame = 0xffff;
}

const char* Movies::Name(uint8_t selection) {
    return selection == 0 ? "DVD" : MoviesData::clips[(selection-1) % (Count-1)].name;
}

void Movies::Start(uint8_t selection) {
    selected = selection % Count;
    started = millis();
    previousFrame = 0xffff;
    Update();
}

void Movies::Update() {
    if (!selected) return;
    const auto& clip = MoviesData::clips[selected-1];
    uint16_t frame = ((millis()-started) / 42u) % clip.frames;
    if (frame == previousFrame) return;
    previousFrame = frame;
    const uint8_t* data = clip.data + clip.offsets[frame];
    uint16_t out = 0;
    while (out < sizeof(framePixels)) {
        uint8_t token = *data++;
        uint16_t count = token < 128 ? token+1 : token-127;
        if (token < 128) {
            while (count--) framePixels[out++] = *data++;
        } else {
            uint8_t value = *data++;
            while (count--) framePixels[out++] = value;
        }
    }
}

RGBColor Movies::Pixel(int x, int y) {
    if (!selected || x < 0 || x >= 51 || y < 0 || y >= 32) return RGBColor(0,0,0);
    const uint8_t* color = MoviesData::clips[selected-1].palette + framePixels[y*51+x]*3;
    return RGBColor(color[0], color[1], color[2]);
}
