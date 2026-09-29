#include "core/input_setup.h"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_map.hpp>

using namespace godot;

namespace ww2 {

namespace {
void ensure(const StringName &action) {
	InputMap *im = InputMap::get_singleton();
	if (!im->has_action(action)) {
		im->add_action(action, 0.3f);
	}
}

void key(const StringName &action, Key k) {
	ensure(action);
	Ref<InputEventKey> e;
	e.instantiate();
	e->set_physical_keycode(k);
	InputMap::get_singleton()->action_add_event(action, e);
}

void joy_button(const StringName &action, JoyButton b) {
	ensure(action);
	Ref<InputEventJoypadButton> e;
	e.instantiate();
	e->set_button_index(b);
	InputMap::get_singleton()->action_add_event(action, e);
}

void joy_axis(const StringName &action, JoyAxis axis, float value) {
	ensure(action);
	Ref<InputEventJoypadMotion> e;
	e.instantiate();
	e->set_axis(axis);
	e->set_axis_value(value);
	InputMap::get_singleton()->action_add_event(action, e);
}

void mouse_button(const StringName &action, MouseButton b) {
	ensure(action);
	Ref<InputEventMouseButton> e;
	e.instantiate();
	e->set_button_index(b);
	InputMap::get_singleton()->action_add_event(action, e);
}
} // namespace

void setup_input_actions() {
	if (InputMap::get_singleton()->has_action("move_left")) {
		return;
	}
	key("move_left", KEY_A);
	key("move_left", KEY_LEFT);
	joy_axis("move_left", JOY_AXIS_LEFT_X, -1.0f);
	joy_button("move_left", JOY_BUTTON_DPAD_LEFT);

	key("move_right", KEY_D);
	key("move_right", KEY_RIGHT);
	joy_axis("move_right", JOY_AXIS_LEFT_X, 1.0f);
	joy_button("move_right", JOY_BUTTON_DPAD_RIGHT);

	key("move_up", KEY_W);
	key("move_up", KEY_UP);
	joy_axis("move_up", JOY_AXIS_LEFT_Y, -1.0f);
	joy_button("move_up", JOY_BUTTON_DPAD_UP);

	key("move_down", KEY_S);
	key("move_down", KEY_DOWN);
	joy_axis("move_down", JOY_AXIS_LEFT_Y, 1.0f);
	joy_button("move_down", JOY_BUTTON_DPAD_DOWN);

	key("fire", KEY_SPACE);
	key("fire", KEY_J);
	mouse_button("fire", MOUSE_BUTTON_LEFT);
	joy_button("fire", JOY_BUTTON_A);
	joy_axis("fire", JOY_AXIS_TRIGGER_RIGHT, 1.0f);

	key("bomb", KEY_SPACE);
	key("bomb", KEY_B);
	key("bomb", KEY_K);
	joy_button("bomb", JOY_BUTTON_A);
	joy_button("bomb", JOY_BUTTON_X);

	key("throttle_up", KEY_SHIFT);
	key("throttle_up", KEY_E);
	joy_button("throttle_up", JOY_BUTTON_RIGHT_SHOULDER);

	key("throttle_down", KEY_CTRL);
	key("throttle_down", KEY_Q);
	joy_button("throttle_down", JOY_BUTTON_LEFT_SHOULDER);

	key("pause", KEY_ESCAPE);
	key("pause", KEY_P);
	joy_button("pause", JOY_BUTTON_START);

	key("accept", KEY_ENTER);
	key("accept", KEY_SPACE);
	key("accept", KEY_KP_ENTER);
	joy_button("accept", JOY_BUTTON_A);

	key("fullscreen", KEY_F11);

	key("back", KEY_ESCAPE);
	key("back", KEY_BACKSPACE);
	joy_button("back", JOY_BUTTON_B);
}

float input_x() {
	return Input::get_singleton()->get_axis("move_left", "move_right");
}

float input_y() {
	return Input::get_singleton()->get_axis("move_up", "move_down");
}

} // namespace ww2
