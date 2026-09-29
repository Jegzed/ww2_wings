#include "core/ui.h"

#include "core/audio.h"
#include "core/materials.h"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/system_font.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

namespace ww2 {

void Canvas::_draw() {
	if (on_draw) {
		on_draw(this);
	}
}

void Canvas::text(const Ref<Font> &font, const Vector2 &pos, const String &s, int size, const Color &color, int align,
		float width) {
	HorizontalAlignment h = align == 1 ? HORIZONTAL_ALIGNMENT_CENTER
									   : (align == 2 ? HORIZONTAL_ALIGNMENT_RIGHT : HORIZONTAL_ALIGNMENT_LEFT);
	draw_string(font, pos, s, h, width, size, color);
}

void Canvas::text_shadowed(const Ref<Font> &font, const Vector2 &pos, const String &s, int size, const Color &color,
		int align, float width) {
	text(font, pos + Vector2(2, 2), s, size, Color(0, 0, 0, 0.7f * color.a), align, width);
	text(font, pos, s, size, color, align, width);
}

namespace ui {

namespace {
struct Cache {
	Ref<Font> title, body, hand, type;
};
Cache *cache = nullptr;

Cache &c() {
	if (!cache) {
		cache = new Cache();
	}
	return *cache;
}

Ref<Font> system_font(const char *const *names, int count, int weight, bool italic = false, int stretch = 100) {
	Ref<SystemFont> f;
	f.instantiate();
	PackedStringArray arr;
	for (int i = 0; i < count; i++) {
		arr.push_back(names[i]);
	}
	f->set_font_names(arr);
	f->set_font_weight(weight);
	f->set_font_italic(italic);
	f->set_font_stretch(stretch);
	f->set_antialiasing(TextServer::FONT_ANTIALIASING_GRAY);
	f->set_generate_mipmaps(true);
	return f;
}
} // namespace

void clear_cache() {
	delete cache;
	cache = nullptr;
}

Ref<Font> font_title() {
	if (c().title.is_null()) {
		static const char *names[] = { "Bahnschrift", "Arial Narrow", "Impact", "Arial", "sans-serif" };
		c().title = system_font(names, 5, 700, false, 75);
	}
	return c().title;
}

Ref<Font> font_body() {
	if (c().body.is_null()) {
		static const char *names[] = { "Bahnschrift", "Segoe UI", "Arial", "sans-serif" };
		c().body = system_font(names, 4, 400, false, 90);
	}
	return c().body;
}

Ref<Font> font_hand() {
	if (c().hand.is_null()) {
		static const char *names[] = { "Ink Free", "Segoe Print", "Segoe Script", "Comic Sans MS", "cursive" };
		c().hand = system_font(names, 5, 400);
	}
	return c().hand;
}

Ref<Font> font_type() {
	if (c().type.is_null()) {
		static const char *names[] = { "Courier New", "Consolas", "Lucida Console", "monospace" };
		c().type = system_font(names, 4, 700);
	}
	return c().type;
}

void fill(Control *ctl) {
	ctl->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
}

Label *label(const String &text, const Ref<Font> &font, int size, const Color &color) {
	Label *l = memnew(Label);
	l->set_text(text);
	l->add_theme_font_override("font", font);
	l->add_theme_font_size_override("font_size", size);
	l->add_theme_color_override("font_color", color);
	return l;
}

RichTextLabel *rich(const String &bbcode, const Ref<Font> &font, int size, const Color &color) {
	RichTextLabel *r = memnew(RichTextLabel);
	r->set_use_bbcode(true);
	r->set_text(bbcode);
	r->set_fit_content(true);
	r->set_scroll_active(false);
	r->add_theme_font_override("normal_font", font);
	r->add_theme_font_override("bold_font", font);
	r->add_theme_font_size_override("normal_font_size", size);
	r->add_theme_font_size_override("bold_font_size", size);
	r->add_theme_color_override("default_color", color);
	r->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	return r;
}

Ref<StyleBoxFlat> flat(const Color &bg, const Color &border, int border_width, int margin, int radius) {
	Ref<StyleBoxFlat> s;
	s.instantiate();
	s->set_bg_color(bg);
	s->set_border_color(border);
	s->set_border_width_all(border_width);
	s->set_content_margin_all((float)margin);
	s->set_corner_radius_all(radius);
	s->set_anti_aliased(true);
	return s;
}

namespace {
void on_button_hover() {
	audio::play("type", -14.0f, 0.8f);
}
void on_button_press() {
	audio::play("click", -6.0f, 1.0f);
}

void style_button(Button *b, int font_size, bool small) {
	b->add_theme_font_override("font", font_title());
	b->add_theme_font_size_override("font_size", font_size);
	b->add_theme_color_override("font_color", CREAM);
	b->add_theme_color_override("font_hover_color", Color(1, 1, 1));
	b->add_theme_color_override("font_focus_color", Color(1, 1, 1));
	b->add_theme_color_override("font_pressed_color", AMBER);
	b->add_theme_color_override("font_disabled_color", Color(0.5, 0.5, 0.48, 0.6));

	const int m = small ? 10 : 16;
	Ref<StyleBoxFlat> normal = flat(Color(0.03, 0.04, 0.04, 0.62), Color(0.96, 0.76, 0.28, 0.0), 0, m);
	normal->set_border_width(SIDE_LEFT, 5);
	normal->set_border_color(Color(0.96, 0.76, 0.28, 0.55));
	normal->set_content_margin(SIDE_LEFT, small ? 18.0f : 30.0f);
	normal->set_content_margin(SIDE_RIGHT, small ? 18.0f : 30.0f);

	Ref<StyleBoxFlat> hover = normal->duplicate();
	hover->set_bg_color(Color(0.96, 0.76, 0.28, 0.22));
	hover->set_border_color(AMBER);
	hover->set_border_width(SIDE_LEFT, 10);

	Ref<StyleBoxFlat> pressed = normal->duplicate();
	pressed->set_bg_color(Color(0.96, 0.76, 0.28, 0.4));
	pressed->set_border_color(Color(1, 1, 1));

	Ref<StyleBoxFlat> disabled = normal->duplicate();
	disabled->set_bg_color(Color(0.03, 0.04, 0.04, 0.35));
	disabled->set_border_color(Color(0.5, 0.5, 0.5, 0.3));

	b->add_theme_stylebox_override("normal", normal);
	b->add_theme_stylebox_override("hover", hover);
	b->add_theme_stylebox_override("focus", hover);
	b->add_theme_stylebox_override("pressed", pressed);
	b->add_theme_stylebox_override("disabled", disabled);
	b->set_text_alignment(small ? HORIZONTAL_ALIGNMENT_CENTER : HORIZONTAL_ALIGNMENT_LEFT);
	b->set_focus_mode(Control::FOCUS_ALL);
	b->connect("mouse_entered", callable_mp_static(&on_button_hover));
	b->connect("focus_entered", callable_mp_static(&on_button_hover));
	b->connect("pressed", callable_mp_static(&on_button_press));
}
} // namespace

Button *button(const String &text, int font_size, float min_width) {
	Button *b = memnew(Button);
	b->set_text(text.to_upper());
	b->set_custom_minimum_size(Vector2(min_width, 0));
	style_button(b, font_size, false);
	return b;
}

Button *small_button(const String &text, int font_size) {
	Button *b = memnew(Button);
	b->set_text(text);
	style_button(b, font_size, true);
	return b;
}

ColorRect *paper(bool ruled) {
	ColorRect *r = memnew(ColorRect);
	Ref<ShaderMaterial> m;
	m.instantiate();
	m->set_shader(mats::canvas_shader("paper"));
	m->set_shader_parameter("lines", ruled ? 1.0f : 0.0f);
	r->set_material(m);
	r->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	return r;
}

ColorRect *vignette(float strength) {
	ColorRect *r = memnew(ColorRect);
	Ref<ShaderMaterial> m;
	m.instantiate();
	m->set_shader(mats::canvas_shader("vignette"));
	m->set_shader_parameter("strength", strength);
	r->set_material(m);
	r->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	fill(r);
	return r;
}

PanelContainer *panel(const Color &bg, const Color &border, int border_width, int margin) {
	PanelContainer *p = memnew(PanelContainer);
	p->add_theme_stylebox_override("panel", flat(bg, border, border_width, margin));
	return p;
}

} // namespace ui
} // namespace ww2
