#pragma once

#include "core/util.h"

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <vector>

namespace ww2 {

enum ScreenId {
	SCREEN_NONE,
	SCREEN_MENU,
	SCREEN_NEW_PILOT,
	SCREEN_DIARY,
	SCREEN_BRIEFING,
	SCREEN_MISSION,
	SCREEN_DEBRIEF,
	SCREEN_MEMORIAL,
	SCREEN_ROSTER,
	SCREEN_ENDING,
	SCREEN_VIEWER,
};

// Root of the game: owns the current screen and cross-fades between them.
class Main : public godot::Node {
	GDCLASS(Main, godot::Node)

public:
	void _ready() override;
	void _process(double delta) override;
	void _exit_tree() override;

	void go(int screen, float fade_time = 0.45f);
	void quit();
	int get_screen() const { return current_id; }

protected:
	static void _bind_methods();

private:
	void swap_screen();
	void parse_args();
	void capture(const String &suffix);

	godot::Node *current = nullptr;
	int current_id = SCREEN_NONE;
	int pending_id = SCREEN_NONE;
	float fade = 1.0f; // 1 = black
	float fade_time = 0.45f;
	int fade_dir = 0; // 1 fading out, -1 fading in
	godot::CanvasLayer *overlay = nullptr;
	godot::ColorRect *fade_rect = nullptr;

	bool dump_only = false;

	// Input simulation for automated checks (--manual --hold=action,action).
	bool manual = false;
	godot::PackedStringArray held_actions;
	bool held_applied = false;

	// Screenshot automation.
	bool shot_mode = false;
	String shot_name;
	String shot_label;
	String shot_dir;
	std::vector<float> shot_times;
	size_t shot_index = 0;
	float shot_clock = 0.0f;
	int shot_wait_frames = 0;

	// Automated play-through of the whole campaign loop (--tour).
	bool tour = false;
	float tour_clock = 0.0f;
	int tour_shots = 0;
	bool tour_captured = false;
	int tour_quit_frames = 0;
};

// Implemented in screens/screen_factory.cpp.
godot::Node *create_screen(int id, Main *main);

} // namespace ww2
