#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

#include "project_config.h"

void initButtons() {
    pinMode(BUTTON_WEATHER, INPUT_PULLUP);
    pinMode(BUTTON_CLEAR, INPUT_PULLUP);

#if BUTTON_TOGGLE_ACTIVE_LOW
    pinMode(BUTTON_TOGGLE_MODE, INPUT_PULLUP);
#else
    pinMode(BUTTON_TOGGLE_MODE, INPUT_PULLDOWN);
#endif
}

bool isButtonPressed(int button) {
    return digitalRead(button) == LOW;
}

bool isToggleModePressed() {
#if BUTTON_TOGGLE_ACTIVE_LOW
    return digitalRead(BUTTON_TOGGLE_MODE) == LOW;
#else
    return digitalRead(BUTTON_TOGGLE_MODE) == HIGH;
#endif
}

#endif

// bool isNextPressed() {
//     return digitalRead(BUTTON_NEXT) == LOW;
// }

// int getButtonInput(int min, int max, int initial) {
//     int value = initial;
//     unsigned long lastDebounce = 0;
    
//     while (true) {
//         if (millis() - lastDebounce > 300) {
//             if (isNextPressed()) {
//                 value++;
//                 if (value > max) value = min;
//                 lastDebounce = millis();
//             } else if (isConfirmPressed()) {
//                 delay(300);
//                 return value;
//             }
//         }
//         delay(50);
//     }
// }