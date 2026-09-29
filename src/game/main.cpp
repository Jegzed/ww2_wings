#include "game/main.h"

#include "core/audio.h"
#include "core/input_setup.h"
#include "game/game_state.h"

#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ww2 {

void Main::_bind_methods() {
}

void Main::parse_args() {
	PackedStringArray args = OS::get_singleton()->get_cmdline_user_args();
	GameState &gs = GameState::get();
	for (int i = 0; i < args.size(); i++) {
		String a = args[i];
		if (a.begins_with("--shot=")) {
			shot_mode = true;
			shot_name = a.substr(7);
		} else if (a.begins_with("--label=")) {
			shot_label = a.substr(8);
		} else if (a.begins_with("--out=")) {
			shot_dir = a.substr(6);
		} else if (a.begins_with("--times=")) {
			PackedStringArray parts = a.substr(8).split(",");
			shot_times.clear();
			for (int k = 0; k < parts.size(); k++) {
				shot_times.push_back((float)parts[k].to_float());
			}
		} else if (a.begins_with("--mission=")) {
			gs.instant_mission = (int)a.substr(10).to_int();
			gs.mission_index = gs.instant_mission;
		} else if (a.begins_with("--dump-audio=")) {
			// Writes every synthesised sound to disk for inspection.
			String dir = a.substr(13);
			DirAccess::make_dir_recursive_absolute(dir);
			const char *names[] = { "engine", "engine_enemy", "wind", "gun", "cannon", "explosion", "explosion_big",
				"flak", "hit", "ricochet", "whistle", "bomb_release", "click", "type", "page", "fanfare", "dirge",
				"music_menu", "music_diary", "alarm" };
			for (const char *n : names) {
				Ref<AudioStreamWAV> s = audio::stream(n);
				s->save_to_wav(dir.path_join(String(n) + String(".wav")));
			}
			dump_only = true;
		} else if (a == "--manual") {
			manual = true;
		} else if (a.begins_with("--hold=")) {
			held_actions = a.substr(7).split(",", false);
		} else if (a == "--demo") {
			gs.demo = true;
		} else if (a == "--tour") {
			tour = true;
			gs.demo = true;
		}
	}
	if (tour) {
		shot_name = "tour";
		if (shot_dir.is_empty()) {
			shot_dir = "user://shots";
		}
		DirAccess::make_dir_recursive_absolute(shot_dir);
		Engine::get_singleton()->set_time_scale(3.0);
	}
	if (shot_mode) {
		gs.demo = true;
		if (shot_times.empty()) {
			shot_times = { 1.5f, 4.0f, 7.0f };
		}
		if (shot_dir.is_empty()) {
			shot_dir = "user://shots";
		}
		DirAccess::make_dir_recursive_absolute(shot_dir);
	}
	if (manual) {
		gs.demo = false;
	}
}

void Main::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		set_process(false);
		return;
	}
	setup_input_actions();
	parse_args();
	if (dump_only) {
		get_tree()->quit();
		return;
	}

	Node *audio_host = memnew(Node);
	audio_host->set_name("Audio");
	audio_host->set_process_mode(Node::PROCESS_MODE_ALWAYS);
	add_child(audio_host);
	audio::set_host(audio_host);

	overlay = memnew(CanvasLayer);
	overlay->set_layer(100);
	overlay->set_name("Overlay");
	add_child(overlay);
	fade_rect = memnew(ColorRect);
	fade_rect->set_color(Color(0, 0, 0, 1));
	fade_rect->set_anchors_preset(Control::PRESET_FULL_RECT);
	fade_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	overlay->add_child(fade_rect);

	set_process_mode(Node::PROCESS_MODE_ALWAYS);

	int start = SCREEN_MENU;
	if (shot_mode) {
		GameState &gs = GameState::get();
		gs.has_pilot = true;
		if (shot_name == "viewer") {
			start = SCREEN_VIEWER;
		} else if (shot_name == "newpilot") {
			start = SCREEN_NEW_PILOT;
		} else if (shot_name == "diary") {
			start = SCREEN_DIARY;
		} else if (shot_name == "briefing") {
			start = SCREEN_BRIEFING;
		} else if (shot_name == "mission") {
			start = SCREEN_MISSION;
		} else if (shot_name == "debrief" || shot_name == "memorial") {
			MissionResult r;
			r.valid = true;
			r.type = gs.current_mission().type;
			r.success = shot_name == "debrief";
			r.air_kills = 3;
			r.ground_kills = 0;
			r.shots_fired = 640;
			r.shots_hit = 171;
			r.score = 1350;
			r.duration = 222.0f;
			r.targets_destroyed = 3;
			r.targets_total = 3;
			if (shot_name == "memorial") {
				r.shot_down = true;
				r.killed = true;
				r.cause = "Shot down in flames over the Channel";
			}
			gs.apply_result(r);
			gs.last = r;
			start = shot_name == "debrief" ? SCREEN_DEBRIEF : SCREEN_MEMORIAL;
		} else if (shot_name == "roster") {
			start = SCREEN_ROSTER;
		} else if (shot_name == "ending") {
			start = SCREEN_ENDING;
		}
	} else if (tour) {
		start = SCREEN_MENU;
	} else if (GameState::get().demo) {
		GameState::get().has_pilot = true;
		start = SCREEN_MISSION;
	}
	pending_id = start;
	swap_screen();
	fade = 1.0f;
	fade_dir = -1;
}

void Main::_exit_tree() {
	audio::shutdown();
	audio::set_host(nullptr);
}

void Main::go(int screen, float p_fade_time) {
	if (fade_dir == 1) {
		return; // already leaving
	}
	pending_id = screen;
	fade_time = MAX(p_fade_time, 0.01f);
	fade_dir = 1;
}

void Main::quit() {
	get_tree()->quit();
}

void Main::swap_screen() {
	get_tree()->set_pause(false);
	if (current) {
		remove_child(current);
		current->queue_free();
		current = nullptr;
	}
	current_id = pending_id;
	pending_id = SCREEN_NONE;
	tour_clock = 0.0f;
	tour_captured = false;
	current = create_screen(current_id, this);
	if (current) {
		add_child(current);
		move_child(current, 0);
	}
}

static void report_playing(Node *n, const String &where) {
	AudioStreamPlayer *p = Object::cast_to<AudioStreamPlayer>(n);
	if (p && p->is_playing() && p->get_stream().is_valid()) {
		UtilityFunctions::print("AUDIO ", where, ": ", p->get_stream()->get_name(), " vol=", p->get_volume_db(), " path=", p->get_path());
	}
	for (int i = 0; i < n->get_child_count(); i++) {
		report_playing(n->get_child(i), where);
	}
}

void Main::capture(const String &suffix) {
	if (tour) {
		report_playing(this, suffix);
	}
	Ref<Image> img = get_viewport()->get_texture()->get_image();
	if (img.is_valid()) {
		String path = shot_dir.path_join((shot_label.is_empty() ? shot_name : shot_label) + String("_") + suffix + String(".png"));
		img->save_png(path);
		UtilityFunctions::print("Saved ", path);
	}
}

void Main::_process(double delta) {
	if (Input::get_singleton()->is_action_just_pressed("fullscreen")) {
		DisplayServer *ds = DisplayServer::get_singleton();
		bool full = ds->window_get_mode() == DisplayServer::WINDOW_MODE_FULLSCREEN ||
				ds->window_get_mode() == DisplayServer::WINDOW_MODE_EXCLUSIVE_FULLSCREEN;
		ds->window_set_mode(full ? DisplayServer::WINDOW_MODE_WINDOWED : DisplayServer::WINDOW_MODE_FULLSCREEN);
	}

	// Loading a screen can stall a frame; do not let that skip fades or captures.
	float dt = MIN((float)delta, 0.1f);
	audio::update(dt);

	if (fade_dir != 0) {
		fade += fade_dir * dt / fade_time;
		if (fade_dir == 1 && fade >= 1.0f) {
			fade = 1.0f;
			swap_screen();
			fade_dir = -1;
		} else if (fade_dir == -1 && fade <= 0.0f) {
			fade = 0.0f;
			fade_dir = 0;
		}
	}
	if (fade_rect) {
		fade_rect->set_color(Color(0, 0, 0, smooth01(fade)));
		fade_rect->set_visible(fade > 0.001f);
	}

	if (tour && current && fade_dir == 0) {
		tour_clock += dt;
		bool is_mission = current_id == SCREEN_MISSION;
		float wait = is_mission ? 12.0f : 4.0f;
		if (!tour_captured && tour_clock >= wait) {
			tour_captured = true;
			String n = String::num_int64(tour_shots++).pad_zeros(2) + String("_") + current->get_class();
			capture(n);
			if (current_id == SCREEN_ENDING) {
				tour_quit_frames = 1;
			} else if (current->has_method("auto_advance")) {
				current->call("auto_advance");
			}
		} else if (tour_captured && !is_mission && tour_clock >= wait + 6.0f && current->has_method("auto_advance")) {
			// Still here: the screen wanted a second press.
			tour_clock = wait;
			current->call("auto_advance");
		}
	}
	if (tour_quit_frames > 0 && ++tour_quit_frames > 4) {
		get_tree()->quit();
	}

	if (shot_mode && !held_applied && shot_clock > 1.0f && held_actions.size() > 0) {
		held_applied = true;
		for (int i = 0; i < held_actions.size(); i++) {
			Input::get_singleton()->action_press(held_actions[i]);
		}
	}
	if (shot_mode) {
		shot_clock += dt;
		if (shot_index < shot_times.size()) {
			if (shot_clock >= shot_times[shot_index]) {
				capture(String::num_int64((int64_t)shot_index));
				shot_index++;
				if (current && current->has_method("set_shot")) {
					current->call("set_shot", (int)shot_index);
				}
			}
		} else {
			shot_wait_frames++;
			if (shot_wait_frames > 3) {
				get_tree()->quit();
			}
		}
	}
}

} // namespace ww2
