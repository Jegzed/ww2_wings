#include "missions/mission_base.h"

#include "core/audio.h"
#include "core/materials.h"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/viewport.hpp>

using namespace godot;

namespace ww2 {

// ---------------------------------------------------------------------------
// Pause menu
// ---------------------------------------------------------------------------

void PauseMenu::_ready() {
	set_process_mode(Node::PROCESS_MODE_ALWAYS);
	ui::fill(this);
	set_mouse_filter(Control::MOUSE_FILTER_IGNORE);

	panel = memnew(Control);
	add_child(panel);
	ui::fill(panel);
	panel->set_visible(false);

	ColorRect *dim = memnew(ColorRect);
	dim->set_color(Color(0, 0, 0, 0.6f));
	panel->add_child(dim);
	ui::fill(dim);

	VBoxContainer *vb = memnew(VBoxContainer);
	vb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	vb->add_theme_constant_override("separation", 16);
	panel->add_child(vb);
	vb->set_anchors_and_offsets_preset(Control::PRESET_CENTER);
	vb->set_custom_minimum_size(Vector2(460, 0));
	vb->set_offset(SIDE_LEFT, -230);
	vb->set_offset(SIDE_RIGHT, 230);
	vb->set_offset(SIDE_TOP, -180);
	vb->set_offset(SIDE_BOTTOM, 180);

	Label *title = ui::label("PAUSED", ui::font_title(), 90, ui::CREAM);
	title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	vb->add_child(title);

	resume = ui::button("Resume", 34, 460);
	resume->connect("pressed", callable_mp(this, &PauseMenu::on_resume));
	vb->add_child(resume);
	Button *abort = ui::button("Abort mission", 34, 460);
	abort->connect("pressed", callable_mp(this, &PauseMenu::on_abort));
	vb->add_child(abort);
}

void PauseMenu::_process(double delta) {
	if (!mission || mission->has_ended()) {
		return;
	}
	if (Input::get_singleton()->is_action_just_pressed("pause")) {
		toggle();
	}
}

void PauseMenu::toggle() {
	open = !open;
	panel->set_visible(open);
	get_tree()->set_pause(open);
	Input::get_singleton()->set_mouse_mode(open ? Input::MOUSE_MODE_VISIBLE : Input::MOUSE_MODE_HIDDEN);
	if (open) {
		resume->grab_focus();
	}
}

void PauseMenu::on_resume() {
	if (open) {
		toggle();
	}
}

void PauseMenu::on_abort() {
	if (open) {
		toggle();
	}
	if (mission) {
		mission->abort_mission();
	}
}

// ---------------------------------------------------------------------------
// Mission base
// ---------------------------------------------------------------------------

void MissionBase::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_shot", "index"), &MissionBase::set_shot);
}

void MissionBase::_ready() {
	// Main keeps running while paused; the mission itself must stop.
	set_process_mode(Node::PROCESS_MODE_PAUSABLE);
	GameState &gs = GameState::get();
	def = &gs.current_mission();
	demo = gs.demo;
	result = MissionResult();
	result.valid = true;
	result.type = def->type;

	build();

	hud_layer = memnew(CanvasLayer);
	hud_layer->set_layer(10);
	add_child(hud_layer);

	hurt_rect = ui::vignette(0.45f);
	hurt_mat = hurt_rect->get_material();
	hud_layer->add_child(hurt_rect);

	hud = memnew(Canvas);
	hud->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	hud->on_draw = [this](Canvas *c) {
		draw_hud(c);
		draw_messages(c);
		draw_title_card(c);
	};
	hud_layer->add_child(hud);
	ui::fill(hud);

	if (!demo) {
		Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_HIDDEN);
		pause_menu = memnew(PauseMenu);
		pause_menu->set_mission(this);
		hud_layer->add_child(pause_menu);
	}
}

void MissionBase::_exit_tree() {
	Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
	// Looping sounds must not outlive the mission.
	for (int i = 0; i < get_child_count(); i++) {
		AudioStreamPlayer *p = Object::cast_to<AudioStreamPlayer>(get_child(i));
		if (p) {
			p->stop();
		}
	}
}

Vector2 MissionBase::hud_size() const {
	return hud ? hud->get_size() : Vector2(1920, 1080);
}

void MissionBase::_process(double delta) {
	float dt = MIN((float)delta, 0.05f);
	clock += dt;

	// Camera shake: decaying random offset.
	shake_amount = MAX(shake_amount - dt * 2.2f, 0.0f);
	if (shake_amount > 0.0f) {
		Rng &r = grng();
		float a = shake_amount * shake_amount;
		shake_vec = Vector3(r.range(-1, 1), r.range(-1, 1), r.range(-1, 1)) * a;
	} else {
		shake_vec = Vector3();
	}

	hurt = MAX(hurt - dt * 1.4f, 0.0f);
	if (hurt_mat.is_valid()) {
		hurt_mat->set_shader_parameter("hurt", saturate(hurt));
	}

	tick(dt);

	for (Message &m : messages) {
		m.time += dt;
	}
	while (!messages.empty() && messages.front().time >= messages.front().duration) {
		messages.erase(messages.begin());
	}

	if (ending) {
		end_timer -= dt;
		if (end_timer <= 0.0f && main) {
			ending = false;
			set_process(false);
			result.success = end_success;
			result.duration = clock;
			on_finish();
			GameState &gs = GameState::get();
			gs.apply_result(result);
			gs.last = result;
			gs.save();
			main->go(SCREEN_DEBRIEF, 0.9f);
		}
	}
	if (hud) {
		hud->queue_redraw();
	}
}

void MissionBase::message(const String &text, float seconds, const Color &color) {
	if (!messages.empty() && messages.back().text == text) {
		messages.back().time = MIN(messages.back().time, 0.3f);
		return;
	}
	if (messages.size() >= 4) {
		messages.erase(messages.begin());
	}
	messages.push_back({ text, 0.0f, seconds, color });
}

void MissionBase::finish(bool success, float delay) {
	if (ending) {
		return;
	}
	ending = true;
	end_success = success;
	end_timer = delay;
}

void MissionBase::abort_mission() {
	if (ending) {
		return;
	}
	result.aborted = true;
	result.cause = "Turned for home before the job was done.";
	finish(false, 0.1f);
}

void MissionBase::player_down(const String &cause, float delay) {
	if (ending) {
		return;
	}
	GameState &gs = GameState::get();
	result.shot_down = true;
	Rng &r = grng();
	if (r.f() < gs.survival_chance()) {
		if (r.chance(0.5f)) {
			result.wounded = true;
			result.cause = cause + String(" Bailed out wounded and was picked up by an infantry patrol.");
		} else {
			result.cause = cause + String(" Got out in time and walked back through the lines.");
		}
	} else {
		result.killed = true;
		result.cause = cause;
	}
	finish(false, delay);
}

void MissionBase::shake(float amount) {
	shake_amount = MIN(MAX(shake_amount, amount), 1.6f);
}

void MissionBase::hurt_flash(float amount) {
	hurt = MIN(hurt + amount, 1.2f);
}

void MissionBase::draw_bar(Canvas *c, const Vector2 &pos, const Vector2 &size, float value, const Color &color,
		const String &label) {
	c->draw_rect(Rect2(pos, size), Color(0, 0, 0, 0.45f), true);
	float v = saturate(value);
	c->draw_rect(Rect2(pos + Vector2(2, 2), Vector2((size.x - 4) * v, size.y - 4)), color, true);
	c->draw_rect(Rect2(pos, size), Color(1, 1, 1, 0.5f), false, 1.5f);
	if (!label.is_empty()) {
		c->text_shadowed(ui::font_title(), pos + Vector2(0, -8), label, 22, Color(1, 1, 1, 0.85f));
	}
}

void MissionBase::draw_messages(Canvas *c) {
	Vector2 size = hud_size();
	float y = 150.0f;
	for (const Message &m : messages) {
		float a = smooth01(m.time / 0.25f) * smooth01((m.duration - m.time) / 0.5f);
		Color col = m.color;
		col.a *= a;
		c->text_shadowed(ui::font_title(), Vector2(0, y), m.text, 46, col, 1, size.x);
		y += 54.0f;
	}
}

void MissionBase::draw_title_card(Canvas *c) {
	if (clock > 5.0f || !def) {
		return;
	}
	Vector2 size = hud_size();
	float a = smooth01(clock / 0.6f) * smooth01((5.0f - clock) / 1.0f);
	float y = size.y * 0.30f;
	c->draw_rect(Rect2(0, y - 90, size.x, 210), Color(0, 0, 0, 0.35f * a), true);
	c->text(ui::font_title(), Vector2(0, y - 30), String(mission_type_name(def->type)) + String("   -   ") +
														   String(def->date).to_upper(),
			30, Color(0.96f, 0.76f, 0.28f, a), 1, size.x);
	c->text(ui::font_title(), Vector2(0, y + 50), String(def->title).to_upper(), 96, Color(0.93f, 0.90f, 0.80f, a), 1,
			size.x);
	c->text(ui::font_body(), Vector2(0, y + 98), String(def->objective), 32, Color(1, 1, 1, 0.85f * a), 1, size.x);
}

} // namespace ww2
