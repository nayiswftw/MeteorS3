#pragma once

/*
 * Input HAL — Button debounce and event state machine.
 */

namespace hal {

void inputInit();
void inputService();

/// True once when a short press (<900ms) is released.
bool buttonShortPressed();

/// True once as soon as a press exceeds 900ms.
bool buttonLongPressed();

}  // namespace hal
