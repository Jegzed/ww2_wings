#pragma once

#include "core/ui.h"
#include "core/world_env.h"
#include "game/game_state.h"
#include "game/main.h"

#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>

#include <vector>

namespace ww2 {

class MissionBase;

class PauseMenu : public godot::Control {
	GDCLASS(PauseMenu, godot::Control)

public:
	void _ready() override;
	void _process(double delta) override;
	void set_mission(MissionBase *m) { mission = m; }
	bool is_open() const { return open; }

protected:
	static void _bind_methods() {}

private:
	void toggle();
	void on_resume();
	void on_abort();

	MissionBase *mission = nullptr;
	godot::Control *panel = nullptr;
	godot::Button *resume = nullptr;
	bool open = false;
};

// Shared plumbing for the three mini-games: HUD layer, messages, pause,
// camera shake, player damage and the hand-off to the debriefing.
class MissionBase : public godot::Node3D {
	GDCLASS(MissionBase, godot::Node3D)

public:
	void set_main(Main *m) { main = m; }
	void _ready() override;
	void _process(double delta) override;
	void _exit_tree() override;

	virtual void set_shot(int index) {}
	void abort_mission();
	bool has_ended() const { return ending; }

protected:
	static void _bind_methods();

	virtual void build() {}
	virtual void tick(float dt) {}
	virtual void draw_hud(Canvas *c) {}
	virtual void on_finish() {} // last chance to fill in the result

	void message(const String &text, float seconds = 3.0f, const Color &color = ui::CREAM);
	void finish(bool success, float delay);
	// Rolls the pilot's fate after being shot down, then ends the mission.
	void player_down(const String &cause, float delay);
	void shake(float amount);
	Vector3 shake_offset() const { return shake_vec; }
	void hurt_flash(float amount);

	// HUD helpers.
	void draw_bar(Canvas *c, const Vector2 &pos, const Vector2 &size, float value, const Color &color, const String &label);
	void draw_title_card(Canvas *c);
	void draw_messages(Canvas *c);
	Vector2 hud_size() const;

	Main *main = nullptr;
	const MissionDef *def = nullptr;
	MissionResult result;
	WorldKit world;
	godot::Camera3D *camera = nullptr;
	godot::CanvasLayer *hud_layer = nullptr;
	Canvas *hud = nullptr;
	godot::ColorRect *hurt_rect = nullptr;
	godot::Ref<godot::ShaderMaterial> hurt_mat;
	PauseMenu *pause_menu = nullptr;

	float clock = 0.0f;
	bool demo = false;
	bool ending = false;
	bool end_success = false;
	float end_timer = 0.0f;
	float hurt = 0.0f;

	float shake_amount = 0.0f;
	Vector3 shake_vec;

	struct Message {
		String text;
		float time;
		float duration;
		Color color;
	};
	std::vector<Message> messages;
};

} // namespace ww2
