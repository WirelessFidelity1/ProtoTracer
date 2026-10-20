#include "TextScroller.h"
#include "../ButtonHandler.h"
#include <glcdfont.c>
#include <string.h>
#include <math.h>

bool TextScroller::Glyph(char character, int x, int y) {
    return x >= 0 && x < 5 && y >= 0 && y < 7
        && (pgm_read_byte(font + uint8_t(character)*5 + x) & (1 << y));
}

void TextScroller::Begin() {
    editing = true;
    previous = millis();
    ButtonHandler::SetTextInputActive(true);
}

void TextScroller::Update() {
    uint32_t now = millis();
    float dt = fminf((now-previous)/1000.0f, 0.1f);
    previous = now;
    for (int key = ButtonHandler::ReadTextKey(); key >= 0; key = ButtonHandler::ReadTextKey()) {
        if (key == 13 || key == 10) {
            if (length) {
                editing = !editing;
                if (!editing) position = 64;
            }
        } else if (key == 8 || key == 127) {
            editing = true;
            if (length) message[--length] = 0;
        } else if (key >= 32 && key <= 126) {
            editing = true;
            if (length < sizeof(message)-1) {
                message[length++] = char(key);
                message[length] = 0;
            }
        }
    }
    if (!editing) {
        position -= dt*24;
        if (position + length*12 < 0) position = 64;
    }
}

RGBColor TextScroller::Pixel(int x, int y, bool preview) const {
    const RGBColor ink(240,250,250), black(0,0,0);
    if (x < 0 || x >= 64 || y < 0 || y >= 32) return black;
    if (!editing) {
        int localX = x-int(floorf(position));
        int row = (y-9)/2;
        if (y < 9 || y >= 23 || localX < 0 || localX/12 >= length) return black;
        return Glyph(message[localX/12],(localX%12)/2,row) ? ink : black;
    }
    // The editor is private to the OLED; visor panels stay blank until playback.
    if (!preview) return black;
    // Keep the end of the message and caret visible at 1:1 on the OLED.
    if (y >= 11 && y < 18) {
        int start = length > 9 ? length-9 : 0;
        int index = x/6 + start;
        if (index < length && Glyph(message[index],x%6,y-11)) return ink;
        if (index == length && x%6 == 0 && millis()%1000 < 500) return ink;
        return black;
    }
    const char* label = y < 7 ? "TYPE TEXT" : "ENTER:GO";
    int row = y < 7 ? y : y-25;
    int column = x/6;
    return column < int(strlen(label)) && Glyph(label[column],x%6,row)
        ? RGBColor(70,200,230) : black;
}
