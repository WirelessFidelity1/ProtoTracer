#pragma once

#include "../Templates/ProtogenProjectTemplate.h"
#include "../../Assets/Models/OBJ/DeltaDisplayBackground.h"
#include "../../Assets/Models/FBX/NukudeFlat.h"

#include "../../Camera/CameraManager/Implementations/HUB75DeltaCameras.h"
#include "../../Controller/HUB75Controller.h"
#include "../../ExternalDevices/InputDevices/Menu/Games.h"
#include "../../Assets/Models/OBJ/DVD.h"
#include "../../Renderer/Utils/Triangle2D.h"

class ProtogenHUB75Project : public ProtogenProject {
private:
    HUB75DeltaCameraManager cameras;
    HUB75Controller controller = HUB75Controller(&cameras, 50, 50);
    NukudeFace pM;
    DeltaDisplayBackground deltaDisplayBackground;
    DVD dvd;
    bool wavySelected = false;
    bool matrixEnabled = false;
    void ApplyMatrix();
    SimpleMaterial dvdMaterial = SimpleMaterial(RGBColor(30, 220, 255));
    Vector2D dvdPosition = Vector2D(21, 15);
    Vector2D dvdVelocity = Vector2D(27, 18);
    uint32_t dvdPreviousTime = 0;
    bool dvdWasVisible = false;
    uint8_t dvdColor = 0;

    void UpdateDVD(bool visible) {
        uint32_t now = micros();
        float dt = visible && dvdWasVisible ? fminf((now - dvdPreviousTime) / 1000000.0f, 0.1f) : 0;
        dvdPreviousTime = now;
        dvdWasVisible = visible;
        if (!visible) { dvd.GetObject()->Disable(); return; }
        dvd.GetObject()->Enable();
        dvdPosition = dvdPosition + dvdVelocity * dt;
        // Existing mesh bounds: X 0..96.4033, Y 22.86..68.0156.
        const float scale = 0.75f;
        const float maxX = 189.0f - 96.4033f * scale;
        const float maxY = 93.0f - (68.0156f - 22.86f) * scale;
        bool bounced = false;
        if (dvdPosition.X < 0) { dvdPosition.X = -dvdPosition.X; dvdVelocity.X = fabsf(dvdVelocity.X); bounced = true; }
        else if (dvdPosition.X > maxX) { dvdPosition.X = 2 * maxX - dvdPosition.X; dvdVelocity.X = -fabsf(dvdVelocity.X); bounced = true; }
        if (dvdPosition.Y < 0) { dvdPosition.Y = -dvdPosition.Y; dvdVelocity.Y = fabsf(dvdVelocity.Y); bounced = true; }
        else if (dvdPosition.Y > maxY) { dvdPosition.Y = 2 * maxY - dvdPosition.Y; dvdVelocity.Y = -fabsf(dvdVelocity.Y); bounced = true; }
        if (bounced) {
            dvdColor = (dvdColor + 1) % 6;
            dvdMaterial.SetRGB(RGBColor(30, 220, 255));
            dvdMaterial.HueShift(dvdColor * 60.0f);
        }
        dvd.GetObject()->ResetVertices();
        dvd.GetObject()->GetTransform()->SetScale(Vector3D(scale, scale, 1));
        dvd.GetObject()->GetTransform()->SetPosition(Vector3D(dvdPosition.X, dvdPosition.Y - 22.86f * scale, 0));
        dvd.GetObject()->UpdateTransform();
    }

    void RenderDVD() {
        // Rasterize the existing mesh only, with four coverage samples per LED.
        // The normal face scene, effects, FFT and side-camera rendering are idle.
        ITriangleGroup* group = dvd.GetObject()->GetTriangleGroup();
        Triangle2D triangles[41];
        for (uint16_t t = 0; t < 41; ++t) {
            Triangle3D& mesh = group->GetTriangles()[t];
            triangles[t] = Triangle2D(Vector2D(mesh.p1->X, mesh.p1->Y),
                Vector2D(mesh.p2->X, mesh.p2->Y), Vector2D(mesh.p3->X, mesh.p3->Y));
        }
        RGBColor color = dvdMaterial.GetRGB(Vector3D(), Vector3D(), Vector3D());
        for (uint8_t c = 0; c < 3; ++c) {
            IPixelGroup* pixels = cameras.GetCameras()[c]->GetPixelGroup();
            for (uint16_t i = 0; i < pixels->GetPixelCount(); ++i) *pixels->GetColor(i) = RGBColor(0,0,0);
        }
        IPixelGroup* pixels = cameras.GetMainPixelGroup();
        int left = max(0, int(floorf(dvdPosition.X / 3.0f)) - 1);
        int right = min(63, int(ceilf((dvdPosition.X + 96.4033f * 0.75f) / 3.0f)) + 1);
        int bottom = max(0, int(floorf(dvdPosition.Y / 3.0f)) - 1);
        int top = min(31, int(ceilf((dvdPosition.Y + 45.1556f * 0.75f) / 3.0f)) + 1);
        for (int y = bottom; y <= top; ++y) for (int x = left; x <= right; ++x) {
            uint8_t coverage = 0;
            for (int sy = 0; sy < 2; ++sy) for (int sx = 0; sx < 2; ++sx) {
                float px = x * 3.0f + (sx ? 0.75f : -0.75f);
                float py = y * 3.0f + (sy ? 0.75f : -0.75f);
                for (uint16_t t = 0; t < 41; ++t) {
                    float u, v, w;
                    if (triangles[t].DidIntersect(px, py, u, v, w)) { ++coverage; break; }
                }
            }
            *pixels->GetColor(y * 64 + x) = RGBColor(color.R * coverage / 4,
                color.G * coverage / 4, color.B * coverage / 4);
        }
    }
    
	const __FlashStringHelper* faceArray[10] = {F("DEFAULT"), F("ANGRY"), F("DOUBT"), F("FROWN"), F("LOOKUP"), F("SAD"), F("AUDIO1"), F("AUDIO2"), F("AUDIO3"), F("WAVY")};

    void LinkControlParameters() override {//Called from parent
        AddParameter(NukudeFace::Anger, pM.GetMorphWeightReference(NukudeFace::Anger), 15);
        AddParameter(NukudeFace::Wavy, pM.GetMorphWeightReference(NukudeFace::Wavy), 15);
        AddParameter(NukudeFace::Sadness, pM.GetMorphWeightReference(NukudeFace::Sadness), 15, IEasyEaseAnimator::InterpolationMethod::Cosine);
        AddParameter(NukudeFace::Surprised, pM.GetMorphWeightReference(NukudeFace::Surprised), 15);
        AddParameter(NukudeFace::Doubt, pM.GetMorphWeightReference(NukudeFace::Doubt), 15);
        AddParameter(NukudeFace::Frown, pM.GetMorphWeightReference(NukudeFace::Frown), 15);
        AddParameter(NukudeFace::LookUp, pM.GetMorphWeightReference(NukudeFace::LookUp), 15);
        AddParameter(NukudeFace::LookDown, pM.GetMorphWeightReference(NukudeFace::LookDown), 15);

        AddParameter(NukudeFace::HideBlush, pM.GetMorphWeightReference(NukudeFace::HideBlush), 15, IEasyEaseAnimator::InterpolationMethod::Cosine, true);

        AddViseme(Viseme::MouthShape::EE, pM.GetMorphWeightReference(NukudeFace::vrc_v_ee));
        AddViseme(Viseme::MouthShape::AH, pM.GetMorphWeightReference(NukudeFace::vrc_v_aa));
        AddViseme(Viseme::MouthShape::UH, pM.GetMorphWeightReference(NukudeFace::vrc_v_dd));
        AddViseme(Viseme::MouthShape::AR, pM.GetMorphWeightReference(NukudeFace::vrc_v_rr));
        AddViseme(Viseme::MouthShape::ER, pM.GetMorphWeightReference(NukudeFace::vrc_v_ch));
        AddViseme(Viseme::MouthShape::OO, pM.GetMorphWeightReference(NukudeFace::vrc_v_oh));
        AddViseme(Viseme::MouthShape::SS, pM.GetMorphWeightReference(NukudeFace::vrc_v_ss));

        AddBlinkParameter(pM.GetMorphWeightReference(NukudeFace::Blink));
    }

    void Default(){}

    void Wavy(){
        wavySelected = true;
        AddParameterFrame(NukudeFace::Wavy, 1.0f);
        AddMaterialFrame(Color::CWHITE);
    }

    void Angry(){
        AddParameterFrame(NukudeFace::Anger, 1.0f);
        AddMaterialFrame(Color::CRED);
    } 

    void Sad(){
        AddParameterFrame(NukudeFace::Sadness, 1.0f);
        AddParameterFrame(NukudeFace::Frown, 1.0f);
        AddMaterialFrame(Color::CBLUE);
    }

    void Surprised(){
        AddParameterFrame(NukudeFace::Surprised, 1.0f);
        AddParameterFrame(NukudeFace::HideBlush, 0.0f);
        AddMaterialFrame(Color::CRAINBOW);
    }
    
    void Doubt(){
        AddParameterFrame(NukudeFace::Doubt, 1.0f);
    }
    
    void Frown(){
        AddParameterFrame(NukudeFace::Frown, 1.0f);
    }

    void LookUp(){
        AddParameterFrame(NukudeFace::LookUp, 1.0f);
    }

    void LookDown(){
        AddParameterFrame(NukudeFace::LookDown, 1.0f);
    }

    void SpectrumAnalyzerCallback() override {
        AddMaterialFrame(Color::CHORIZONTALRAINBOW, 0.8f);
    }

    void AudioReactiveGradientCallback() override {
        AddMaterialFrame(Color::CHORIZONTALRAINBOW, 0.8f);
    }

    void OscilloscopeCallback() override {
        AddMaterialFrame(Color::CHORIZONTALRAINBOW, 0.8f);
    }

    // use dummy 34 pin for button, using usb keyboard for input, so no button is needed
public:
    bool MatrixEnabled() const { return matrixEnabled; }
#ifdef PROTOTRACER_SIMULATOR
    bool WavyEyeIsOpen() {
        return wavySelected && *pM.GetMorphWeightReference(NukudeFace::Blink) == 0.0f;
    }
#endif
    bool IsDVDVisible() {
        return Games::Instance().DVDActive();
    }

    void UpdatePreview() {
        // Keep the 6 KB preview buffer off the limited rendering stack.
        DMAMEM static RGBColor frame[64 * 32];
        IPixelGroup* pixels = cameras.GetMainPixelGroup();
        bool activity = Games::Instance().Active() && !IsDVDVisible();
        bool mirrorFace = !Games::Instance().Active();
        for (uint16_t y = 0; y < 32; ++y) for (uint16_t x = 0; x < 64; ++x) {
            frame[y * 64 + x] = activity ? Games::Instance().PreviewPixel(x, y)
                : *pixels->GetColor((31 - y) * 64 + (mirrorFace ? 63 - x : x));
        }
        hud.SetPreviewFrame(frame);
        hud.Update();
    }

    void Render() {
        uint32_t start = micros();
        controller.ClearSecondPanel();
        if (Games::Instance().FastCanvasActive()) {
            // The controller presents the game canvas directly; no 3D pass is needed.
            UpdatePreview();
            renderTime = (micros() - start) / 1000000.0f;
            return;
        }
        if (IsDVDVisible()) {
            RenderDVD();
            UpdatePreview();
            renderTime = (micros() - start) / 1000000.0f;
            return;
        }
        // The menu slides down from above the camera. Render its visible portion
        // with reflected text sampling, while leaving face geometry untouched.
        if (!Games::Instance().Active() && Menu::GetPosition().Y < 96.0f) {
            IPixelGroup* pixels = cameras.GetMainPixelGroup();
            float coordinateSum = pixels->GetCoordinate(0).X + pixels->GetCoordinate(63).X;
            Menu::SetRenderMirrorX(true, coordinateSum);
            Project::Render();
            ApplyMatrix();
            controller.CaptureSecondPanel(pixels->GetColors());
            Menu::SetRenderMirrorX(false);
        }
        Project::Render();
        ApplyMatrix();
        UpdatePreview();
        renderTime = (micros() - start) / 1000000.0f;
    }

    ProtogenHUB75Project() : ProtogenProject(&cameras, &controller, 3, Vector2D(), Vector2D(192.0f, 94.0f), 22, 34, 10){
        hud.UseExternalPreview();
        scene.AddObject(pM.GetObject());
        scene.AddObject(deltaDisplayBackground.GetObject());
        scene.AddObject(dvd.GetObject());
        dvd.GetObject()->SetMaterial(&dvdMaterial);
        dvd.GetObject()->Disable();

        pM.GetObject()->SetMaterial(GetFaceMaterial());
        deltaDisplayBackground.GetObject()->SetMaterial(GetFaceMaterial());

        hud.SetFaceArray(faceArray);
        // hud.SetFacePixelArray(cameras.GetMainPixelGroup());
        

        LinkControlParameters();
        
        SetWiggleSpeed(5.0f);
        SetMenuWiggleSpeed(0.0f, 0.0f, 0.0f);
        SetMenuOffset(Vector2D(17.5f, -3.0f));
        SetMenuSize(Vector2D(192, 56));
    }

    void Update(float ratio) override;

    void SelectFace(uint8_t code) {
        if (IsBooped() && code != 6) {
            Surprised();
            return;
        }

        switch(code) {
            case 0: Default();  break;
            case 1: Angry();    break;
            case 2: Doubt();    break;
            case 3: Frown();    break;
            case 4: LookUp();   break;
            case 5: Sad();      break;
            case 6: AudioReactiveGradientFace();    break;
            case 7: OscilloscopeFace();             break;
            case 8: SpectrumAnalyzerFace();        break;
            case 9: Wavy();                        break;
            default: Default();                    break;
        }
    }

    void SelectFaceFromMorse(uint8_t code) {
        if (IsBooped() && code != 24) {
            Surprised();
            return;
        }

        switch(code) {
            case 1: Angry();        break; // [A]ngry
            case 2: Surprised();    break; // [B]lush
            case 4: Doubt();        break; // [D]oubt
            case 6: Frown();        break; // [F]rown
            case 19: Sad();         break; // [S]ad
            case 21: LookUp();      break; // Look [U]p
            case 22: LookDown();    break; // Look [V] Down
            case 24: AudioReactiveGradientFace();   break; // [X] X.X
            case 25: OscilloscopeFace();            break; // [Y] Oscilloscope
            case 26: SpectrumAnalyzerFace();        break; // [Z] Spectrum
            case 23: Wavy();                        break; // [W] Wavy
            default: Default();     break; // [H] Happy
        }
    }
};
