#pragma once

/*
 * Input HAL — Button debounce and event state machine.
 */

namespace hal {

void inputInit();
void inputService();

/// True once when a single short press is confirmed.
bool buttonShortPressed();

/// True once when a double press is detected within 300ms.
bool buttonDoublePressed();

/// True once as soon as a press exceeds 800ms.
bool buttonLongPressed();

/// True if button is currently being held down.
bool isButtonPressed();

}  // namespace hal

