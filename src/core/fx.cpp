#include "core/fx.h"

#include "core/materials.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/classes/curve_texture.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/gradient_texture1_d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/omni_light3d.hpp>
#include <godot_cpp/classes/particle_process_material.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

using namespace godot;

namespace ww2 {
namespace fx {

namespace {

struct Stop {
	float t;
	Color c;
};

struct PSpec {
	int amount = 16;
	float lifetime = 1.0f;
	bool one_shot = true;
	float explosiveness = 1.0f;
	Vector3 direction = Vector3(0, 1, 0);
	float spread = 180.0f;
	float vel_min = 1.0f;
	float vel_max = 2.0f;
	Vector3 gravity = Vector3(0, 0, 0);
	float scale_min = 1.0f;
	float scale_max = 1.0f;
	float scale_start = 1.0f; // curve, as a fraction of scale
	float scale_end = 1.0f;
	float damping = 0.0f;
	float emission_radius = 0.0f;
	float spin = 0.0f;
	bool additive = false;
	bool lit = false;
	bool local = false;
	float randomness = 0.3f;
	std::vector<Stop> ramp;
};

GPUParticles3D *make(const PSpec &s) {
	Ref<ParticleProcessMaterial> pm;
	pm.instantiate();
	pm->set_direction(s.direction);
	pm->set_spread(s.spread);
	pm->set_param_min(ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, s.vel_min);
	pm->set_param_max(ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, s.vel_max);
	pm->set_gravity(s.gravity);
	pm->set_param_min(ParticleProcessMaterial::PARAM_SCALE, s.scale_min);
	pm->set_param_max(ParticleProcessMaterial::PARAM_SCALE, s.scale_max);
	pm->set_param_min(ParticleProcessMaterial::PARAM_DAMPING, s.damping);
	pm->set_param_max(ParticleProcessMaterial::PARAM_DAMPING, s.damping);
	pm->set_param_min(ParticleProcessMaterial::PARAM_ANGLE, -180.0f);
	pm->set_param_max(ParticleProcessMaterial::PARAM_ANGLE, 180.0f);
	pm->set_param_min(ParticleProcessMaterial::PARAM_ANGULAR_VELOCITY, -s.spin);
	pm->set_param_max(ParticleProcessMaterial::PARAM_ANGULAR_VELOCITY, s.spin);
	if (s.emission_radius > 0.0f) {
		pm->set_emission_shape(ParticleProcessMaterial::EMISSION_SHAPE_SPHERE);
		pm->set_emission_sphere_radius(s.emission_radius);
	}

	Ref<Curve> curve;
	curve.instantiate();
	curve->set_max_value(MAX(MAX(s.scale_start, s.scale_end), 1.0f));
	curve->add_point(Vector2(0.0f, s.scale_start));
	curve->add_point(Vector2(0.35f, lerpf(s.scale_start, s.scale_end, 0.7f)));
	curve->add_point(Vector2(1.0f, s.scale_end));
	Ref<CurveTexture> curve_tex;
	curve_tex.instantiate();
	curve_tex->set_curve(curve);
	pm->set_param_texture(ParticleProcessMaterial::PARAM_SCALE, curve_tex);

	if (!s.ramp.empty()) {
		Ref<Gradient> g;
		g.instantiate();
		PackedFloat32Array offsets;
		PackedColorArray colors;
		for (const Stop &stop : s.ramp) {
			offsets.push_back(stop.t);
			colors.push_back(stop.c);
		}
		g->set_offsets(offsets);
		g->set_colors(colors);
		Ref<GradientTexture1D> gt;
		gt.instantiate();
		gt->set_use_hdr(true);
		gt->set_gradient(g);
		pm->set_color_ramp(gt);
	}

	Ref<QuadMesh> quad;
	quad.instantiate();
	quad->set_size(Vector2(1, 1));
	quad->set_material(mats::particle(s.additive, s.lit));

	GPUParticles3D *p = memnew(GPUParticles3D);
	p->set_amount(s.amount);
	p->set_lifetime(s.lifetime);
	p->set_one_shot(s.one_shot);
	p->set_explosiveness_ratio(s.explosiveness);
	p->set_randomness_ratio(s.randomness);
	p->set_process_material(pm);
	p->set_draw_pass_mesh(0, quad);
	p->set_use_local_coordinates(s.local);
	p->set_visibility_aabb(AABB(Vector3(-400, -400, -400), Vector3(800, 800, 800)));
	p->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	p->set_emitting(true);
	return p;
}

Node3D *holder(Node *parent, const Vector3 &pos, float free_after) {
	Node3D *h = memnew(Node3D);
	parent->add_child(h);
	h->set_global_position(pos);
	SceneTree *tree = parent->get_tree();
	if (tree) {
		Ref<SceneTreeTimer> timer = tree->create_timer(free_after, false);
		timer->connect("timeout", Callable(h, "queue_free"));
	}
	return h;
}

void flash(Node3D *h, const Color &color, float energy, float range, float time) {
	OmniLight3D *light = memnew(OmniLight3D);
	light->set_color(color);
	light->set_param(Light3D::PARAM_ENERGY, energy);
	light->set_param(Light3D::PARAM_RANGE, range);
	light->set_shadow(false);
	h->add_child(light);
	Ref<Tween> tw = h->create_tween();
	tw->tween_property(light, "light_energy", 0.0f, time);
}

} // namespace

void explosion(Node *parent, const Vector3 &pos, float size, bool ground) {
	Node3D *h = holder(parent, pos, 5.0f);

	PSpec fire;
	fire.amount = 26;
	fire.lifetime = 0.75f;
	fire.vel_min = size * 1.0f;
	fire.vel_max = size * 4.5f;
	fire.damping = size * 3.0f;
	fire.scale_min = size * 1.2f;
	fire.scale_max = size * 2.6f;
	fire.scale_start = 0.35f;
	fire.scale_end = 1.0f;
	fire.emission_radius = size * 0.3f;
	fire.additive = true;
	fire.spin = 60.0f;
	fire.ramp = { { 0.0f, Color(6.0, 5.0, 3.0, 1.0) }, { 0.2f, Color(4.0, 1.8, 0.4, 0.9) },
		{ 0.6f, Color(1.2, 0.25, 0.05, 0.5) }, { 1.0f, Color(0.2, 0.02, 0.0, 0.0) } };
	if (ground) {
		fire.direction = Vector3(0, 1, 0);
		fire.spread = 70.0f;
	}
	h->add_child(make(fire));

	PSpec smoke;
	smoke.amount = 22;
	smoke.lifetime = 3.6f;
	smoke.explosiveness = 0.9f;
	smoke.vel_min = size * 0.6f;
	smoke.vel_max = size * 2.8f;
	smoke.damping = size * 1.2f;
	smoke.gravity = Vector3(0, size * 0.5f, 0);
	smoke.scale_min = size * 2.0f;
	smoke.scale_max = size * 4.2f;
	smoke.scale_start = 0.3f;
	smoke.scale_end = 1.0f;
	smoke.emission_radius = size * 0.4f;
	smoke.spin = 25.0f;
	smoke.ramp = { { 0.0f, Color(0.25, 0.18, 0.12, 0.0) }, { 0.08f, Color(0.12, 0.10, 0.09, 0.85) },
		{ 0.5f, Color(0.16, 0.15, 0.15, 0.55) }, { 1.0f, Color(0.3, 0.3, 0.3, 0.0) } };
	if (ground) {
		smoke.direction = Vector3(0, 1, 0);
		smoke.spread = 60.0f;
		smoke.gravity = Vector3(0, size * 0.9f, 0);
	}
	h->add_child(make(smoke));

	PSpec sparks;
	sparks.amount = 30;
	sparks.lifetime = 1.3f;
	sparks.vel_min = size * 5.0f;
	sparks.vel_max = size * 14.0f;
	sparks.gravity = Vector3(0, -9.8f * 2.0f, 0);
	sparks.scale_min = size * 0.12f;
	sparks.scale_max = size * 0.3f;
	sparks.scale_start = 1.0f;
	sparks.scale_end = 0.2f;
	sparks.additive = true;
	sparks.ramp = { { 0.0f, Color(6.0, 4.0, 1.5, 1.0) }, { 0.6f, Color(3.0, 0.8, 0.1, 0.9) },
		{ 1.0f, Color(0.5, 0.05, 0.0, 0.0) } };
	if (ground) {
		sparks.direction = Vector3(0, 1, 0);
		sparks.spread = 65.0f;
	}
	h->add_child(make(sparks));

	if (ground) {
		PSpec dirt;
		dirt.amount = 24;
		dirt.lifetime = 1.8f;
		dirt.direction = Vector3(0, 1, 0);
		dirt.spread = 35.0f;
		dirt.vel_min = size * 4.0f;
		dirt.vel_max = size * 11.0f;
		dirt.gravity = Vector3(0, -9.8f * 1.6f, 0);
		dirt.scale_min = size * 0.6f;
		dirt.scale_max = size * 1.6f;
		dirt.scale_start = 0.6f;
		dirt.scale_end = 1.0f;
		dirt.spin = 80.0f;
		dirt.ramp = { { 0.0f, Color(0.20, 0.15, 0.10, 0.95) }, { 0.7f, Color(0.22, 0.17, 0.12, 0.7) },
			{ 1.0f, Color(0.25, 0.2, 0.15, 0.0) } };
		h->add_child(make(dirt));
	}

	flash(h, Color(1.0, 0.7, 0.35), 6.0f * size, size * 14.0f, 0.5f);
}

void flak_burst(Node *parent, const Vector3 &pos, float size) {
	Node3D *h = holder(parent, pos, 6.0f);

	PSpec core;
	core.amount = 8;
	core.lifetime = 0.25f;
	core.vel_min = size * 1.0f;
	core.vel_max = size * 5.0f;
	core.scale_min = size * 1.0f;
	core.scale_max = size * 2.0f;
	core.additive = true;
	core.ramp = { { 0.0f, Color(5.0, 3.5, 1.5, 1.0) }, { 1.0f, Color(1.0, 0.2, 0.0, 0.0) } };
	h->add_child(make(core));

	PSpec smoke;
	smoke.amount = 18;
	smoke.lifetime = 5.0f;
	smoke.explosiveness = 0.95f;
	smoke.vel_min = size * 1.5f;
	smoke.vel_max = size * 6.0f;
	smoke.damping = size * 5.0f;
	smoke.scale_min = size * 1.6f;
	smoke.scale_max = size * 3.2f;
	smoke.scale_start = 0.4f;
	smoke.scale_end = 1.0f;
	smoke.emission_radius = size * 0.3f;
	smoke.spin = 15.0f;
	smoke.ramp = { { 0.0f, Color(0.03, 0.03, 0.03, 0.0) }, { 0.04f, Color(0.03, 0.03, 0.03, 0.95) },
		{ 0.6f, Color(0.08, 0.08, 0.08, 0.6) }, { 1.0f, Color(0.15, 0.15, 0.15, 0.0) } };
	h->add_child(make(smoke));
	flash(h, Color(1.0, 0.75, 0.4), 3.0f * size, size * 10.0f, 0.2f);
}

void bullet_puff(Node *parent, const Vector3 &pos, float size, const Color &color) {
	Node3D *h = holder(parent, pos, 1.6f);
	PSpec dust;
	dust.amount = 6;
	dust.lifetime = 0.9f;
	dust.direction = Vector3(0, 1, 0);
	dust.spread = 30.0f;
	dust.vel_min = size * 2.0f;
	dust.vel_max = size * 6.0f;
	dust.damping = size * 4.0f;
	dust.gravity = Vector3(0, -3.0f, 0);
	dust.scale_min = size * 0.8f;
	dust.scale_max = size * 1.8f;
	dust.scale_start = 0.4f;
	dust.scale_end = 1.0f;
	Color c0 = color;
	c0.a = 0.8f;
	Color c1 = color;
	c1.a = 0.0f;
	dust.ramp = { { 0.0f, c0 }, { 1.0f, c1 } };
	h->add_child(make(dust));
}

void spark(Node *parent, const Vector3 &pos, float size) {
	Node3D *h = holder(parent, pos, 1.0f);
	PSpec s;
	s.amount = 8;
	s.lifetime = 0.35f;
	s.vel_min = size * 4.0f;
	s.vel_max = size * 14.0f;
	s.gravity = Vector3(0, -9.8f, 0);
	s.scale_min = size * 0.25f;
	s.scale_max = size * 0.6f;
	s.scale_start = 1.0f;
	s.scale_end = 0.1f;
	s.additive = true;
	s.ramp = { { 0.0f, Color(6.0, 5.0, 2.5, 1.0) }, { 1.0f, Color(2.0, 0.4, 0.0, 0.0) } };
	h->add_child(make(s));
}

GPUParticles3D *smoke_trail(Node3D *attach, const Color &color, float size, float lifetime, int amount) {
	PSpec s;
	s.amount = amount;
	s.lifetime = lifetime;
	s.one_shot = false;
	s.explosiveness = 0.0f;
	s.vel_min = 0.2f;
	s.vel_max = 1.5f;
	s.gravity = Vector3(0, 0.6f, 0);
	s.scale_min = size * 0.8f;
	s.scale_max = size * 1.4f;
	s.scale_start = 0.35f;
	s.scale_end = 1.0f;
	s.spin = 30.0f;
	s.randomness = 0.2f;
	Color c0 = color;
	c0.a = 0.0f;
	Color c1 = color;
	Color c2 = color.lerp(Color(0.5, 0.5, 0.5), 0.4f);
	c2.a = color.a * 0.5f;
	Color c3 = c2;
	c3.a = 0.0f;
	s.ramp = { { 0.0f, c0 }, { 0.06f, c1 }, { 0.5f, c2 }, { 1.0f, c3 } };
	GPUParticles3D *p = make(s);
	attach->add_child(p);
	return p;
}

GPUParticles3D *fire_trail(Node3D *attach, float size) {
	PSpec s;
	s.amount = 40;
	s.lifetime = 0.45f;
	s.one_shot = false;
	s.explosiveness = 0.0f;
	s.vel_min = 0.5f;
	s.vel_max = 3.0f;
	s.scale_min = size * 0.8f;
	s.scale_max = size * 1.5f;
	s.scale_start = 1.0f;
	s.scale_end = 0.3f;
	s.additive = true;
	s.spin = 90.0f;
	s.ramp = { { 0.0f, Color(5.0, 3.0, 0.8, 0.9) }, { 0.4f, Color(3.0, 0.9, 0.1, 0.7) },
		{ 1.0f, Color(0.4, 0.05, 0.0, 0.0) } };
	GPUParticles3D *p = make(s);
	attach->add_child(p);
	return p;
}

GPUParticles3D *smoke_column(Node *parent, const Vector3 &pos, float size, bool with_fire) {
	PSpec s;
	s.amount = 36;
	s.lifetime = 7.0f;
	s.one_shot = false;
	s.explosiveness = 0.0f;
	s.direction = Vector3(0.25f, 1, 0.1f);
	s.spread = 12.0f;
	s.vel_min = size * 1.6f;
	s.vel_max = size * 2.6f;
	s.gravity = Vector3(size * 0.4f, size * 0.3f, 0);
	s.scale_min = size * 1.5f;
	s.scale_max = size * 2.6f;
	s.scale_start = 0.3f;
	s.scale_end = 1.0f;
	s.emission_radius = size * 0.3f;
	s.spin = 20.0f;
	s.ramp = { { 0.0f, Color(0.2, 0.1, 0.05, 0.0) }, { 0.06f, Color(0.04, 0.035, 0.03, 0.9) },
		{ 0.6f, Color(0.10, 0.10, 0.10, 0.6) }, { 1.0f, Color(0.25, 0.25, 0.25, 0.0) } };
	GPUParticles3D *p = make(s);
	parent->add_child(p);
	p->set_global_position(pos);
	if (with_fire) {
		PSpec f;
		f.amount = 24;
		f.lifetime = 0.8f;
		f.one_shot = false;
		f.explosiveness = 0.0f;
		f.direction = Vector3(0, 1, 0);
		f.spread = 25.0f;
		f.vel_min = size * 1.0f;
		f.vel_max = size * 3.0f;
		f.scale_min = size * 0.8f;
		f.scale_max = size * 1.6f;
		f.scale_start = 1.0f;
		f.scale_end = 0.2f;
		f.emission_radius = size * 0.5f;
		f.additive = true;
		f.local = true;
		f.ramp = { { 0.0f, Color(5.0, 3.0, 0.8, 0.0) }, { 0.15f, Color(4.0, 2.0, 0.4, 0.9) },
			{ 1.0f, Color(0.6, 0.06, 0.0, 0.0) } };
		p->add_child(make(f));
	}
	return p;
}

// ---------------------------------------------------------------------------

void Bullets::setup(Node *parent, int p_capacity, float tracer_length, float tracer_width) {
	capacity = p_capacity;
	length = tracer_length;
	width = tracer_width;
	bullets.reserve(capacity);

	Ref<BoxMesh> box;
	box.instantiate();
	box->set_size(Vector3(1, 1, 1));
	Ref<StandardMaterial3D> mat;
	mat.instantiate();
	mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	mat->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	mat->set_flag(BaseMaterial3D::FLAG_DISABLE_FOG, true);
	mat->set_albedo(Color(1, 1, 1));
	box->set_material(mat);

	Ref<MultiMesh> mm;
	mm.instantiate();
	mm->set_transform_format(MultiMesh::TRANSFORM_3D);
	mm->set_use_colors(true);
	mm->set_mesh(box);
	mm->set_instance_count(capacity);
	mm->set_visible_instance_count(0);

	node = memnew(MultiMeshInstance3D);
	node->set_name("Tracers");
	node->set_multimesh(mm);
	node->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	node->set_custom_aabb(AABB(Vector3(-1e5f, -1e4f, -1e5f), Vector3(2e5f, 2e4f, 2e5f)));
	parent->add_child(node);
}

Bullet &Bullets::fire(const Vector3 &pos, const Vector3 &vel, float life, float damage, int team, int owner) {
	if ((int)bullets.size() >= capacity) {
		bullets.erase(bullets.begin());
	}
	Bullet b;
	b.pos = pos;
	b.prev = pos;
	b.vel = vel;
	b.life = life;
	b.damage = damage;
	b.team = team;
	b.owner = owner;
	bullets.push_back(b);
	return bullets.back();
}

void Bullets::step(float dt, float gravity) {
	for (Bullet &b : bullets) {
		b.prev = b.pos;
		b.vel.y -= gravity * dt;
		b.pos += b.vel * dt;
		b.life -= dt;
		if (b.life <= 0.0f) {
			b.alive = false;
		}
	}
	size_t w = 0;
	for (size_t i = 0; i < bullets.size(); i++) {
		if (bullets[i].alive) {
			if (w != i) {
				bullets[w] = bullets[i];
			}
			w++;
		}
	}
	bullets.resize(w);
}

void Bullets::render() {
	if (!node) {
		return;
	}
	Ref<MultiMesh> mm = node->get_multimesh();
	int n = MIN((int)bullets.size(), capacity);
	for (int i = 0; i < n; i++) {
		const Bullet &b = bullets[i];
		Vector3 dir = b.vel.normalized();
		Vector3 up = std::fabs(dir.y) > 0.95f ? Vector3(1, 0, 0) : Vector3(0, 1, 0);
		Vector3 x = up.cross(dir).normalized();
		Vector3 y = dir.cross(x).normalized();
		Basis basis(x * width, y * width, dir * length);
		mm->set_instance_transform(i, Transform3D(basis, b.pos - dir * (length * 0.5f)));
		mm->set_instance_color(i, b.team == 0 ? Color(4.0, 2.6, 0.7) : Color(2.2, 3.6, 1.6));
	}
	mm->set_visible_instance_count(n);
}

} // namespace fx
} // namespace ww2
