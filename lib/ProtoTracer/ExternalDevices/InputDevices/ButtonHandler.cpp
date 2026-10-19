#include "ButtonHandler.h"
#include "USBHost_t36.h"

USBHost myusb;
USBHub hub1(myusb);
KeyboardController keyboard1(myusb);
MouseController mouse1(myusb);

USBHIDParser hid1(myusb);
USBHIDParser hid2(myusb);
USBHIDParser hid3(myusb);

volatile bool ButtonHandler::rightPressed = false;
volatile bool ButtonHandler::leftPressed = false;
volatile bool ButtonHandler::okPressed = false;
volatile bool ButtonHandler::upPressed = false;
volatile bool ButtonHandler::spacePressed = false;
volatile bool ButtonHandler::gamesInputActive = false;

long ButtonHandler::previousMillisISR;
long ButtonHandler::previousMillisHold;
uint16_t ButtonHandler::holdingToggle;
uint8_t ButtonHandler::currentValue;
uint8_t ButtonHandler::maxValue;
uint8_t ButtonHandler::pin;
bool ButtonHandler::holdingState;


// --------------------------------------------------
// Keyboard callbacks
// --------------------------------------------------

void ButtonHandler::OnRawPress(uint8_t keycode) {
    #if defined(PROJECT_PROTOGEN_HUB75)
    if (keycode == 0x52) upPressed = true;
    if (keycode == 0x2C) spacePressed = true;
    #endif

    Serial.print("USB KEY: ");
    Serial.print(keycode);
    Serial.print(" HEX: ");
    Serial.println(keycode, HEX);

    if (keycode == 0x50) leftPressed = true;

    if (keycode == 0x4F) {
        Serial.println("RIGHT");
        rightPressed = true;
    }

    if (keycode == 0x28) {
        Serial.println("OK");
        okPressed = true;
    }
}

void ButtonHandler::OnRawRelease(uint8_t keycode) {
    // Nothing needed for menu presses
}


// --------------------------------------------------
// USB initialization
// --------------------------------------------------

void ButtonHandler::BeginUSB() {

    myusb.begin();

    keyboard1.attachRawPress(OnRawPress);
    keyboard1.attachRawRelease(OnRawRelease);
}


// --------------------------------------------------
// Must be called from loop()
// --------------------------------------------------

void ButtonHandler::UpdateUSB() {
    myusb.Task();
}


bool ButtonHandler::ReadMouse(int& x, int& y, uint8_t& buttons) {
    static uint8_t heldButtons = 0;
    x = 0; y = 0;
    bool connected = static_cast<USBHIDInput&>(mouse1).operator bool();
    if (mouse1.available()) {
        x = mouse1.getMouseX();
        y = mouse1.getMouseY();
        heldButtons = mouse1.getButtons();
        mouse1.mouseDataClear();
    }
    if (!connected) heldButtons = 0;
    buttons = heldButtons;
    return connected;
}

// --------------------------------------------------
// Get and clear Right Arrow press
// --------------------------------------------------

bool ButtonHandler::GetLeftPressed() {
    bool pressed = leftPressed;
    leftPressed = false;
    return pressed;
}

bool ButtonHandler::GetRightPressed() {

    if (rightPressed) {
        rightPressed = false;
        return true;
    }

    return false;
}


// --------------------------------------------------
// Get and clear OK press
// --------------------------------------------------

bool ButtonHandler::GetOKPressed() {

    if (okPressed) {
        okPressed = false;
        return true;
    }

    return false;
}

bool ButtonHandler::GetUpPressed() {
    bool pressed = upPressed;
    upPressed = false;
    return pressed;
}

bool ButtonHandler::GetSpacePressed() {
    bool pressed = spacePressed;
    spacePressed = false;
    return pressed;
}

bool ButtonHandler::GamesInputActive() { return gamesInputActive || upPressed; }

void ButtonHandler::SetGamesInputActive(bool active) {
    rightPressed = false;
    leftPressed = false;
    okPressed = false;
    gamesInputActive = active;
}


// --------------------------------------------------
// Existing functions
// --------------------------------------------------

void ButtonHandler::isr() {
    if (millis() - previousMillisISR > 250) {
        currentValue += 1;

        if (currentValue > maxValue)
            currentValue = 0;

        previousMillisISR = millis();
    }
}

void ButtonHandler::Initialize(uint8_t pin, uint8_t maxValue, uint16_t holdingToggle) {

    ButtonHandler::maxValue = maxValue;
    ButtonHandler::pin = pin;
    ButtonHandler::holdingToggle = holdingToggle;
}

bool ButtonHandler::GetHoldingState() {
    return holdingState;
}

uint8_t ButtonHandler::GetValue() {
    return currentValue;
}
