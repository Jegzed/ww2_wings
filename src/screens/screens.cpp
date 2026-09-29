#include "screens/screens.h"

#include "core/audio.h"
#include "core/materials.h"
#include "screens/backdrop.h"

#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/gradient_texture2d.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

using namespace godot;

namespace ww2 {

namespace {

TextureRect *gradient_rect(const Color &a, const Color &b, const Vector2 &from, const Vector2 &to) {
	Ref<Gradient> g;
	g.instantiate();
	PackedFloat32Array offsets;
	offsets.push_back(0.0f);
	offsets.push_back(1.0f);
	PackedColorArray colors;
	colors.push_back(a);
	colors.push_back(b);
	g->set_offsets(offsets);
	g->set_colors(colors);
	Ref<GradientTexture2D> t;
	t.instantiate();
	t->set_gradient(g);
	t->set_width(256);
	t->set_height(256);
	t->set_fill_from(from);
	t->set_fill_to(to);
	TextureRect *r = memnew(TextureRect);
	r->set_texture(t);
	r->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	r->set_stretch_mode(TextureRect::STRETCH_SCALE);
	r->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	return r;
}

Control *place(Control *parent, Control *child, const Vector2 &pos, const Vector2 &size = Vector2()) {
	parent->add_child(child);
	child->set_position(pos);
	if (size.x > 0.0f || size.y > 0.0f) {
		child->set_size(size);
	}
	return child;
}

// Anchors a control to a screen corner with a pixel offset.
void anchor(Control *c, Control::LayoutPreset preset, const Vector2 &offset, const Vector2 &size) {
	c->set_anchors_preset(preset);
	float ax = c->get_anchor(SIDE_LEFT);
	float ay = c->get_anchor(SIDE_TOP);
	float x = offset.x - size.x * ax;
	float y = offset.y - size.y * ay;
	c->set_offset(SIDE_LEFT, x);
	c->set_offset(SIDE_TOP, y);
	c->set_offset(SIDE_RIGHT, x + size.x);
	c->set_offset(SIDE_BOTTOM, y + size.y);
}

String pilot_title(const Pilot &p) {
	return String(rank_name(p.rank)) + String(" ") + p.name;
}

String pips_text(int value) {
	String s;
	for (int i = 0; i < 10; i++) {
		s += i < value ? String::utf8("■") : String::utf8("□");
	}
	return s;
}

const char *controls_text(MissionType t) {
	switch (t) {
		case MISSION_DOGFIGHT:
			return "STICK   A / D  roll     S  pull up     W  push over\n"
				   "THROTTLE   Shift  open     Ctrl  close\n"
				   "GUNS   Space\n"
				   "Roll toward the enemy, then pull. Fire when the pipper is on him.";
		case MISSION_BOMBING:
			return "STEER   W A S D  or arrow keys\n"
				   "BOMBS   Space\n"
				   "The circle on the ground shows where a bomb released NOW will land.";
		case MISSION_STRAFING:
			return "SLIDE   A / D\n"
				   "HEIGHT   W  dive     S  climb\n"
				   "GUNS   Space\n"
				   "Your rounds strike ahead of you - get low for a tight pattern.";
	}
	return "";
}

} // namespace

// ---------------------------------------------------------------------------

void ScreenBase::_bind_methods() {
	ClassDB::bind_method(D_METHOD("auto_advance"), &ScreenBase::auto_advance);
}

void ScreenBase::add_desk(const Color &top, const Color &bottom) {
	ui::fill(this);
	TextureRect *bg = gradient_rect(top, bottom, Vector2(0.5f, 0.0f), Vector2(0.5f, 1.0f));
	add_child(bg);
	ui::fill(bg);
	add_child(ui::vignette(0.85f));
}

Control *ScreenBase::add_page(const Vector2 &size, bool ruled, float tilt_deg, const Vector2 &offset) {
	Control *holder = memnew(Control);
	add_child(holder);
	anchor(holder, Control::PRESET_CENTER, offset, size);
	holder->set_pivot_offset(size * 0.5f);
	holder->set_rotation_degrees(tilt_deg);

	ColorRect *shadow = memnew(ColorRect);
	shadow->set_color(Color(0, 0, 0, 0.4f));
	shadow->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	place(holder, shadow, Vector2(14, 18), size);

	ColorRect *paper = ui::paper(ruled);
	place(holder, paper, Vector2(), size);
	Ref<ShaderMaterial> mat = paper->get_material();
	if (mat.is_valid()) {
		mat->set_shader_parameter("rect_size", size);
	}
	return holder;
}

// ---------------------------------------------------------------------------
// Menu
// ---------------------------------------------------------------------------

void MenuScreen::_ready() {
	ui::fill(this);
	GameState &gs = GameState::get();
	gs.instant_action = false;

	Backdrop *bd = memnew(Backdrop);
	bd->set_time(TOD_DUSK);
	add_child(bd);

	TextureRect *shade = gradient_rect(Color(0, 0, 0, 0.78f), Color(0, 0, 0, 0.0f), Vector2(0, 0.5f), Vector2(1, 0.5f));
	add_child(shade);
	shade->set_anchors_preset(Control::PRESET_LEFT_WIDE);
	shade->set_offset(SIDE_RIGHT, 1150.0f);
	add_child(ui::vignette(0.6f));

	Label *title = ui::label("WW2 WINGS", ui::font_title(), 170, ui::CREAM);
	title->add_theme_constant_override("outline_size", 0);
	title->add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.6f));
	title->add_theme_constant_override("shadow_offset_x", 5);
	title->add_theme_constant_override("shadow_offset_y", 6);
	place(this, title, Vector2(110, 70));

	ColorRect *rule = memnew(ColorRect);
	rule->set_color(ui::AMBER);
	place(this, rule, Vector2(116, 268), Vector2(560, 5));

	Label *sub = ui::label("N O R M A N D Y   ·   1 9 4 4", ui::font_title(), 40, ui::CREAM);
	sub->set_text(String::utf8("N O R M A N D Y   ·   1 9 4 4"));
	place(this, sub, Vector2(116, 286));

	VBoxContainer *box = memnew(VBoxContainer);
	box->add_theme_constant_override("separation", 12);
	place(this, box, Vector2(110, 410));

	bool has_save = gs.has_save();
	Button *first = nullptr;
	if (has_save) {
		Button *b = ui::button("Continue Career");
		b->connect("pressed", callable_mp(this, &MenuScreen::on_continue));
		box->add_child(b);
		first = b;
	}
	{
		Button *b = ui::button("New Career");
		b->connect("pressed", callable_mp(this, &MenuScreen::on_new));
		box->add_child(b);
		if (!first) {
			first = b;
		}
	}
	{
		Label *l = ui::label("INSTANT ACTION", ui::font_title(), 24, Color(0.96f, 0.76f, 0.28f, 0.8f));
		Control *gap = memnew(Control);
		gap->set_custom_minimum_size(Vector2(0, 10));
		box->add_child(gap);
		box->add_child(l);
		HBoxContainer *row = memnew(HBoxContainer);
		row->add_theme_constant_override("separation", 10);
		const char *names[] = { "Dogfight", "Bombing", "Strafing" };
		for (int i = 0; i < 3; i++) {
			Button *b = ui::small_button(String(names[i]).to_upper(), 26);
			b->set_custom_minimum_size(Vector2(133, 0));
			b->connect("pressed", callable_mp(this, &MenuScreen::on_instant).bind(i));
			row->add_child(b);
		}
		box->add_child(row);
		Control *gap2 = memnew(Control);
		gap2->set_custom_minimum_size(Vector2(0, 10));
		box->add_child(gap2);
	}
	if (has_save) {
		Button *b = ui::button("Squadron Roster");
		b->connect("pressed", callable_mp(this, &MenuScreen::on_roster));
		box->add_child(b);
	}
	{
		Button *b = ui::button("Quit");
		b->connect("pressed", callable_mp(this, &MenuScreen::on_quit));
		box->add_child(b);
	}
	if (first) {
		first->call_deferred("grab_focus");
	}

	Label *foot = ui::label("A tribute to Cinemaware's Wings (1990).  Built with Godot and C++.", ui::font_body(), 22,
			Color(1, 1, 1, 0.45f));
	add_child(foot);
	anchor(foot, Control::PRESET_BOTTOM_LEFT, Vector2(116, -30), Vector2(900, 30));

	audio::play_music("music_menu", -9.0f);
}

void MenuScreen::on_continue() {
	GameState &gs = GameState::get();
	if (!gs.load()) {
		on_new();
		return;
	}
	if (gs.campaign_complete()) {
		main->go(SCREEN_ENDING);
	} else if (!gs.has_pilot) {
		main->go(SCREEN_NEW_PILOT);
	} else {
		main->go(SCREEN_DIARY);
	}
}

void MenuScreen::on_new() {
	GameState &gs = GameState::get();
	gs.new_campaign();
	main->go(SCREEN_NEW_PILOT);
}

void MenuScreen::on_instant(int mission) {
	GameState &gs = GameState::get();
	if (gs.has_save()) {
		gs.load();
	}
	gs.instant_action = true;
	gs.instant_mission = mission;
	main->go(SCREEN_BRIEFING);
}

void MenuScreen::on_roster() {
	GameState::get().load();
	main->go(SCREEN_ROSTER);
}

void MenuScreen::on_quit() {
	main->quit();
}

// ---------------------------------------------------------------------------
// New pilot
// ---------------------------------------------------------------------------

namespace {
const char *FIRST_NAMES[] = { "James", "Robert", "William", "Frank", "Eddie", "Walter", "Thomas", "Charles", "Harold",
	"Raymond", "Arthur", "Louis", "George", "Daniel", "Henry" };
const char *LAST_NAMES[] = { "Carter", "Sullivan", "Kowalski", "Bennett", "Hayes", "Moretti", "Lindqvist", "Brooks",
	"McAllister", "Novak", "Reed", "Fischer", "Delgado", "Whitaker", "Olsen" };
const char *TOWNS[] = { "Dayton, Ohio", "Brooklyn, New York", "Tulsa, Oklahoma", "Duluth, Minnesota",
	"Macon, Georgia", "Butte, Montana", "Fresno, California", "Scranton, Pennsylvania", "Lubbock, Texas",
	"Bangor, Maine", "Cedar Rapids, Iowa", "Mobile, Alabama" };

void style_line_edit(LineEdit *e) {
	e->add_theme_font_override("font", ui::font_type());
	e->add_theme_font_size_override("font_size", 34);
	e->add_theme_color_override("font_color", ui::INK_BLUE);
	e->add_theme_color_override("caret_color", ui::INK);
	e->add_theme_color_override("selection_color", Color(0.3f, 0.4f, 0.7f, 0.4f));
	Ref<StyleBoxFlat> s = ui::flat(Color(0, 0, 0, 0.0f), ui::INK, 0, 4);
	s->set_border_width(SIDE_BOTTOM, 2);
	Ref<StyleBoxFlat> f = s->duplicate();
	f->set_bg_color(Color(1, 1, 1, 0.25f));
	e->add_theme_stylebox_override("normal", s);
	e->add_theme_stylebox_override("focus", f);
	e->set_max_length(24);
}
} // namespace

void NewPilotScreen::_ready() {
	add_desk(Color(0.10f, 0.11f, 0.09f), Color(0.03f, 0.035f, 0.03f));
	GameState &gs = GameState::get();
	bool replacement = !gs.fallen.empty();

	Control *page = add_page(Vector2(900, 1000), false, -0.6f);

	place(page, ui::label("PERSONNEL RECORD", ui::font_type(), 48, ui::INK), Vector2(70, 50));
	place(page, ui::label("406th FIGHTER SQUADRON  -  NINTH AIR FORCE", ui::font_type(), 22, ui::INK), Vector2(72, 112));
	ColorRect *rule = memnew(ColorRect);
	rule->set_color(ui::INK);
	place(page, rule, Vector2(70, 150), Vector2(760, 3));

	if (replacement) {
		Label *l = ui::label("REPLACEMENT PILOT - reporting for duty", ui::font_type(), 22, ui::RED);
		place(page, l, Vector2(72, 160));
	}

	place(page, ui::label("NAME", ui::font_type(), 22, ui::INK), Vector2(72, 200));
	name_edit = memnew(LineEdit);
	style_line_edit(name_edit);
	place(page, name_edit, Vector2(250, 186), Vector2(580, 50));

	place(page, ui::label("HOME TOWN", ui::font_type(), 22, ui::INK), Vector2(72, 270));
	town_edit = memnew(LineEdit);
	style_line_edit(town_edit);
	town_edit->set_max_length(30);
	place(page, town_edit, Vector2(250, 256), Vector2(580, 50));

	place(page, ui::label("APTITUDES", ui::font_type(), 28, ui::INK), Vector2(72, 350));

	for (int i = 0; i < SKILL_COUNT; i++) {
		float y = 410.0f + i * 100.0f;
		place(page, ui::label(String(skill_name(i)).to_upper(), ui::font_type(), 28, ui::INK), Vector2(72, y));
		pips[i] = ui::label("", ui::font_body(), 34, ui::INK_BLUE);
		place(page, pips[i], Vector2(300, y - 6));
		Label *blurb = ui::label(skill_blurb(i), ui::font_hand(), 22, Color(0.2f, 0.2f, 0.3f, 0.9f));
		place(page, blurb, Vector2(72, y + 40));

		minus[i] = ui::small_button("-", 30);
		minus[i]->connect("pressed", callable_mp(this, &NewPilotScreen::on_skill).bind(i, -1));
		place(page, minus[i], Vector2(690, y - 8), Vector2(60, 50));
		plus[i] = ui::small_button("+", 30);
		plus[i]->connect("pressed", callable_mp(this, &NewPilotScreen::on_skill).bind(i, 1));
		place(page, plus[i], Vector2(765, y - 8), Vector2(60, 50));
	}

	points_label = ui::label("", ui::font_type(), 26, ui::RED);
	place(page, points_label, Vector2(72, 820));

	Button *random = ui::small_button("RANDOM", 28);
	random->connect("pressed", callable_mp(this, &NewPilotScreen::on_random));
	place(page, random, Vector2(72, 890), Vector2(220, 64));

	accept = ui::small_button("REPORT FOR DUTY", 28);
	accept->connect("pressed", callable_mp(this, &NewPilotScreen::on_accept));
	place(page, accept, Vector2(490, 890), Vector2(340, 64));

	for (int i = 0; i < SKILL_COUNT; i++) {
		draft.skills[i] = 3;
	}
	points = 8;
	on_random();
	accept->call_deferred("grab_focus");
	audio::play_music("music_diary", -10.0f);
}

void NewPilotScreen::refresh() {
	for (int i = 0; i < SKILL_COUNT; i++) {
		pips[i]->set_text(pips_text(draft.skills[i]));
		minus[i]->set_disabled(draft.skills[i] <= 1);
		plus[i]->set_disabled(points <= 0 || draft.skills[i] >= 8);
	}
	points_label->set_text(points > 0 ? String("POINTS TO ASSIGN: ") + String::num_int64(points)
									  : String("ALL POINTS ASSIGNED"));
	points_label->add_theme_color_override("font_color", points > 0 ? ui::RED : ui::INK);
}

void NewPilotScreen::on_skill(int skill, int delta) {
	int v = draft.skills[skill] + delta;
	if (v < 1 || v > 8) {
		return;
	}
	if (delta > 0 && points <= 0) {
		return;
	}
	draft.skills[skill] = v;
	points -= delta;
	refresh();
}

void NewPilotScreen::on_random() {
	Rng &r = grng();
	const int nf = sizeof(FIRST_NAMES) / sizeof(FIRST_NAMES[0]);
	const int nl = sizeof(LAST_NAMES) / sizeof(LAST_NAMES[0]);
	const int nt = sizeof(TOWNS) / sizeof(TOWNS[0]);
	name_edit->set_text(String(FIRST_NAMES[r.irange(0, nf - 1)]) + String(" ") + String(LAST_NAMES[r.irange(0, nl - 1)]));
	town_edit->set_text(TOWNS[r.irange(0, nt - 1)]);
	for (int i = 0; i < SKILL_COUNT; i++) {
		draft.skills[i] = 3;
	}
	points = 8;
	while (points > 0) {
		int s = r.irange(0, SKILL_COUNT - 1);
		if (draft.skills[s] < 7) {
			draft.skills[s]++;
			points--;
		}
	}
	refresh();
}

void NewPilotScreen::on_accept() {
	GameState &gs = GameState::get();
	String n = name_edit->get_text().strip_edges();
	if (n.is_empty()) {
		n = "John Doe";
	}
	draft.name = n;
	draft.hometown = town_edit->get_text().strip_edges();
	// Unspent points are not lost: spread them evenly.
	int s = 0;
	while (points > 0) {
		if (draft.skills[s % SKILL_COUNT] < 8) {
			draft.skills[s % SKILL_COUNT]++;
			points--;
		}
		s++;
	}
	gs.set_pilot(draft);
	gs.save();
	if (gs.campaign_complete()) {
		main->go(SCREEN_ENDING);
	} else {
		main->go(SCREEN_DIARY);
	}
}

// ---------------------------------------------------------------------------
// Diary
// ---------------------------------------------------------------------------

void DiaryScreen::_ready() {
	add_desk(Color(0.16f, 0.10f, 0.06f), Color(0.05f, 0.03f, 0.02f));
	GameState &gs = GameState::get();
	const MissionDef &m = gs.current_mission();

	Control *page = add_page(Vector2(920, 1010), false, -1.2f, Vector2(0, -10));

	Label *owner = ui::label(String("The diary of ") + pilot_title(gs.pilot), ui::font_hand(), 24,
			Color(0.1f, 0.14f, 0.32f, 0.7f));
	place(page, owner, Vector2(90, 46));

	Label *date = ui::label(m.date, ui::font_hand(), 50, ui::INK_BLUE);
	place(page, date, Vector2(90, 84));
	Label *where = ui::label(m.location, ui::font_hand(), 28, Color(0.1f, 0.14f, 0.32f, 0.8f));
	place(page, where, Vector2(92, 152));

	body = ui::label(String::utf8(m.diary), ui::font_hand(), 33, ui::INK_BLUE);
	body->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	body->add_theme_constant_override("line_spacing", 9);
	body->set_vertical_alignment(VERTICAL_ALIGNMENT_TOP);
	place(page, body, Vector2(90, 220), Vector2(740, 740));
	body->set_visible_characters(0);
	total_chars = String::utf8(m.diary).length();

	next = ui::button("Turn the page", 32, 360);
	next->connect("pressed", callable_mp(this, &DiaryScreen::on_continue));
	add_child(next);
	anchor(next, Control::PRESET_BOTTOM_RIGHT, Vector2(-60, -50), Vector2(360, 70));
	next->call_deferred("grab_focus");

	audio::play_music("music_diary", -10.0f);
	audio::play("page", -4.0f);
	if (gs.demo) {
		reveal = (float)total_chars;
	}
}

void DiaryScreen::_process(double delta) {
	if (reveal < (float)total_chars) {
		reveal += (float)delta * 42.0f;
		body->set_visible_characters((int)reveal);
	} else {
		body->set_visible_characters(-1);
	}
}

void DiaryScreen::on_continue() {
	if (reveal < (float)total_chars) {
		reveal = (float)total_chars;
		return;
	}
	audio::play("page", -4.0f);
	main->go(SCREEN_BRIEFING);
}

// ---------------------------------------------------------------------------
// Briefing
// ---------------------------------------------------------------------------

void BriefingScreen::_ready() {
	add_desk(Color(0.09f, 0.11f, 0.08f), Color(0.025f, 0.03f, 0.025f));
	GameState &gs = GameState::get();
	const MissionDef &m = gs.current_mission();

	Control *page = add_page(Vector2(840, 960), false, 0.7f, Vector2(-480, -20));
	place(page, ui::label("OPERATIONS ORDER", ui::font_type(), 44, ui::INK), Vector2(64, 46));
	place(page, ui::label(String("406th FS  -  ") + String(m.date).to_upper(), ui::font_type(), 22, ui::INK), Vector2(66, 104));
	ColorRect *rule = memnew(ColorRect);
	rule->set_color(ui::INK);
	place(page, rule, Vector2(64, 142), Vector2(712, 3));

	place(page, ui::label(String("MISSION:  ") + String(m.title).to_upper(), ui::font_type(), 28, ui::INK), Vector2(66, 166));
	place(page, ui::label(String("TYPE:     ") + mission_type_name(m.type), ui::font_type(), 28, ui::INK), Vector2(66, 206));
	place(page, ui::label(String("AREA:     ") + String(m.location).to_upper(), ui::font_type(), 28, ui::INK), Vector2(66, 246));

	Label *text = ui::label(String::utf8(m.briefing), ui::font_type(), 26, ui::INK);
	text->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	text->add_theme_constant_override("line_spacing", 6);
	place(page, text, Vector2(66, 320), Vector2(710, 420));

	place(page, ui::label("OBJECTIVE", ui::font_type(), 22, ui::RED), Vector2(66, 770));
	place(page, ui::label(String(m.objective).to_upper(), ui::font_type(), 30, ui::RED), Vector2(66, 800));
	place(page, ui::label(String("PILOT: ") + pilot_title(gs.pilot), ui::font_type(), 22, ui::INK), Vector2(66, 880));

	// Stamp.
	Label *stamp = ui::label(gs.instant_action ? "TRAINING" : "SECRET", ui::font_title(), 64, Color(0.75f, 0.12f, 0.1f, 0.55f));
	place(page, stamp, Vector2(560, 40));
	stamp->set_rotation_degrees(-9.0f);

	Control *map_page = add_page(Vector2(820, 600), false, -1.0f, Vector2(450, -190));
	map = memnew(Canvas);
	map->on_draw = [this](Canvas *c) { draw_map(c); };
	place(map_page, map, Vector2(), Vector2(820, 600));

	PanelContainer *controls = ui::panel(Color(0.02f, 0.03f, 0.03f, 0.8f), Color(0.96f, 0.76f, 0.28f, 0.5f), 2, 26);
	add_child(controls);
	anchor(controls, Control::PRESET_CENTER, Vector2(450, 260), Vector2(820, 230));
	VBoxContainer *vb = memnew(VBoxContainer);
	controls->add_child(vb);
	vb->add_child(ui::label("CONTROLS", ui::font_title(), 26, ui::AMBER));
	Label *ct = ui::label(controls_text(m.type), ui::font_body(), 25, ui::CREAM);
	ct->add_theme_constant_override("line_spacing", 6);
	vb->add_child(ct);

	Button *go = ui::button("Take off", 34, 360);
	go->connect("pressed", callable_mp(this, &BriefingScreen::on_go));
	add_child(go);
	anchor(go, Control::PRESET_BOTTOM_RIGHT, Vector2(-60, -50), Vector2(360, 72));
	go->call_deferred("grab_focus");

	Button *back = ui::button("Back", 30, 220);
	back->connect("pressed", callable_mp(this, &BriefingScreen::on_back));
	add_child(back);
	anchor(back, Control::PRESET_BOTTOM_RIGHT, Vector2(-440, -50), Vector2(220, 72));

	audio::play_music("music_diary", -12.0f);
}

void BriefingScreen::_process(double delta) {
	clock += (float)delta;
	if (map) {
		map->queue_redraw();
	}
}

void BriefingScreen::draw_map(Canvas *c) {
	const MissionDef &m = GameState::get().current_mission();
	const Color sea(0.35f, 0.47f, 0.55f, 0.35f);
	const Color ink(0.15f, 0.13f, 0.12f, 0.9f);
	const Color land(0.45f, 0.5f, 0.3f, 0.18f);

	c->draw_rect(Rect2(20, 20, 780, 560), sea, true);

	// England (top-left) and France (bottom) as loose hand-drawn outlines.
	PackedVector2Array england;
	const float e[][2] = { { 20, 20 }, { 470, 20 }, { 500, 60 }, { 455, 95 }, { 470, 130 }, { 400, 165 }, { 330, 170 },
		{ 250, 195 }, { 150, 185 }, { 80, 210 }, { 20, 200 } };
	for (auto &p : e) {
		england.push_back(Vector2(p[0], p[1]));
	}
	PackedVector2Array france;
	const float f[][2] = { { 800, 120 }, { 700, 150 }, { 640, 215 }, { 620, 300 }, { 540, 345 }, { 450, 340 },
		{ 380, 365 }, { 300, 350 }, { 255, 300 }, { 215, 330 }, { 225, 400 }, { 160, 440 }, { 20, 450 }, { 20, 580 },
		{ 800, 580 } };
	for (auto &p : f) {
		france.push_back(Vector2(p[0], p[1]));
	}
	c->draw_colored_polygon(england, land);
	c->draw_colored_polygon(france, land);
	england.push_back(england[0]);
	france.push_back(france[0]);
	c->draw_polyline(england, ink, 2.5f, true);
	c->draw_polyline(france, ink, 2.5f, true);

	Ref<Font> hand = ui::font_hand();
	c->text(hand, Vector2(90, 110), "ENGLAND", 30, Color(0.15f, 0.13f, 0.12f, 0.55f));
	c->text(hand, Vector2(520, 520), "FRANCE", 34, Color(0.15f, 0.13f, 0.12f, 0.55f));
	c->text(hand, Vector2(330, 262), "English Channel", 24, Color(0.1f, 0.2f, 0.35f, 0.6f));

	// Targets per mission.
	struct Spot {
		float x, y;
	};
	const Spot targets[] = { { 655, 190 }, { 610, 380 }, { 400, 400 }, { 290, 385 }, { 385, 440 }, { 520, 450 } };
	GameState &gs = GameState::get();
	int idx = gs.instant_action ? gs.instant_mission : gs.mission_index;
	idx = CLAMP(idx, 0, 5);
	Vector2 base = idx >= 3 ? Vector2(300, 372) : Vector2(415, 120);
	Vector2 target(targets[idx].x, targets[idx].y);
	if (idx == 3) {
		base = Vector2(330, 368);
		target = Vector2(285, 330);
	}

	c->draw_circle(base, 8.0f, Color(0.1f, 0.14f, 0.32f));
	c->text(hand, base + Vector2(14, -8), idx >= 3 ? "A-6 strip" : "Ashford", 24, ui::INK_BLUE);

	// Animated route.
	Vector2 mid = (base + target) * 0.5f + Vector2(-40, 30);
	const int n = 40;
	float reveal = saturate(clock / 1.6f);
	Vector2 prev = base;
	for (int i = 1; i <= (int)(n * reveal); i++) {
		float t = (float)i / n;
		Vector2 a = base.lerp(mid, t);
		Vector2 b = mid.lerp(target, t);
		Vector2 p = a.lerp(b, t);
		if (i % 2 == 0) {
			c->draw_line(prev, p, ui::RED, 3.5f, true);
		}
		prev = p;
	}
	if (reveal >= 1.0f) {
		float pulse = 1.0f + 0.15f * std::sin(clock * 5.0f);
		float s = 15.0f * pulse;
		c->draw_line(target + Vector2(-s, -s), target + Vector2(s, s), ui::RED, 5.0f, true);
		c->draw_line(target + Vector2(-s, s), target + Vector2(s, -s), ui::RED, 5.0f, true);
		c->draw_arc(target, 28.0f * pulse, 0.0f, TAU_F, 40, ui::RED, 2.5f, true);
		String loc = String(m.location).get_slice(",", 0);
		if (target.x > 560.0f) {
			c->text(hand, target + Vector2(-330, 62), loc, 28, ui::RED, 2, 360);
		} else {
			c->text(hand, target + Vector2(30, 12), loc, 28, ui::RED);
		}
	}

	// Compass rose.
	Vector2 cr(730, 80);
	c->draw_line(cr + Vector2(0, 32), cr + Vector2(0, -32), ink, 2.0f, true);
	c->draw_line(cr + Vector2(-22, 0), cr + Vector2(22, 0), ink, 2.0f, true);
	PackedVector2Array arrow;
	arrow.push_back(cr + Vector2(0, -42));
	arrow.push_back(cr + Vector2(-8, -24));
	arrow.push_back(cr + Vector2(8, -24));
	c->draw_colored_polygon(arrow, ink);
	c->text(hand, cr + Vector2(-9, -46), "N", 24, ink);
}

void BriefingScreen::on_go() {
	audio::stop_music();
	main->go(SCREEN_MISSION, 0.7f);
}

void BriefingScreen::on_back() {
	GameState &gs = GameState::get();
	if (gs.instant_action) {
		gs.instant_action = false;
	}
	main->go(SCREEN_MENU);
}

// ---------------------------------------------------------------------------
// Medals
// ---------------------------------------------------------------------------

void draw_medal(Canvas *c, const Vector2 &centre, float s, const String &name) {
	struct Ribbon {
		Color a, b, c;
	};
	Ribbon rb = { Color(0.1f, 0.2f, 0.5f), Color(0.9f, 0.7f, 0.2f), Color(0.1f, 0.2f, 0.5f) };
	int kind = 0;
	if (name == "Purple Heart") {
		rb = { Color(0.9f, 0.9f, 0.9f), Color(0.42f, 0.16f, 0.5f), Color(0.9f, 0.9f, 0.9f) };
		kind = 1;
	} else if (name == "Distinguished Flying Cross") {
		rb = { Color(0.12f, 0.2f, 0.55f), Color(0.85f, 0.85f, 0.9f), Color(0.7f, 0.12f, 0.12f) };
		kind = 2;
	} else if (name == "Silver Star") {
		rb = { Color(0.12f, 0.2f, 0.55f), Color(0.9f, 0.9f, 0.9f), Color(0.7f, 0.12f, 0.12f) };
		kind = 3;
	} else if (name == "Distinguished Unit Citation") {
		rb = { Color(0.1f, 0.2f, 0.55f), Color(0.1f, 0.2f, 0.55f), Color(0.1f, 0.2f, 0.55f) };
		kind = 4;
	}
	const Color gold(0.85f, 0.66f, 0.22f);
	const Color gold_dark(0.55f, 0.40f, 0.12f);
	const Color silver(0.82f, 0.84f, 0.88f);

	float rw = 44.0f * s;
	float rh = 60.0f * s;
	Vector2 top = centre + Vector2(-rw * 0.5f, -rh - 20.0f * s);
	if (kind == 4) {
		c->draw_rect(Rect2(centre + Vector2(-rw, -16.0f * s), Vector2(rw * 2.0f, 32.0f * s)), gold, true);
		c->draw_rect(Rect2(centre + Vector2(-rw + 5 * s, -11.0f * s), Vector2(rw * 2.0f - 10 * s, 22.0f * s)), rb.a, true);
		return;
	}
	c->draw_rect(Rect2(top, Vector2(rw, rh)), rb.a, true);
	c->draw_rect(Rect2(top + Vector2(rw * 0.3f, 0), Vector2(rw * 0.4f, rh)), rb.b, true);
	c->draw_rect(Rect2(top + Vector2(rw * 0.7f, 0), Vector2(rw * 0.3f, rh)), rb.c, true);
	c->draw_rect(Rect2(top, Vector2(rw, rh)), Color(0, 0, 0, 0.35f), false, 1.5f);
	c->draw_rect(Rect2(top + Vector2(-3 * s, -5 * s), Vector2(rw + 6 * s, 7 * s)), gold_dark, true);

	Vector2 p = centre + Vector2(0, 22.0f * s);
	float r = 30.0f * s;
	if (kind == 0) {
		c->draw_circle(p, r, gold_dark);
		c->draw_circle(p, r * 0.86f, gold);
		// Eagle suggestion: wings.
		PackedVector2Array w;
		w.push_back(p + Vector2(-r * 0.7f, -r * 0.1f));
		w.push_back(p + Vector2(0, r * 0.25f));
		w.push_back(p + Vector2(r * 0.7f, -r * 0.1f));
		w.push_back(p + Vector2(0, r * 0.05f));
		c->draw_colored_polygon(w, gold_dark);
	} else if (kind == 1) {
		PackedVector2Array h;
		const int n = 28;
		for (int i = 0; i < n; i++) {
			float t = TAU_F * i / n;
			float x = 16.0f * std::pow(std::sin(t), 3.0f);
			float y = -(13.0f * std::cos(t) - 5.0f * std::cos(2 * t) - 2.0f * std::cos(3 * t) - std::cos(4 * t));
			h.push_back(p + Vector2(x, y) * (r / 16.0f));
		}
		c->draw_colored_polygon(h, gold);
		PackedVector2Array h2;
		for (int i = 0; i < n; i++) {
			h2.push_back(p + (h[i] - p) * 0.8f);
		}
		c->draw_colored_polygon(h2, Color(0.42f, 0.16f, 0.5f));
		c->draw_circle(p + Vector2(0, -2 * s), r * 0.22f, gold);
	} else if (kind == 2) {
		// Cross pattee with a propeller.
		for (int k = 0; k < 4; k++) {
			float a = k * PI_F * 0.5f;
			Vector2 d(std::cos(a), std::sin(a));
			Vector2 n2(-d.y, d.x);
			PackedVector2Array arm;
			arm.push_back(p + d * r * 0.15f + n2 * r * 0.12f);
			arm.push_back(p + d * r * 1.05f + n2 * r * 0.42f);
			arm.push_back(p + d * r * 1.05f - n2 * r * 0.42f);
			arm.push_back(p + d * r * 0.15f - n2 * r * 0.12f);
			c->draw_colored_polygon(arm, Color(0.6f, 0.42f, 0.2f));
		}
		c->draw_line(p + Vector2(-r, -r) * 0.6f, p + Vector2(r, r) * 0.6f, gold, 5.0f * s, true);
		c->draw_line(p + Vector2(-r, r) * 0.6f, p + Vector2(r, -r) * 0.6f, gold, 5.0f * s, true);
		c->draw_circle(p, r * 0.16f, gold_dark);
	} else {
		for (int pass = 0; pass < 2; pass++) {
			PackedVector2Array star;
			float rr = pass == 0 ? r * 1.1f : r * 0.35f;
			for (int i = 0; i < 10; i++) {
				float a = -PI_F * 0.5f + TAU_F * i / 10.0f;
				float rad = (i % 2 == 0) ? rr : rr * 0.4f;
				star.push_back(p + Vector2(std::cos(a), std::sin(a)) * rad);
			}
			c->draw_colored_polygon(star, pass == 0 ? gold : silver);
		}
	}
}

// ---------------------------------------------------------------------------
// Debrief
// ---------------------------------------------------------------------------

void DebriefScreen::_ready() {
	add_desk(Color(0.09f, 0.10f, 0.11f), Color(0.025f, 0.03f, 0.035f));
	GameState &gs = GameState::get();
	const MissionResult &r = gs.last;
	const MissionDef &m = gs.current_mission();

	Control *page = add_page(Vector2(900, 1000), false, 0.5f, Vector2(-300, 0));
	place(page, ui::label("COMBAT REPORT", ui::font_type(), 48, ui::INK), Vector2(64, 46));
	place(page, ui::label(String(m.title).to_upper() + String("  -  ") + String(m.date).to_upper(), ui::font_type(), 22, ui::INK),
			Vector2(66, 108));
	ColorRect *rule = memnew(ColorRect);
	rule->set_color(ui::INK);
	place(page, rule, Vector2(64, 146), Vector2(770, 3));

	struct Row {
		String k, v;
	};
	std::vector<Row> rows;
	rows.push_back({ "PILOT", pilot_title(gs.pilot) });
	if (r.type == MISSION_DOGFIGHT) {
		rows.push_back({ "ENEMY AIRCRAFT DESTROYED", String::num_int64(r.air_kills) });
	} else {
		rows.push_back({ "GROUND TARGETS DESTROYED", String::num_int64(r.targets_destroyed) + String(" of ") +
															   String::num_int64(r.targets_total) });
		if (r.air_kills > 0) {
			rows.push_back({ "ENEMY AIRCRAFT DESTROYED", String::num_int64(r.air_kills) });
		}
	}
	if (r.shots_fired > 0) {
		int pct = (int)std::round(100.0f * r.shots_hit / MAX(r.shots_fired, 1));
		rows.push_back({ r.type == MISSION_BOMBING ? "BOMBS ON TARGET" : "GUNNERY",
				r.type == MISSION_BOMBING
						? String::num_int64(r.shots_hit) + String(" of ") + String::num_int64(r.shots_fired)
						: String::num_int64(pct) + String("% hits") });
	}
	int mins = (int)(r.duration / 60.0f);
	int secs = (int)r.duration % 60;
	rows.push_back({ "TIME OVER TARGET", String::num_int64(mins) + String(" min ") + String::num_int64(secs) + String(" sec") });
	String status = "Returned safely";
	if (r.killed) {
		status = "KILLED IN ACTION";
	} else if (r.wounded) {
		status = "Shot down - wounded, recovered";
	} else if (r.shot_down) {
		status = "Shot down - returned on foot";
	} else if (r.aborted) {
		status = "Aborted the mission";
	}
	rows.push_back({ "PILOT STATUS", status });
	rows.push_back({ "SCORE", String::num_int64(r.score) });

	float y = 180.0f;
	for (const Row &row : rows) {
		place(page, ui::label(row.k, ui::font_type(), 22, Color(0.13f, 0.12f, 0.16f, 0.75f)), Vector2(66, y));
		place(page, ui::label(row.v, ui::font_type(), 30, ui::INK_BLUE), Vector2(66, y + 26));
		y += 78.0f;
	}

	if (!r.cause.is_empty()) {
		Label *cause = ui::label(r.cause, ui::font_hand(), 28, ui::INK_BLUE);
		cause->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		place(page, cause, Vector2(66, y), Vector2(760, 80));
		y += 70.0f;
	}

	// Stamp.
	stamp = memnew(Canvas);
	String stamp_text = r.success ? "MISSION ACCOMPLISHED" : (r.killed ? "KILLED IN ACTION" : "MISSION FAILED");
	Color stamp_col = r.success ? Color(0.1f, 0.35f, 0.15f, 0.7f) : Color(0.7f, 0.1f, 0.08f, 0.7f);
	stamp->on_draw = [this, stamp_text, stamp_col](Canvas *c) {
		float k = saturate((clock - 0.6f) / 0.25f);
		if (k <= 0.0f) {
			return;
		}
		float scale = lerpf(2.2f, 1.0f, smooth01(k));
		c->draw_set_transform(Vector2(260, 60), deg2rad(-8.0f), Vector2(scale, scale));
		Color col = stamp_col;
		col.a *= k;
		c->draw_rect(Rect2(-250, -48, 500, 96), col, false, 6.0f);
		c->text(ui::font_title(), Vector2(-250, 22), stamp_text, 54, col, 1, 500);
	};
	place(page, stamp, Vector2(330, 820), Vector2(520, 120));

	// Right column: awards.
	PanelContainer *side = ui::panel(Color(0.02f, 0.03f, 0.03f, 0.78f), Color(0.96f, 0.76f, 0.28f, 0.5f), 2, 30);
	add_child(side);
	anchor(side, Control::PRESET_CENTER, Vector2(520, -60), Vector2(640, 760));
	VBoxContainer *vb = memnew(VBoxContainer);
	vb->add_theme_constant_override("separation", 14);
	side->add_child(vb);

	vb->add_child(ui::label("SERVICE RECORD", ui::font_title(), 34, ui::AMBER));
	vb->add_child(ui::label(pilot_title(gs.pilot), ui::font_title(), 44, ui::CREAM));
	String tally = String::num_int64(gs.pilot.air_kills) + String(" air victories   ") +
			String::num_int64(gs.pilot.ground_kills) + String(" ground targets   ") +
			String::num_int64(gs.pilot.missions) + String(gs.pilot.missions == 1 ? " mission" : " missions");
	vb->add_child(ui::label(tally, ui::font_body(), 24, ui::CREAM));

	if (gs.instant_action) {
		Label *l = ui::label("Instant action - results are not recorded.", ui::font_body(), 24, Color(1, 1, 1, 0.6f));
		vb->add_child(l);
	}
	if (r.promoted) {
		vb->add_child(ui::label(String("PROMOTED to ") + rank_name(gs.pilot.rank), ui::font_title(), 34, ui::AMBER));
	}
	if (r.skill_gained >= 0) {
		vb->add_child(ui::label(String("Experience gained: ") + skill_name(r.skill_gained) + String(" +1"), ui::font_body(), 26,
				Color(0.7f, 0.9f, 0.7f)));
	}

	PackedStringArray medals = r.new_medals;
	if (medals.size() > 0) {
		vb->add_child(ui::label("DECORATIONS AWARDED", ui::font_title(), 26, ui::AMBER));
		Canvas *mc = memnew(Canvas);
		mc->set_custom_minimum_size(Vector2(560, 230));
		mc->on_draw = [medals](Canvas *c) {
			for (int i = 0; i < medals.size(); i++) {
				Vector2 p(80.0f + i * 190.0f, 110.0f);
				draw_medal(c, p, 1.25f, medals[i]);
				String caption = medals[i];
				if (caption == String("Distinguished Unit Citation")) {
					caption = "Unit Citation";
				} else if (caption == String("Distinguished Flying Cross")) {
					caption = "Flying Cross";
				}
				c->text(ui::font_body(), Vector2(p.x - 95.0f, 215.0f), caption, 19, ui::CREAM, 1, 190.0f);
			}
		};
		vb->add_child(mc);
	}

	for (int i = 0; i < SKILL_COUNT; i++) {
		HBoxContainer *row = memnew(HBoxContainer);
		Label *n = ui::label(String(skill_name(i)).to_upper(), ui::font_title(), 24, Color(1, 1, 1, 0.7f));
		n->set_custom_minimum_size(Vector2(190, 0));
		row->add_child(n);
		row->add_child(ui::label(pips_text(gs.pilot.skills[i]), ui::font_body(), 24,
				i == r.skill_gained ? ui::AMBER : ui::CREAM));
		vb->add_child(row);
	}

	Button *next = ui::button("Continue", 34, 360);
	next->connect("pressed", callable_mp(this, &DebriefScreen::on_continue));
	add_child(next);
	anchor(next, Control::PRESET_BOTTOM_RIGHT, Vector2(-60, -50), Vector2(360, 72));
	next->call_deferred("grab_focus");

	if (r.killed) {
		audio::play("dirge", -4.0f);
	} else if (r.success) {
		audio::play("fanfare", -5.0f);
	}
	audio::play_music("music_diary", -14.0f);
}

void DebriefScreen::_process(double delta) {
	float before = clock;
	clock += (float)delta;
	if (before < 0.8f && clock >= 0.8f) {
		audio::play("bomb_release", -2.0f, 0.7f);
	}
	if (stamp && clock < 2.0f) {
		stamp->queue_redraw();
	}
}

void DebriefScreen::on_continue() {
	GameState &gs = GameState::get();
	if (gs.instant_action) {
		gs.instant_action = false;
		main->go(SCREEN_MENU);
		return;
	}
	if (gs.last.killed) {
		main->go(SCREEN_MEMORIAL, 1.2f);
		return;
	}
	gs.advance();
	gs.save();
	main->go(gs.campaign_complete() ? SCREEN_ENDING : SCREEN_DIARY);
}

// ---------------------------------------------------------------------------
// Memorial
// ---------------------------------------------------------------------------

void MemorialScreen::_ready() {
	ui::fill(this);
	GameState &gs = GameState::get();

	Backdrop *bd = memnew(Backdrop);
	bd->set_time(TOD_OVERCAST);
	bd->set_quiet(true);
	add_child(bd);
	ColorRect *dim = memnew(ColorRect);
	dim->set_color(Color(0.02f, 0.02f, 0.03f, 0.72f));
	add_child(dim);
	ui::fill(dim);
	add_child(ui::vignette(0.9f));

	VBoxContainer *vb = memnew(VBoxContainer);
	vb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	vb->add_theme_constant_override("separation", 16);
	add_child(vb);
	ui::fill(vb);

	auto centred = [&](Label *l) {
		l->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		vb->add_child(l);
	};

	Fallen f;
	if (!gs.fallen.empty()) {
		f = gs.fallen.back();
	} else {
		f.name = gs.pilot.name;
		f.rank = rank_name(gs.pilot.rank);
	}

	centred(ui::label("IN MEMORIAM", ui::font_title(), 40, ui::AMBER));
	centred(ui::label(f.rank + String(" ") + f.name, ui::font_title(), 110, ui::CREAM));
	centred(ui::label(String("406th Fighter Squadron  -  ") + f.date, ui::font_body(), 34, ui::CREAM));
	centred(ui::label(f.cause, ui::font_hand(), 36, Color(1, 1, 1, 0.8f)));
	centred(ui::label(String::num_int64(f.missions) + String(f.missions == 1 ? " mission flown     " : " missions flown     ") + String::num_int64(f.air_kills) +
							  String(" aerial victories"),
			ui::font_body(), 30, Color(1, 1, 1, 0.7f)));
	Control *gap = memnew(Control);
	gap->set_custom_minimum_size(Vector2(0, 40));
	vb->add_child(gap);
	centred(ui::label("The squadron flies again at dawn. A replacement is already on his way.", ui::font_hand(), 32,
			Color(1, 1, 1, 0.7f)));

	Button *next = ui::button("A new pilot arrives", 34, 440);
	next->connect("pressed", callable_mp(this, &MemorialScreen::on_continue));
	add_child(next);
	anchor(next, Control::PRESET_BOTTOM_RIGHT, Vector2(-60, -50), Vector2(440, 72));
	next->call_deferred("grab_focus");

	audio::stop_music();
	audio::play("dirge", -3.0f);
}

void MemorialScreen::on_continue() {
	GameState &gs = GameState::get();
	gs.advance();
	gs.pilot = Pilot();
	gs.has_pilot = false;
	gs.save();
	main->go(SCREEN_NEW_PILOT);
}

// ---------------------------------------------------------------------------
// Roster
// ---------------------------------------------------------------------------

void RosterScreen::_ready() {
	add_desk(Color(0.09f, 0.10f, 0.11f), Color(0.025f, 0.03f, 0.035f));
	GameState &gs = GameState::get();

	Control *page = add_page(Vector2(1100, 960), false, 0.0f, Vector2(0, -30));
	place(page, ui::label("SQUADRON ROSTER", ui::font_type(), 48, ui::INK), Vector2(64, 46));
	place(page, ui::label("406th FIGHTER SQUADRON", ui::font_type(), 22, ui::INK), Vector2(66, 108));
	ColorRect *rule = memnew(ColorRect);
	rule->set_color(ui::INK);
	place(page, rule, Vector2(64, 146), Vector2(970, 3));

	float y = 176.0f;
	if (gs.has_pilot) {
		place(page, ui::label("ON ACTIVE DUTY", ui::font_type(), 22, Color(0.1f, 0.35f, 0.15f)), Vector2(66, y));
		y += 30.0f;
		place(page, ui::label(pilot_title(gs.pilot) + String("  -  ") + gs.pilot.hometown, ui::font_type(), 32, ui::INK_BLUE),
				Vector2(66, y));
		y += 44.0f;
		String line = String::num_int64(gs.pilot.missions) + String(" missions, ") +
				String::num_int64(gs.pilot.air_kills) + String(" air victories, ") +
				String::num_int64(gs.pilot.ground_kills) + String(" ground targets, score ") +
				String::num_int64(gs.pilot.score);
		place(page, ui::label(line, ui::font_type(), 22, ui::INK), Vector2(66, y));
		y += 40.0f;
		PackedStringArray medals = gs.pilot.medals;
		if (medals.size() > 0) {
			Canvas *mc = memnew(Canvas);
			mc->on_draw = [medals](Canvas *c) {
				for (int i = 0; i < medals.size(); i++) {
					draw_medal(c, Vector2(60.0f + i * 110.0f, 90.0f), 0.85f, medals[i]);
				}
			};
			place(page, mc, Vector2(66, y), Vector2(900, 150));
			y += 160.0f;
		}
	}
	y += 20.0f;
	place(page, ui::label("KILLED IN ACTION", ui::font_type(), 22, ui::RED), Vector2(66, y));
	y += 36.0f;
	if (gs.fallen.empty()) {
		place(page, ui::label("None. Keep it that way.", ui::font_hand(), 30, ui::INK_BLUE), Vector2(66, y));
	}
	int shown = 0;
	for (auto it = gs.fallen.rbegin(); it != gs.fallen.rend() && shown < 8; ++it, ++shown) {
		String line = String::utf8("✝  ") + it->rank + String(" ") + it->name + String("  -  ") + it->date +
				String("  -  ") + String::num_int64(it->air_kills) + String(" victories");
		place(page, ui::label(line, ui::font_type(), 24, ui::INK), Vector2(66, y));
		y += 38.0f;
	}

	Button *back = ui::button("Back", 34, 300);
	back->connect("pressed", callable_mp(this, &RosterScreen::on_back));
	add_child(back);
	anchor(back, Control::PRESET_BOTTOM_RIGHT, Vector2(-60, -50), Vector2(300, 72));
	back->call_deferred("grab_focus");
}

void RosterScreen::on_back() {
	main->go(SCREEN_MENU);
}

// ---------------------------------------------------------------------------
// Ending
// ---------------------------------------------------------------------------

void EndingScreen::_ready() {
	ui::fill(this);
	GameState &gs = GameState::get();

	Backdrop *bd = memnew(Backdrop);
	bd->set_time(TOD_MORNING);
	add_child(bd);
	ColorRect *dim = memnew(ColorRect);
	dim->set_color(Color(0.02f, 0.02f, 0.03f, 0.5f));
	add_child(dim);
	ui::fill(dim);
	add_child(ui::vignette(0.8f));

	VBoxContainer *vb = memnew(VBoxContainer);
	vb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	vb->add_theme_constant_override("separation", 18);
	add_child(vb);
	ui::fill(vb);
	auto centred = [&](Label *l) {
		l->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		vb->add_child(l);
	};
	centred(ui::label("JUNE 1944", ui::font_title(), 40, ui::AMBER));
	centred(ui::label("THE BEACHHEAD HOLDS", ui::font_title(), 120, ui::CREAM));
	centred(ui::label(pilot_title(gs.pilot), ui::font_title(), 54, ui::CREAM));
	centred(ui::label(String::num_int64(gs.pilot.missions) + String(" missions     ") +
							  String::num_int64(gs.pilot.air_kills) + String(" aerial victories     ") +
							  String::num_int64(gs.pilot.ground_kills) + String(" ground targets"),
			ui::font_body(), 32, ui::CREAM));
	if (!gs.fallen.empty()) {
		centred(ui::label(String::num_int64((int64_t)gs.fallen.size()) + String(" of the squadron did not come home."),
				ui::font_hand(), 32, Color(1, 1, 1, 0.75f)));
	}
	Control *gap = memnew(Control);
	gap->set_custom_minimum_size(Vector2(0, 30));
	vb->add_child(gap);
	centred(ui::label("The war goes on. This is the end of the vertical slice - thank you for flying.", ui::font_hand(),
			32, Color(1, 1, 1, 0.8f)));

	Button *back = ui::button("Main menu", 34, 340);
	back->connect("pressed", callable_mp(this, &EndingScreen::on_back));
	add_child(back);
	anchor(back, Control::PRESET_BOTTOM_RIGHT, Vector2(-60, -50), Vector2(340, 72));
	back->call_deferred("grab_focus");
	audio::play_music("music_menu", -9.0f);
	audio::play("fanfare", -6.0f);
}

void EndingScreen::on_back() {
	main->go(SCREEN_MENU);
}

} // namespace ww2
