#pragma once

#include "core/ui.h"
#include "game/game_state.h"
#include "game/main.h"

#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ww2 {

class ScreenBase : public godot::Control {
	GDCLASS(ScreenBase, godot::Control)

public:
	void set_main(Main *m) { main = m; }
	// Performs the screen's default action; used by the automated tour.
	virtual void auto_advance() {}

protected:
	static void _bind_methods();
	Main *main = nullptr;

	// Adds a dark desk-like background with a vignette.
	void add_desk(const Color &top, const Color &bottom);
	// Fixed-size page centred on screen; children use absolute positions.
	godot::Control *add_page(const Vector2 &size, bool ruled, float tilt_deg, const Vector2 &offset = Vector2());
};

class MenuScreen : public ScreenBase {
	GDCLASS(MenuScreen, ScreenBase)

public:
	void _ready() override;
	void auto_advance() override { on_new(); }

protected:
	static void _bind_methods() {}

private:
	void on_continue();
	void on_new();
	void on_instant(int mission);
	void on_roster();
	void on_quit();
};

class NewPilotScreen : public ScreenBase {
	GDCLASS(NewPilotScreen, ScreenBase)

public:
	void _ready() override;
	void auto_advance() override { on_accept(); }

protected:
	static void _bind_methods() {}

private:
	void refresh();
	void on_skill(int skill, int delta);
	void on_random();
	void on_accept();

	Pilot draft;
	int points = 8;
	godot::LineEdit *name_edit = nullptr;
	godot::LineEdit *town_edit = nullptr;
	godot::Label *pips[SKILL_COUNT] = {};
	godot::Button *minus[SKILL_COUNT] = {};
	godot::Button *plus[SKILL_COUNT] = {};
	godot::Label *points_label = nullptr;
	godot::Button *accept = nullptr;
};

class DiaryScreen : public ScreenBase {
	GDCLASS(DiaryScreen, ScreenBase)

public:
	void _ready() override;
	void auto_advance() override { on_continue(); }
	void _process(double delta) override;

protected:
	static void _bind_methods() {}

private:
	void on_continue();

	godot::Label *body = nullptr;
	godot::Button *next = nullptr;
	float reveal = 0.0f;
	int total_chars = 0;
	float scratch = 0.0f;
};

class BriefingScreen : public ScreenBase {
	GDCLASS(BriefingScreen, ScreenBase)

public:
	void _ready() override;
	void auto_advance() override { on_go(); }
	void _process(double delta) override;

protected:
	static void _bind_methods() {}

private:
	void on_go();
	void on_back();
	void draw_map(Canvas *c);

	Canvas *map = nullptr;
	float clock = 0.0f;
};

class DebriefScreen : public ScreenBase {
	GDCLASS(DebriefScreen, ScreenBase)

public:
	void _ready() override;
	void auto_advance() override { on_continue(); }
	void _process(double delta) override;

protected:
	static void _bind_methods() {}

private:
	void on_continue();
	float clock = 0.0f;
	Canvas *stamp = nullptr;
};

class MemorialScreen : public ScreenBase {
	GDCLASS(MemorialScreen, ScreenBase)

public:
	void _ready() override;
	void auto_advance() override { on_continue(); }

protected:
	static void _bind_methods() {}

private:
	void on_continue();
};

class RosterScreen : public ScreenBase {
	GDCLASS(RosterScreen, ScreenBase)

public:
	void _ready() override;

protected:
	static void _bind_methods() {}

private:
	void on_back();
};

class EndingScreen : public ScreenBase {
	GDCLASS(EndingScreen, ScreenBase)

public:
	void _ready() override;

protected:
	static void _bind_methods() {}

private:
	void on_back();
};

// Draws a medal (ribbon + pendant) for the named award.
void draw_medal(Canvas *c, const Vector2 &centre, float scale, const String &name);

} // namespace ww2
