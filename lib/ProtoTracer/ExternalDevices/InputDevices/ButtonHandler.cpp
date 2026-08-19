#include "ButtonHandler.h"
#include "USBHost_t36.h"

USBHost myusb;
USBHub hub1(myusb);
KeyboardController keyboard1(myusb);

USBHIDParser hid1(myusb);
USBHIDParser hid2(myusb);
USBHIDParser hid3(myusb);

volatile bool ButtonHandler::rightPressed = false;
volatile bool ButtonHandler::okPressed = false;

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

    Serial.print("USB KEY: ");
    Serial.print(keycode);
    Serial.print(" HEX: ");
    Serial.println(keycode, HEX);

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


// --------------------------------------------------
// Get and clear Right Arrow press
// --------------------------------------------------

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