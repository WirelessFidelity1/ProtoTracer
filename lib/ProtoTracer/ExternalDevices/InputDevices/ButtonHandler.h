/**
 * @file ButtonHandler.h
 * @brief Declares the ButtonHandler class for managing button input and state.
 *
 * This file defines the ButtonHandler class, which provides static methods for
 * initializing a button, handling interrupts, and retrieving button states and values.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include <Arduino.h>

/**
 * @class ButtonHandler
 * @brief Manages button input and state with USB keyboard support.
 */
class ButtonHandler {
private:
    static long previousMillisISR;
    static long previousMillisHold;
    static uint16_t holdingToggle;
    static uint8_t currentValue;
    static uint8_t maxValue;
    static uint8_t pin;
    static bool holdingState;

    // USB keyboard state
    static volatile bool rightPressed;
    static volatile bool okPressed;

    // USB keyboard callbacks
    static void OnRawPress(uint8_t keycode);
    static void OnRawRelease(uint8_t keycode);

    // Original interrupt handler
    static void isr();

public:
    /**
     * @brief Initializes the ButtonHandler.
     */
    static void Initialize(uint8_t pin, uint8_t maxValue, uint16_t holdingToggle);

    /**
     * @brief Retrieves the holding state.
     */
    static bool GetHoldingState();

    /**
     * @brief Retrieves the current value.
     */
    static uint8_t GetValue();

    // --------------------------------------------------
    // USB Keyboard
    // --------------------------------------------------

    /**
     * @brief Initializes the USB Host keyboard.
     */
    static void BeginUSB();

    /**
     * @brief Services the USB Host.
     * Must be called repeatedly from loop().
     */
    static void UpdateUSB();

    /**
     * @brief Returns true when a Right Arrow press occurred.
     * The press is consumed when read.
     */
    static bool GetRightPressed();

    /**
     * @brief Returns true when an Enter/OK press occurred.
     * The press is consumed when read.
     */
    static bool GetOKPressed();
};