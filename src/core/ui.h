#pragma once

#include "core/util.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

#include <functional>

namespace ww2 {

// A control whose drawing is supplied by a callback; used for HUDs, maps and medals.
class Canvas : public godot::Control {
	GDCLASS(Canvas, godot::Control)

public:
	std::function<void(Canvas *)> on_draw;
	void _draw() override;

	// Convenience wrappers.
	void text(const godot::Ref<godot::Font> &font, const Vector2 &pos, const String &s, int size, const Color &color,
			int align = 0, float width = -1.0f);
	void text_shadowed(const godot::Ref<godot::Font> &font, const Vector2 &pos, const String &s, int size,
			const Color &color, int align = 0, float width = -1.0f);

protected:
	static void _bind_methods() {}
};

namespace ui {

using godot::Button;
using godot::ColorRect;
using godot::Control;
using godot::Font;
using godot::Label;
using godot::Ref;

const Color AMBER(0.96f, 0.76f, 0.28f);
const Color CREAM(0.93f, 0.90f, 0.80f);
const Color INK(0.13f, 0.12f, 0.16f);
const Color INK_BLUE(0.10f, 0.14f, 0.32f);
const Color RED(0.75f, 0.16f, 0.12f);
const Color OLIVE(0.20f, 0.23f, 0.15f);
const Color DARK(0.04f, 0.05f, 0.05f);

Ref<Font> font_title(); // condensed bold
Ref<Font> font_body();
Ref<Font> font_hand(); // diary handwriting
Ref<Font> font_type(); // typewriter

Label *label(const String &text, const Ref<Font> &font, int size, const Color &color);
godot::RichTextLabel *rich(const String &bbcode, const Ref<Font> &font, int size, const Color &color);
Button *button(const String &text, int font_size = 34, float min_width = 420.0f);
Button *small_button(const String &text, int font_size = 26);
ColorRect *paper(bool ruled);
ColorRect *vignette(float strength);
godot::PanelContainer *panel(const Color &bg, const Color &border, int border_width = 2, int margin = 24);
Ref<godot::StyleBoxFlat> flat(const Color &bg, const Color &border, int border_width, int margin, int radius = 0);
void fill(Control *c); // anchor to the full parent rect

void clear_cache();

} // namespace ui
} // namespace ww2
