#include "core/materials.h"

#include "core/shaders.h"

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <map>

using namespace godot;

namespace ww2 {
namespace mats {

namespace {
struct Cache {
	Ref<Shader> plane;
	Ref<Shader> prop;
	Ref<Shader> terrain;
	Ref<Shader> cloud;
	Ref<Shader> paper;
	Ref<Shader> vignette;
	Ref<StandardMaterial3D> glass;
	Ref<Texture2D> dot;
	Ref<Texture2D> smoke;
	Ref<Texture2D> ring;
	std::map<int, Ref<StandardMaterial3D>> vertex;
	std::map<int, Ref<StandardMaterial3D>> particles;
};
Cache *cache = nullptr;

Cache &c() {
	if (!cache) {
		cache = new Cache();
	}
	return *cache;
}

Ref<Shader> make_shader(const String &code) {
	Ref<Shader> s;
	s.instantiate();
	s->set_code(code);
	return s;
}

Vector3 rgb(const Color &col) {
	return Vector3(col.r, col.g, col.b);
}
} // namespace

void clear_cache() {
	delete cache;
	cache = nullptr;
}

Ref<ShaderMaterial> plane_paint(PaintScheme scheme) {
	if (c().plane.is_null()) {
		c().plane = make_shader(String("shader_type spatial;\n") + shaders::NOISE_LIB + shaders::PLANE_PAINT);
	}
	Ref<ShaderMaterial> m;
	m.instantiate();
	m->set_shader(c().plane);

	switch (scheme) {
		case PAINT_P47_SILVER: {
			m->set_shader_parameter("top_color", Color(0.78, 0.79, 0.80));
			m->set_shader_parameter("bottom_color", Color(0.74, 0.75, 0.77));
			m->set_shader_parameter("nose_color", Color(0.72, 0.06, 0.05));
			m->set_shader_parameter("accent_color", Color(0.72, 0.06, 0.05));
			m->set_shader_parameter("glare_panel", 1.0f);
			m->set_shader_parameter("glare_color", Color(0.16, 0.19, 0.10));
			m->set_shader_parameter("nose_z", -3.9f);
			m->set_shader_parameter("rudder_z", 5.35f);
			m->set_shader_parameter("insignia", 1);
			m->set_shader_parameter("stripes", 1.0f);
			m->set_shader_parameter("stripe_w", 0.34f);
			m->set_shader_parameter("stripe_wing_x", 2.0f);
			m->set_shader_parameter("stripe_fus_z", 2.35f);
			m->set_shader_parameter("wing_mark", Vector3(4.75f, -0.35f, 0.42f));
			m->set_shader_parameter("fus_mark", Vector3(1.55f, 0.1f, 0.36f));
			m->set_shader_parameter("metal", 0.78f);
			m->set_shader_parameter("rough", 0.42f);
			m->set_shader_parameter("cockpit_z0", -0.9f);
		} break;
		case PAINT_P47_OLIVE: {
			m->set_shader_parameter("top_color", Color(0.20, 0.23, 0.12));
			m->set_shader_parameter("bottom_color", Color(0.50, 0.53, 0.55));
			m->set_shader_parameter("nose_color", Color(0.85, 0.72, 0.10));
			m->set_shader_parameter("accent_color", Color(0.85, 0.72, 0.10));
			m->set_shader_parameter("glare_panel", 0.0f);
			m->set_shader_parameter("nose_z", -3.9f);
			m->set_shader_parameter("rudder_z", 5.35f);
			m->set_shader_parameter("insignia", 1);
			m->set_shader_parameter("stripes", 1.0f);
			m->set_shader_parameter("stripe_w", 0.34f);
			m->set_shader_parameter("stripe_wing_x", 2.0f);
			m->set_shader_parameter("stripe_fus_z", 2.35f);
			m->set_shader_parameter("wing_mark", Vector3(4.75f, -0.35f, 0.42f));
			m->set_shader_parameter("fus_mark", Vector3(1.55f, 0.1f, 0.36f));
			m->set_shader_parameter("metal", 0.15f);
			m->set_shader_parameter("rough", 0.62f);
			m->set_shader_parameter("cockpit_z0", -0.9f);
		} break;
		case PAINT_BF109_GREY:
		case PAINT_BF109_ACE: {
			const bool ace = scheme == PAINT_BF109_ACE;
			m->set_shader_parameter("top_color", ace ? Color(0.20, 0.24, 0.22) : Color(0.33, 0.36, 0.38));
			m->set_shader_parameter("camo_color", ace ? Color(0.10, 0.13, 0.11) : Color(0.19, 0.22, 0.24));
			m->set_shader_parameter("camo", 1.0f);
			m->set_shader_parameter("bottom_color", Color(0.50, 0.62, 0.68));
			m->set_shader_parameter("nose_color", ace ? Color(0.70, 0.07, 0.05) : Color(0.88, 0.72, 0.08));
			m->set_shader_parameter("accent_color", ace ? Color(0.70, 0.07, 0.05) : Color(0.88, 0.72, 0.08));
			m->set_shader_parameter("glare_panel", 0.0f);
			m->set_shader_parameter("nose_z", -3.05f);
			m->set_shader_parameter("rudder_z", 4.25f);
			m->set_shader_parameter("insignia", 2);
			m->set_shader_parameter("stripes", 0.0f);
			m->set_shader_parameter("wing_mark", Vector3(3.55f, -0.25f, 0.50f));
			m->set_shader_parameter("fus_mark", Vector3(1.55f, 0.08f, 0.34f));
			m->set_shader_parameter("metal", 0.1f);
			m->set_shader_parameter("rough", 0.6f);
			m->set_shader_parameter("cockpit_z0", -0.8f);
		} break;
		case PAINT_FW190: {
			m->set_shader_parameter("top_color", Color(0.28, 0.33, 0.30));
			m->set_shader_parameter("camo_color", Color(0.16, 0.21, 0.18));
			m->set_shader_parameter("camo", 1.0f);
			m->set_shader_parameter("bottom_color", Color(0.52, 0.63, 0.68));
			m->set_shader_parameter("nose_color", Color(0.88, 0.72, 0.08));
			m->set_shader_parameter("accent_color", Color(0.88, 0.72, 0.08));
			m->set_shader_parameter("nose_z", -3.3f);
			m->set_shader_parameter("rudder_z", 4.3f);
			m->set_shader_parameter("insignia", 2);
			m->set_shader_parameter("wing_mark", Vector3(3.7f, -0.2f, 0.52f));
			m->set_shader_parameter("fus_mark", Vector3(1.6f, 0.08f, 0.36f));
			m->set_shader_parameter("metal", 0.1f);
			m->set_shader_parameter("rough", 0.6f);
		} break;
	}
	return m;
}

Ref<StandardMaterial3D> vertex_color(float rough, float metal) {
	int key = (int)(rough * 100.0f) * 1000 + (int)(metal * 100.0f);
	auto it = c().vertex.find(key);
	if (it != c().vertex.end()) {
		return it->second;
	}
	Ref<StandardMaterial3D> m;
	m.instantiate();
	m->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	m->set_flag(BaseMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
	m->set_roughness(rough);
	m->set_metallic(metal);
	c().vertex[key] = m;
	return m;
}

Ref<StandardMaterial3D> glass() {
	if (c().glass.is_null()) {
		Ref<StandardMaterial3D> m;
		m.instantiate();
		m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
		m->set_albedo(Color(0.55, 0.75, 0.85, 0.28));
		m->set_roughness(0.04f);
		m->set_metallic(0.3f);
		m->set_specular(0.9f);
		m->set_cull_mode(BaseMaterial3D::CULL_BACK);
		c().glass = m;
	}
	return c().glass;
}

Ref<StandardMaterial3D> unlit(const Color &color, float energy, bool additive) {
	Ref<StandardMaterial3D> m;
	m.instantiate();
	m->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	Color col = color;
	col.r *= energy;
	col.g *= energy;
	col.b *= energy;
	m->set_albedo(col);
	if (additive) {
		m->set_blend_mode(BaseMaterial3D::BLEND_MODE_ADD);
		m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
	} else if (color.a < 0.999f) {
		m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
	}
	m->set_flag(BaseMaterial3D::FLAG_DISABLE_FOG, additive);
	return m;
}

Ref<StandardMaterial3D> particle(bool additive, bool lit) {
	int key = (additive ? 1 : 0) + (lit ? 2 : 0);
	auto it = c().particles.find(key);
	if (it != c().particles.end()) {
		return it->second;
	}
	Ref<StandardMaterial3D> m;
	m.instantiate();
	m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
	m->set_shading_mode(lit ? BaseMaterial3D::SHADING_MODE_PER_VERTEX : BaseMaterial3D::SHADING_MODE_UNSHADED);
	m->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	m->set_billboard_mode(BaseMaterial3D::BILLBOARD_PARTICLES);
	m->set_flag(BaseMaterial3D::FLAG_BILLBOARD_KEEP_SCALE, true);
	m->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, additive ? soft_dot() : smoke_sprite());
	m->set_depth_draw_mode(BaseMaterial3D::DEPTH_DRAW_DISABLED);
	m->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
	if (additive) {
		m->set_blend_mode(BaseMaterial3D::BLEND_MODE_ADD);
	}
	m->set_proximity_fade_enabled(true);
	m->set_proximity_fade_distance(1.5f);
	c().particles[key] = m;
	return m;
}

Ref<ShaderMaterial> prop_disc(float blades, const Color &tip) {
	if (c().prop.is_null()) {
		c().prop = make_shader(shaders::PROP_DISC);
	}
	Ref<ShaderMaterial> m;
	m.instantiate();
	m->set_shader(c().prop);
	m->set_shader_parameter("blades", blades);
	m->set_shader_parameter("tip_color", tip);
	return m;
}

Ref<ShaderMaterial> terrain() {
	if (c().terrain.is_null()) {
		c().terrain = make_shader(
				String("shader_type spatial;\n") + shaders::NOISE_LIB + shaders::TERRAIN_A + shaders::TERRAIN_B);
	}
	Ref<ShaderMaterial> m;
	m.instantiate();
	m->set_shader(c().terrain);
	return m;
}

Ref<ShaderMaterial> cloud() {
	if (c().cloud.is_null()) {
		c().cloud = make_shader(shaders::CLOUD);
	}
	Ref<ShaderMaterial> m;
	m.instantiate();
	m->set_shader(c().cloud);
	return m;
}

Ref<Shader> canvas_shader(const char *which) {
	if (String(which) == "paper") {
		if (c().paper.is_null()) {
			c().paper = make_shader(shaders::PAPER);
		}
		return c().paper;
	}
	if (c().vignette.is_null()) {
		c().vignette = make_shader(shaders::VIGNETTE);
	}
	return c().vignette;
}

namespace {
Ref<Texture2D> make_sprite(int kind) {
	const int size = 128;
	PackedByteArray data;
	data.resize(size * size * 4);
	uint8_t *w = data.ptrw();
	Rng rng(kind * 7919 + 13);
	// A few random lobes make the smoke sprite less obviously circular.
	float lobes[6][3];
	for (int i = 0; i < 6; i++) {
		lobes[i][0] = rng.range(-0.3f, 0.3f);
		lobes[i][1] = rng.range(-0.3f, 0.3f);
		lobes[i][2] = rng.range(0.35f, 0.6f);
	}
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			float u = ((float)x + 0.5f) / size * 2.0f - 1.0f;
			float v = ((float)y + 0.5f) / size * 2.0f - 1.0f;
			float r = std::sqrt(u * u + v * v);
			float a = 0.0f;
			if (kind == 0) {
				a = saturate(1.0f - r);
				a = a * a * (3.0f - 2.0f * a);
				a *= a;
			} else if (kind == 1) {
				for (int i = 0; i < 6; i++) {
					float dx = u - lobes[i][0];
					float dy = v - lobes[i][1];
					float d = std::sqrt(dx * dx + dy * dy) / lobes[i][2];
					float l = saturate(1.0f - d);
					a = MAX(a, l * l * (3.0f - 2.0f * l));
				}
				a *= smooth01((1.0f - r) * 4.0f);
				a = saturate(a * 1.15f);
			} else {
				float d = std::fabs(r - 0.8f) / 0.14f;
				a = saturate(1.0f - d);
				a *= a;
			}
			uint8_t *p = w + (y * size + x) * 4;
			p[0] = 255;
			p[1] = 255;
			p[2] = 255;
			p[3] = (uint8_t)(a * 255.0f);
		}
	}
	Ref<Image> img = Image::create_from_data(size, size, false, Image::FORMAT_RGBA8, data);
	img->generate_mipmaps();
	return ImageTexture::create_from_image(img);
}
} // namespace

Ref<Texture2D> soft_dot() {
	if (c().dot.is_null()) {
		c().dot = make_sprite(0);
	}
	return c().dot;
}

Ref<Texture2D> smoke_sprite() {
	if (c().smoke.is_null()) {
		c().smoke = make_sprite(1);
	}
	return c().smoke;
}

Ref<Texture2D> ring_sprite() {
	if (c().ring.is_null()) {
		c().ring = make_sprite(2);
	}
	return c().ring;
}

} // namespace mats
} // namespace ww2
