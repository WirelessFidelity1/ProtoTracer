#include "HUB75Controller.h"
#include "../ExternalDevices/InputDevices/Menu/Games.h"
#include "../ExternalDevices/InputDevices/Menu/Menu.h"

// Keep transition images in RAM2, preserving RAM1 stack space for rendering.
DMAMEM RGBColor HUB75Controller::displayFrame[2 * 64 * 32];
DMAMEM RGBColor HUB75Controller::slideFrom[2 * 64 * 32];

//Macro calls from SmartMatrix library included in SmartMatrixHUB75
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);

SMARTMATRIX_APA_ALLOCATE_BUFFERS(apamatrix, kApaMatrixWidth, kApaMatrixHeight, kApaRefreshDepth, kApaDmaBufferRows, kApaPanelType, kApaMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(apaBackgroundLayer, kApaMatrixWidth, kApaMatrixHeight, COLOR_DEPTH, kApaBackgroundLayerOptions);

HUB75Controller::HUB75Controller(CameraManager* cameras, uint8_t maxBrightness, uint8_t maxAccentBrightness) : Controller(cameras, maxBrightness, maxAccentBrightness){}

void HUB75Controller::Initialize(){
    for (uint16_t i=0;i<2*64*32;++i) {
        displayFrame[i]=RGBColor(0,0,0);
        slideFrom[i]=RGBColor(0,0,0);
    }
    //HUB75
    matrix.addLayer(&backgroundLayer);
    matrix.begin();
    matrix.setRefreshRate(240);

    backgroundLayer.swapBuffers();//for ESP32 - first is ignored

    //APA102
    apamatrix.addLayer(&apaBackgroundLayer);
    apamatrix.begin();
    apamatrix.setRefreshRate(240);
}

void HUB75Controller::Display(){
    UpdateBrightness();

    matrix.setBrightness(brightness);
    apamatrix.setBrightness(accentBrightness);

    IPixelGroup* camPixels = cameras->GetCameras()[0]->GetPixelGroup();
    IPixelGroup* camSidePixelsL = cameras->GetCameras()[1]->GetPixelGroup();
    IPixelGroup* camSidePixelsR = cameras->GetCameras()[2]->GetPixelGroup();
    bool showDVD = Games::Instance().DVDActive();
    bool upMenu = Games::Instance().UpMenuActive();
    uint32_t now = millis();
    if (upMenu != previousUpMenu && !Games::Instance().BSODActive()) {
        for (uint16_t i = 0; i < 2 * 64 * 32; ++i) slideFrom[i] = displayFrame[i];
        slideStarted = now;
        slideRunning = true;
    }
    previousUpMenu = upMenu;
    if (Games::Instance().BSODActive()) slideRunning = false;

    for (uint16_t y = 0; y < 32; y++) {
        for (uint16_t x = 0; x < 64; x++){
            uint16_t pixelNum = y * 64 + x;

            rgb24 rgbColor = rgb24((uint16_t)camPixels->GetColor(pixelNum)->R, (uint16_t)camPixels->GetColor(pixelNum)->G, (uint16_t)camPixels->GetColor(pixelNum)->B);
            if (Games::Instance().Active() && !showDVD) {
                RGBColor gameColor = Games::Instance().Pixel(x, 31 - y, true);
                rgbColor = rgb24(gameColor.R, gameColor.G, gameColor.B);
            }

            // Faces mirror across the visor; game text must read normally on both panels.
            uint16_t secondPanelX = Games::Instance().Active() || showDVD ? x : 63 - x;
            rgb24 firstColor = rgbColor;
            if (Games::Instance().Active() && !showDVD) {
                // The two physical panels have their covered rear edges on opposite ends.
                RGBColor firstGameColor = Games::Instance().Pixel(x, 31 - y, false);
                firstColor = rgb24(firstGameColor.R, firstGameColor.G, firstGameColor.B);
            }
            if (useSecondPanelPixels && !Games::Instance().Active() && !showDVD) {
                // The reflected text pass belongs on the first physical panel.
                // Both passes contain identical face geometry.
                const RGBColor& color = secondPanelPixels[pixelNum];
                firstColor = rgb24(color.R, color.G, color.B);
            }
            displayFrame[(31-y)*64+x] = RGBColor(firstColor.red, firstColor.green, firstColor.blue);
            displayFrame[2048+(31-y)*64+secondPanelX] = RGBColor(rgbColor.red, rgbColor.green, rgbColor.blue);
        }
    }

    float progress = fminf((now-slideStarted)/250.0f,1.0f);
    if (progress >= 1.0f) slideRunning = false;
    int travel = int(32 * progress * progress * (3-2*progress) + 0.5f);
    // Read incoming rows before overwriting them, so this buffer also retains
    // the presented frame for a seamless reversal if Up is pressed mid-slide.
    for (int row = 0; row < 32; ++row) {
        int y = upMenu ? row : 31-row;
        int incomingY = upMenu ? y+32-travel : y-32+travel;
        int outgoingY = upMenu ? y-travel : y+travel;
        for (uint8_t panel = 0; panel < 2; ++panel) for (int x = 0; x < 64; ++x) {
            int index = panel*2048+y*64+x;
            RGBColor color = displayFrame[index];
            if (slideRunning) {
                color = incomingY >= 0 && incomingY < 32
                    ? displayFrame[panel*2048+incomingY*64+x]
                    : slideFrom[panel*2048+outgoingY*64+x];
            }
            displayFrame[index] = color;
            backgroundLayer.drawPixel(x,y+panel*32,rgb24(color.R,color.G,color.B));
        }
    }
    backgroundLayer.swapBuffers(false);

    for (uint16_t x = 0; x < kApaMatrixWidth / 2; x++){
        rgb24 rgbColorL = rgb24((uint16_t)camSidePixelsL->GetColor(x)->R, (uint16_t)camSidePixelsL->GetColor(x)->G, (uint16_t)camSidePixelsL->GetColor(x)->B);
        rgb24 rgbColorR = rgb24((uint16_t)camSidePixelsR->GetColor(x)->R, (uint16_t)camSidePixelsR->GetColor(x)->G, (uint16_t)camSidePixelsR->GetColor(x)->B);
        
        apaBackgroundLayer.drawPixel(x, 0, rgbColorL);
        apaBackgroundLayer.drawPixel(x + 88, 0, rgbColorR);
    }
    
    apaBackgroundLayer.swapBuffers(false);
}

void HUB75Controller::SetBrightness(uint8_t maxBrightness){
    this->maxBrightness = maxBrightness * 25 + 5;
    
    if(isOn){//past soft start
        this->brightness = maxBrightness * 25 + 5;
    }
}

void HUB75Controller::CaptureSecondPanel(const RGBColor* pixels) {
    for (uint16_t i = 0; i < 64 * 32; ++i) secondPanelPixels[i] = pixels[i];
    useSecondPanelPixels = true;
}

void HUB75Controller::SetAccentBrightness(uint8_t maxAccentBrightness){
    this->maxAccentBrightness = maxAccentBrightness * 12 + 5;
    
    if(isOn){//past soft start
        this->accentBrightness = maxAccentBrightness * 12 + 5;
    }
}
