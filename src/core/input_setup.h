#pragma once

namespace ww2 {

// Registers the game's input actions (keyboard + gamepad) at runtime:
// move_left, move_right, move_up, move_down, fire, bomb, throttle_up,
// throttle_down, pause, accept, back
void setup_input_actions();

// Analogue helpers that merge keys and sticks. Range -1..1.
float input_x();
float input_y();

} // namespace ww2
