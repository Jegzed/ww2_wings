#pragma once

#include "core/util.h"

#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace ww2 {
namespace mats {

using godot::Ref;
using godot::Shader;
using godot::ShaderMaterial;
using godot::StandardMaterial3D;
using godot::Texture2D;

enum PaintScheme {
	PAINT_P47_SILVER, // bare metal, red cowl, invasion stripes (player)
	PAINT_P47_OLIVE, // olive drab wingmen
	PAINT_BF109_GREY, // grey mottled, yellow nose
	PAINT_BF109_ACE, // darker, red nose
	PAINT_FW190, // grey-green, yellow undercowl
	PAINT_B26, // olive drab Marauder
	PAINT_JU88, // splinter green bomber
	PAINT_V1, // plain grey flying bomb
};

// Each call returns a fresh material so per-plane damage can be animated.
Ref<ShaderMaterial> plane_paint(PaintScheme scheme);

Ref<StandardMaterial3D> vertex_color(float rough = 0.85f, float metal = 0.0f);
Ref<StandardMaterial3D> glass();
Ref<StandardMaterial3D> unlit(const Color &color, float energy = 1.0f, bool additive = false);
Ref<StandardMaterial3D> particle(bool additive, bool lit = false);
Ref<ShaderMaterial> prop_disc(float blades, const Color &tip);
Ref<ShaderMaterial> terrain();
Ref<ShaderMaterial> cloud();
Ref<Shader> canvas_shader(const char *which); // "paper" or "vignette"

Ref<Texture2D> soft_dot(); // radial falloff sprite
Ref<Texture2D> smoke_sprite(); // lumpier falloff for smoke
Ref<Texture2D> ring_sprite(); // shockwave ring

void clear_cache();

} // namespace mats
} // namespace ww2
