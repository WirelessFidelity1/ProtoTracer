// Keep the large face update routine in flash to leave RAM1 for the stack.
#include <Arduino.h>
#if defined(PROJECT_PROTOGEN_HUB75)
#include "../Protogen/ProtogenHUB75Project.h"

FLASHMEM __attribute__((noinline)) void ProtogenHUB75Project::Update(float ratio) {
    uint8_t mode = Menu::GetFaceState();//change by button press

    controller.SetBrightness(Menu::GetBrightness());
    controller.SetAccentBrightness(Menu::GetAccentBrightness());

    if (Games::Instance().FastCanvasActive()) {
        Menu::Update(ratio);
        Games::Instance().UpdateBoop(UpdateBoopSensor());
        UpdateAntenna();
        UpdateDVD(false);
        return;
    }

    if (IsDVDVisible()) {
        Menu::Update(ratio);
        UpdateAntenna();
        UpdateDVD(true);
        pM.GetObject()->Disable();
        deltaDisplayBackground.GetObject()->Disable();
        return;
    }

    pM.Reset();
    wavySelected = false;

#ifdef MORSEBUTTON
    SelectFaceFromMorse(mode);
#else
    SelectFace(mode);
#endif

    UpdateFace(ratio);
    // The reference arrow is a fixed eye; suppress the shared blink track.
    if (wavySelected) pM.SetMorphWeight(NukudeFace::Blink, 0.0f);
    Games::Instance().UpdateBoop(Menu::UseBoopSensor() && IsBooped());

    bool showDVD = IsDVDVisible();
    UpdateDVD(showDVD);
    if (showDVD) {
        pM.GetObject()->Disable();
        deltaDisplayBackground.GetObject()->Disable();
    } else {
        pM.GetObject()->Enable();
        deltaDisplayBackground.GetObject()->Enable();
    }

    // Fit the neutral mesh separately so Wavy's different eye/mouth bounds
    // cannot stretch or skew the base nose during automatic plane fitting.
    static const uint8_t noseIndexes[8] = {0, 22, 23, 24, 25, 26, 27, 28};
    static Vector3D baseNose[8];
    if (wavySelected) {
        float wavyWeight = *pM.GetMorphWeightReference(NukudeFace::Wavy);
        pM.SetMorphWeight(NukudeFace::Wavy, 0.0f);
        pM.Update();
        AlignObjectFace(pM.GetObject(), -7.5f);
        Vector3D* neutral = pM.GetObject()->GetTriangleGroup()->GetVertices();
        for (int i = 0; i < 8; ++i) baseNose[i] = neutral[noseIndexes[i]];
        pM.SetMorphWeight(NukudeFace::Wavy, wavyWeight);
    }

    pM.Update();

    AlignObjectFace(pM.GetObject(), -7.5f);

    if (wavySelected) {
        // Keep the chevron balanced in panel space after automatic plane fitting.
        Vector3D* v = pM.GetObject()->GetTriangleGroup()->GetVertices();
        for (int i = 0; i < 8; ++i) {
            v[noseIndexes[i]].X = baseNose[i].X;
            v[noseIndexes[i]].Y = baseNose[i].Y;
        }
        float outerX = (v[1].X + v[3].X) * 0.5f;
        float innerX = (v[4].X + v[6].X) * 0.5f;
        float centerY = (v[1].Y + v[3].Y) * 0.5f;
        v[1].X = v[3].X = outerX;
        v[4].X = v[6].X = innerX;
        v[2].Y = v[5].Y = centerY;
        v[6].Y = v[1].Y;
        v[4].Y = v[3].Y;
        // A slightly larger, lower eye while retaining the balanced chevron.
        float centerX = (outerX + v[2].X) * 0.5f;
        // Expand both edges equally about each stroke's centerline.
        for (int i = 1; i <= 3; ++i) {
            int opposite = 7 - i;
            float halfThickness = (v[i].X - v[opposite].X) * 0.5f;
            v[i].X += halfThickness;
            v[opposite].X -= halfThickness;
        }
        for (int i = 1; i <= 6; ++i) {
            v[i].X = centerX - (v[i].X - centerX) * 1.15f;
            v[i].Y = centerY + (v[i].Y - centerY) * 1.15f - 6.0f;
        }
    }

    pM.GetObject()->GetTransform()->SetPosition(GetWiggleOffset());
    pM.GetObject()->UpdateTransform();
}
#endif
