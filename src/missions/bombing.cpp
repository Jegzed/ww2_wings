#include "missions/bombing.h"

#include "core/audio.h"
#include "core/input_setup.h"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/plane_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector4.hpp>

using namespace godot;

namespace ww2 {

namespace {
const float BOMB_GRAVITY = 45.0f;
const float BLAST_RADIUS = 28.0f;
const float BLAST_DAMAGE = 170.0f;
const float LATERAL_LIMIT = 92.0f;
const float PLANE_SCALE = 1.7f;
const float FLAK_RANGE = 430.0f;
const float FLAK_FLIGHT = 1.35f;
} // namespace

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------

void Bombing::add_farm(Rng &rng, const Vector3 &at) {
	float yaw = rng.range(0.0f, 360.0f);
	ground.add("house", at, yaw, false);
	Vector3 side = Basis(Vector3(0, 1, 0), deg2rad(yaw)).xform(Vector3(22.0f, 0, 6.0f));
	ground.add("barn", at + side, yaw + 90.0f, false);
}

void Bombing::add_village(Rng &rng, const Vector3 &at, bool church) {
	if (church) {
		ground.add("church", at, rng.range(-20.0f, 20.0f), false);
	}
	int n = rng.irange(7, 11);
	for (int i = 0; i < n; i++) {
		float a = TAU_F * i / n + rng.range(-0.2f, 0.2f);
		float r = rng.range(34.0f, 85.0f);
		Vector3 p = at + Vector3(std::cos(a) * r, 0, std::sin(a) * r);
		ground.add(rng.chance(0.8f) ? "house" : "barn", p, rad2deg(a) + 90.0f + rng.range(-15.0f, 15.0f), false);
	}
}

void Bombing::scatter_scenery(Rng &rng, float length) {
	std::vector<Transform3D> trees, poplars, bushes;
	int count = (int)(length * 0.22f);
	for (int i = 0; i < count; i++) {
		Vector3 p(rng.range(-520.0f, 520.0f), 0.0f, rng.range(-length - 400.0f, 300.0f));
		if (std::fabs(p.x) < 34.0f) {
			continue; // keep the line of flight clear
		}
		bool blocked = false;
		for (const GroundTarget &t : ground.targets) {
			if (std::fabs(t.pos.x - p.x) < MAX(t.hx, t.hz) + 6.0f && std::fabs(t.pos.z - p.z) < MAX(t.hx, t.hz) + 6.0f) {
				blocked = true;
				break;
			}
		}
		if (blocked) {
			continue;
		}
		float s = rng.range(0.8f, 1.5f);
		Basis b = Basis(Vector3(0, 1, 0), rng.range(0, TAU_F)).scaled(Vector3(s, s * rng.range(0.9f, 1.2f), s));
		// Clumps: a few neighbours around each seed.
		int clump = rng.irange(1, 5);
		for (int k = 0; k < clump; k++) {
			Vector3 q = p + Vector3(rng.range(-14.0f, 14.0f), 0, rng.range(-14.0f, 14.0f)) * (k > 0 ? 1.0f : 0.0f);
			if (std::fabs(q.x) < 30.0f) {
				continue;
			}
			float pick = rng.f();
			if (pick < 0.6f) {
				trees.push_back(Transform3D(b, q));
			} else if (pick < 0.8f) {
				poplars.push_back(Transform3D(b, q));
			} else {
				bushes.push_back(Transform3D(b, q));
			}
		}
	}
	scatter(this, "tree", trees);
	scatter(this, "poplar", poplars);
	scatter(this, "bush", bushes);
}

void Bombing::layout_rail_yard(Rng &rng) {
	run_length = 3250.0f;
	const float yard0 = 1750.0f;
	const float yard1 = 2450.0f;

	// A goods train caught on the main line.
	{
		float z = -760.0f;
		ground.add("loco", Vector3(0, 0, z), 0.0f, true);
		const char *cars[] = { "boxcar", "tanker", "boxcar", "flatcar", "tanker", "boxcar" };
		for (int i = 0; i < 6; i++) {
			ground.add(cars[i], Vector3(0, 0, z + 13.0f + i * 10.6f), 0.0f, true);
		}
	}

	add_farm(rng, Vector3(-150.0f, 0, -330.0f));
	add_farm(rng, Vector3(190.0f, 0, -1050.0f));
	add_village(rng, Vector3(-230.0f, 0, -1330.0f), true);
	add_farm(rng, Vector3(230.0f, 0, -3200.0f));

	// The yard: five roads, the centre one kept clear as the through line.
	const float tracks[] = { -14.0f, -7.0f, 7.0f, 14.0f, 21.0f };
	for (int t = 0; t < 5; t++) {
		float z = -yard0 - 70.0f - rng.range(0.0f, 60.0f);
		while (z > -yard1 + 80.0f) {
			int len = rng.irange(3, 6);
			bool with_loco = rng.chance(0.3f);
			for (int i = 0; i < len; i++) {
				const char *kind = "boxcar";
				float pick = rng.f();
				if (i == 0 && with_loco) {
					kind = "loco";
				} else if (pick < 0.3f) {
					kind = "tanker";
				} else if (pick < 0.5f) {
					kind = "flatcar";
				}
				ground.add(kind, Vector3(tracks[t], 0, z), 0.0f, true);
				z -= (i == 0 && with_loco) ? 12.4f : 10.6f;
			}
			z -= rng.range(70.0f, 170.0f);
		}
	}
	for (int side = -1; side <= 1; side += 2) {
		for (int i = 0; i < 3; i++) {
			ground.add("warehouse", Vector3(52.0f * side, 0, -yard0 - 150.0f - i * 250.0f + rng.range(-30.0f, 30.0f)), 0.0f,
					true);
			ground.add("crate_stack", Vector3(36.0f * side, 0, -yard0 - 260.0f - i * 250.0f), rng.range(0, 360.0f), true);
		}
	}
	for (int i = 0; i < 3; i++) {
		ground.add("fuel_tank", Vector3(88.0f + (i % 2) * 15.0f, 0, -yard0 - 520.0f - i * 15.0f), 0.0f, true);
	}
	ground.add("factory", Vector3(-84.0f, 0, -yard1 - 90.0f), 0.0f, true);
	ground.add("tower", Vector3(34.0f, 0, -yard0 - 40.0f), 0.0f, true);

	int flak = 2 + def->difficulty;
	for (int i = 0; i < flak; i++) {
		float side = (i % 2 == 0) ? 1.0f : -1.0f;
		float z = -yard0 + 120.0f - i * (yard1 - yard0 + 200.0f) / flak;
		ground.add("flak", Vector3(side * rng.range(70.0f, 105.0f), 0, z), rng.range(0, 360.0f), true);
	}

	Ref<ShaderMaterial> mat = terrain->get_mesh()->surface_get_material(0);
	mat->set_shader_parameter("strip_a", Vector4(0.0f, 0.0f, 0.0f, -1.0f));
	mat->set_shader_parameter("strip_a_width", 2.6f);
	mat->set_shader_parameter("strip_a_kind", 2);
	mat->set_shader_parameter("strip_b", Vector4(0.0f, 0.0f, 0.0f, -1.0f));
	mat->set_shader_parameter("strip_b_width", 25.0f);
	mat->set_shader_parameter("strip_b_kind", 2);
	mat->set_shader_parameter("strip_b_range", Vector2(yard0, yard1));
	mat->set_shader_parameter("strip_c", Vector4(0.0f, -1500.0f, 1.0f, 0.12f));
	mat->set_shader_parameter("strip_c_width", 3.5f);
	mat->set_shader_parameter("strip_c_kind", 1);
	mat->set_shader_parameter("river_amount", 0.0f);
}

void Bombing::layout_bridge(Rng &rng) {
	run_length = 3100.0f;
	const float river_z = -2300.0f;

	primary = ground.add("bridge", Vector3(0, 1.2f, river_z), 0.0f, true);
	ground.targets[primary].primary = true;
	ground.targets[primary].hp = ground.targets[primary].max_hp = 300.0f;

	// Traffic stacked up on the approach.
	const char *column[] = { "truck", "truck", "halftrack", "truck", "tank", "truck", "staff_car", "truck", "tank" };
	for (int i = 0; i < 9; i++) {
		ground.add(column[i], Vector3(rng.range(-1.2f, 1.2f), 0, river_z + 120.0f + i * 17.0f + rng.range(-2.0f, 2.0f)),
				rng.range(-4.0f, 4.0f), true);
	}
	for (int i = 0; i < 4; i++) {
		ground.add("truck", Vector3(rng.range(-1.2f, 1.2f), 0, river_z - 110.0f - i * 19.0f), 180.0f, true);
	}
	for (int i = 0; i < 3; i++) {
		ground.add("fuel_tank", Vector3(95.0f + i * 14.0f, 0, river_z + 210.0f + (i % 2) * 12.0f), 0.0f, true);
	}
	ground.add("crate_stack", Vector3(70.0f, 0, river_z + 190.0f), 20.0f, true);
	ground.add("crate_stack", Vector3(72.0f, 0, river_z + 225.0f), 80.0f, true);
	ground.add("tent", Vector3(55.0f, 0, river_z + 250.0f), 10.0f, true);
	ground.add("tent", Vector3(55.0f, 0, river_z + 262.0f), 10.0f, true);

	add_farm(rng, Vector3(160.0f, 0, -420.0f));
	add_village(rng, Vector3(-210.0f, 0, -1050.0f), true);
	add_farm(rng, Vector3(-180.0f, 0, -1750.0f));
	add_village(rng, Vector3(150.0f, 0, -2750.0f), false);

	int flak = 1 + def->difficulty;
	for (int i = 0; i < flak; i++) {
		float side = (i % 2 == 0) ? 1.0f : -1.0f;
		float bank_side = (i % 4 < 2) ? 1.0f : -1.0f;
		ground.add("flak", Vector3(side * rng.range(45.0f, 90.0f), 0, river_z + bank_side * rng.range(80.0f, 170.0f)),
				rng.range(0, 360.0f), true);
	}

	Ref<ShaderMaterial> mat = terrain->get_mesh()->surface_get_material(0);
	mat->set_shader_parameter("strip_a", Vector4(0.0f, 0.0f, 0.0f, -1.0f));
	mat->set_shader_parameter("strip_a_width", 4.5f);
	mat->set_shader_parameter("strip_a_kind", 1);
	mat->set_shader_parameter("strip_b", Vector4(0.0f, river_z, 1.0f, 0.0f));
	mat->set_shader_parameter("strip_b_width", 42.0f);
	mat->set_shader_parameter("strip_b_kind", 5);
	mat->set_shader_parameter("strip_c", Vector4(0.0f, -1050.0f, 1.0f, -0.2f));
	mat->set_shader_parameter("strip_c_width", 3.0f);
	mat->set_shader_parameter("strip_c_kind", 1);
	mat->set_shader_parameter("river_amount", 0.0f);
}

void Bombing::build() {
	GameState &gs = GameState::get();

	WorldOptions opt;
	opt.time = def->time;
	opt.shadow_distance = 650.0f;
	opt.fog_density = 0.00012f;
	opt.sun_azimuth_deg = 125.0f;
	opt.sun_elevation_deg = 66.0f;
	opt.sun_scatter = 0.0f;
	world = build_world(this, opt);

	Ref<ShaderMaterial> terrain_mat;
	terrain = build_terrain(this, terrain_mat, 12000.0f);
	terrain_mat->set_shader_parameter("field_scale", 120.0f);
	terrain_mat->set_shader_parameter("wood_amount", 0.0f);

	ground.setup(this);
	ground.fx_scale = 1.3f;
	ground.on_destroyed = [this](int index, bool by_player) {
		const GroundTarget &t = ground.targets[index];
		result.score += t.score * 2 / 5;
		if (t.counts) {
			result.ground_kills++;
		}
		shake(0.35f);
		if (t.kind == "bridge") {
			message("The bridge is down!", 4.0f, ui::AMBER);
		} else if (t.kind == "loco") {
			message("Locomotive destroyed!", 2.5f, ui::AMBER);
		} else if (t.kind == "fuel_tank") {
			message("Fuel dump burning!", 2.5f, ui::AMBER);
		} else if (t.kind == "factory") {
			message("Direct hit on the works!", 2.5f, ui::AMBER);
		} else if (t.kind == "flak") {
			message("Flak position silenced", 2.0f);
		}
	};

	Rng rng(101 + def->variant * 17);
	if (def->variant == 1) {
		layout_bridge(rng);
	} else {
		layout_rail_yard(rng);
	}
	scatter_scenery(rng, run_length);

	tracers.setup(this, 200, 9.0f, 0.35f);

	plane_paint = mats::plane_paint(mats::PAINT_P47_SILVER);
	// Seen from above in full sun, polished metal just burns out.
	plane_paint->set_shader_parameter("metal", 0.3f);
	plane_paint->set_shader_parameter("rough", 0.55f);
	plane_paint->set_shader_parameter("top_color", Color(0.62f, 0.64f, 0.66f));
	plane = models::make_plane(models::PLANE_P47, plane_paint, true);
	add_child(plane);
	plane->set_scale(Vector3(PLANE_SCALE, PLANE_SCALE, PLANE_SCALE));
	altitude = 100.0f;

	// Aiming marker on the ground.
	{
		Ref<PlaneMesh> pm;
		pm.instantiate();
		pm->set_size(Vector2(1, 1));
		Ref<StandardMaterial3D> m;
		m.instantiate();
		m->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
		m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
		m->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, mats::ring_sprite());
		m->set_albedo(Color(1.0f, 0.85f, 0.3f, 0.95f));
		m->set_flag(BaseMaterial3D::FLAG_DISABLE_FOG, true);
		m->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
		m->set_render_priority(10);
		pm->set_material(m);
		marker = memnew(MeshInstance3D);
		marker->set_mesh(pm);
		marker->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
		marker->set_scale(Vector3(38.0f, 1.0f, 38.0f));
		add_child(marker);
	}

	camera = memnew(Camera3D);
	camera->set_near(2.0f);
	camera->set_far(6000.0f);
	camera->set_fov(50.0f);
	add_child(camera);
	camera->make_current();

	bombs_total = 12 + gs.pilot.skills[SKILL_MECHANICAL] / 2;
	if (def->variant == 1) {
		bombs_total -= 2;
	}
	bombs_left = bombs_total;
	scroll_speed = 40.0f;
	anchor_z = 160.0f;

	snd_engine = audio::make_loop(this, "engine", -11.0f);
	snd_engine->play();
	snd_wind = audio::make_loop(this, "wind", -20.0f);
	snd_wind->play();
}

// ---------------------------------------------------------------------------
// Rules
// ---------------------------------------------------------------------------

int Bombing::required() const {
	return MAX(1, (int)std::ceil(ground.total() * 0.2f));
}

bool Bombing::objective_met() const {
	if (primary >= 0) {
		return !ground.targets[primary].alive;
	}
	return ground.destroyed() >= required();
}

Vector3 Bombing::player_velocity() const {
	return Vector3(velocity.x, 0.0f, velocity.z - scroll_speed);
}

Vector3 Bombing::predicted_impact() const {
	float t = std::sqrt(2.0f * altitude / BOMB_GRAVITY);
	Vector3 v = player_velocity();
	v.x *= 0.6f;
	Vector3 p = plane->get_position();
	return Vector3(p.x + v.x * t, 0.0f, p.z + v.z * t);
}

void Bombing::drop_bomb() {
	if (bombs_left <= 0 || drop_cooldown > 0.0f || dead || turning) {
		return;
	}
	bombs_left--;
	drop_cooldown = 0.38f;
	result.shots_fired++;

	Bomb b;
	b.node = models::instance("bomb");
	add_child(b.node);
	b.pos = plane->get_position() + Vector3((bombs_left % 2 == 0 ? -3.6f : 3.6f), -1.5f, 0.0f);
	b.vel = player_velocity();
	b.vel.x *= 0.6f;
	b.node->set_position(b.pos);
	b.node->set_scale(Vector3(1.6f, 1.6f, 1.6f));
	bombs.push_back(b);

	audio::play("bomb_release", -6.0f);
	audio::play("whistle", -12.0f, grng().range(0.9f, 1.1f));
}

void Bombing::explode_bomb(const Bomb &b) {
	Vector3 at(b.pos.x, 0.0f, b.pos.z);
	fx::explosion(this, at + Vector3(0, 1.0f, 0), 11.0f, true);
	audio::play("explosion", -1.0f, grng().range(0.85f, 1.1f));
	shake(0.5f);

	MeshInstance3D *crater = models::instance("crater");
	add_child(crater);
	crater->set_position(at + Vector3(0, 0.05f, 0));
	float s = grng().range(8.0f, 11.0f);
	crater->set_scale(Vector3(s, 2.5f, s));
	crater->set_rotation(Vector3(0, grng().range(0, TAU_F), 0));
	crater->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);

	int touched = 0;
	int killed = ground.blast(at, BLAST_RADIUS, BLAST_DAMAGE, &touched);
	if (killed > 0 || touched > 0) {
		result.shots_hit++;
	}
	if (killed >= 3) {
		message(String::num_int64(killed) + String(" with one bomb!"), 2.5f, ui::AMBER);
	}
}

void Bombing::hit_player(float amount, const String &cause) {
	if (dead) {
		return;
	}
	GameState &gs = GameState::get();
	hp -= amount * gs.toughness();
	shake(0.8f);
	hurt_flash(0.6f);
	audio::play("ricochet", -5.0f, grng().range(0.8f, 1.1f));
	plane_paint->set_shader_parameter("damage", saturate(1.0f - hp / 100.0f));
	if (!plane_smoke && hp < 60.0f) {
		plane_smoke = fx::smoke_trail(plane, Color(0.1f, 0.1f, 0.1f, 0.7f), 5.0f, 2.5f, 100);
	}
	if (hp <= 0.0f) {
		dead = true;
		fx::explosion(this, plane->get_position(), 9.0f, false);
		audio::play("explosion_big", 0.0f);
		plane->set_visible(false);
		shake(1.4f);
		player_down(cause, 3.2f);
	}
}

void Bombing::update_flak(float dt) {
	Vector3 p = plane->get_position();
	Rng &rng = grng();
	flak_warning = MAX(flak_warning - dt, 0.0f);

	for (GroundTarget &t : ground.targets) {
		if (!t.alive || !t.is_flak || dead || turning) {
			continue;
		}
		Vector3 rel = p - t.pos;
		rel.y = 0.0f;
		if (rel.length() > FLAK_RANGE) {
			continue;
		}
		flak_warning = 0.5f;
		t.flak_timer -= dt;
		if (t.flak_timer > 0.0f) {
			continue;
		}
		t.flak_timer = rng.range(2.0f, 3.6f) - def->difficulty * 0.2f;
		// Aim where the aircraft will be, with some error.
		Vector3 aim = p + player_velocity() * FLAK_FLIGHT;
		float err = lerpf(42.0f, 20.0f, saturate(def->difficulty / 5.0f));
		aim += Vector3(rng.range(-err, err), rng.range(-10.0f, 10.0f), rng.range(-err, err));
		shells.push_back({ aim, FLAK_FLIGHT });
		Vector3 muzzle = t.pos + Vector3(0, 3.0f, 0);
		for (int k = 0; k < 3; k++) {
			Vector3 v = (aim - muzzle) / FLAK_FLIGHT;
			tracers.fire(muzzle + v * (k * 0.05f), v, FLAK_FLIGHT - k * 0.05f, 0.0f, 1, -1);
		}
	}

	for (Shell &s : shells) {
		s.time -= dt;
		if (s.time <= 0.0f) {
			fx::flak_burst(this, s.at, 6.0f);
			float d = s.at.distance_to(p);
			audio::play("flak", -6.0f - clampf(d / 25.0f, 0.0f, 14.0f), rng.range(0.8f, 1.2f));
			if (d < 22.0f && !dead && !turning) {
				hit_player(16.0f * (1.0f - d / 22.0f) + 4.0f, String("Brought down by flak over ") +
																	  String(def->location).get_slice(",", 0) + String("."));
			} else if (d < 60.0f) {
				shake(0.3f);
			}
		}
	}
	size_t w = 0;
	for (size_t i = 0; i < shells.size(); i++) {
		if (shells[i].time > 0.0f) {
			shells[w++] = shells[i];
		}
	}
	shells.resize(w);

	tracers.step(dt, 0.0f);
	tracers.render();
}

void Bombing::autopilot(float &ix, float &iy, bool &drop) {
	Vector3 p = plane->get_position();
	Vector3 impact = predicted_impact();
	float best = 1e9f;
	const GroundTarget *goal = nullptr;
	for (const GroundTarget &t : ground.targets) {
		if (!t.alive || !t.counts || t.is_flak) {
			continue;
		}
		float ahead = impact.z - t.pos.z; // positive when the target is still ahead of the impact point
		if (ahead < -8.0f || ahead > 420.0f || std::fabs(t.pos.x) > LATERAL_LIMIT) {
			continue;
		}
		float cost = ahead + std::fabs(t.pos.x - p.x) * 1.5f;
		if (cost < best) {
			best = cost;
			goal = &t;
		}
	}
	ix = 0.0f;
	iy = 0.0f;
	drop = false;
	// Jink a little to throw off the flak.
	float weave = std::sin(clock * 1.3f) * 0.25f;
	if (goal) {
		float dx = goal->pos.x - impact.x;
		ix = clampf(dx * 0.08f, -1.0f, 1.0f);
		if (ground.footprint_distance(*goal, impact) < 9.0f) {
			drop = true;
		}
	} else {
		ix = clampf(-p.x * 0.03f, -1.0f, 1.0f) + weave;
	}
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void Bombing::tick(float dt) {
	Input *in = Input::get_singleton();
	float ix = 0.0f;
	float iy = 0.0f;
	bool drop = false;
	if (demo) {
		autopilot(ix, iy, drop);
	} else if (!dead && !turning) {
		ix = input_x();
		iy = input_y();
		drop = in->is_action_pressed("bomb");
	}

	GameState &gs = GameState::get();
	float agility = gs.handling();

	// Relative motion inside the scrolling window.
	Vector3 want(ix * 62.0f * agility, 0.0f, iy * 30.0f);
	velocity = damp(velocity, want, 3.2f, dt);
	if (!dead) {
		offset += velocity * dt;
	}
	if (std::fabs(offset.x) > LATERAL_LIMIT) {
		offset.x = clampf(offset.x, -LATERAL_LIMIT, LATERAL_LIMIT);
		velocity.x = 0.0f;
	}
	if (offset.z > 40.0f || offset.z < -34.0f) {
		offset.z = clampf(offset.z, -34.0f, 40.0f);
		velocity.z = 0.0f;
	}

	if (!dead) {
		anchor_z -= scroll_speed * dt;
	}

	bank = damp(bank, -velocity.x / 62.0f * deg2rad(38.0f), 5.0f, dt);
	float pitch = velocity.z / 30.0f * deg2rad(6.0f);
	Vector3 ppos(offset.x, altitude + std::sin(clock * 0.8f) * 1.2f, anchor_z + offset.z);
	plane->set_position(ppos);
	plane->set_rotation(Vector3(pitch, 0.0f, bank));
	plane->set_scale(Vector3(PLANE_SCALE, PLANE_SCALE, PLANE_SCALE));

	drop_cooldown = MAX(drop_cooldown - dt, 0.0f);
	if (drop) {
		drop_bomb();
	}

	// Bombs in flight.
	for (Bomb &b : bombs) {
		b.vel.y -= BOMB_GRAVITY * dt;
		b.pos += b.vel * dt;
		if (b.node) {
			b.node->set_position(b.pos);
			Vector3 d = b.vel.normalized();
			b.node->look_at_from_position(b.pos, b.pos + d, std::fabs(d.y) > 0.98f ? Vector3(1, 0, 0) : Vector3(0, 1, 0));
			b.node->set_scale(Vector3(2.2f, 2.2f, 2.2f));
		}
	}
	for (size_t i = 0; i < bombs.size();) {
		if (bombs[i].pos.y <= 0.5f) {
			explode_bomb(bombs[i]);
			if (bombs[i].node) {
				bombs[i].node->queue_free();
			}
			bombs.erase(bombs.begin() + i);
		} else {
			i++;
		}
	}

	ground.update(dt);
	update_flak(dt);

	// Aiming marker.
	Vector3 impact = predicted_impact();
	marker->set_position(impact + Vector3(0, 0.6f, 0));
	marker->set_visible(!dead && !turning && bombs_left > 0);
	marker->set_rotation(Vector3(0, clock * 0.8f, 0));

	terrain->set_position(Vector3(0, 0, std::floor(anchor_z / 500.0f) * 500.0f));

	// Camera: above and slightly behind, north up.
	cam_x = damp(cam_x, offset.x * 0.55f, 3.0f, dt);
	Vector3 eye(cam_x, 215.0f, anchor_z + 30.0f);
	Vector3 look(cam_x, 0.0f, anchor_z - 52.0f);
	eye += shake_offset() * 5.0f;
	camera->look_at_from_position(eye, look, Vector3(0, 0, -1));

	if (snd_engine) {
		snd_engine->set_pitch_scale(0.95f + velocity.z * -0.004f + (dead ? 0.4f : 0.0f));
		snd_engine->set_volume_db(dead ? -60.0f : -11.0f);
	}

	// Run bookkeeping.
	if (dead || ending) {
		return;
	}
	if (turning) {
		turn_fade += dt;
		if (turn_fade >= 1.2f && anchor_z < 0.0f) {
			anchor_z = 160.0f;
			offset = Vector3();
			velocity = Vector3();
			pass++;
		}
		if (turn_fade >= 2.4f) {
			turning = false;
			turn_fade = 0.0f;
			message(String("Second pass - make it count"), 3.0f, ui::AMBER);
		}
		return;
	}

	bool all_landed = bombs.empty();
	if (bombs_left == 0 && all_landed) {
		empty_timer += dt;
		if (empty_timer > 2.0f) {
			bool ok = objective_met();
			message(ok ? "Bombs gone. Good work - heading home." : "Bombs gone. Target still standing.", 4.0f,
					ok ? ui::AMBER : ui::RED);
			if (ok) {
				result.score += 500;
			}
			finish(ok, 4.0f);
		}
	} else if (-anchor_z > run_length) {
		if (objective_met()) {
			message("Target destroyed. Heading home.", 4.0f, ui::AMBER);
			result.score += 500 + bombs_left * 40;
			finish(true, 4.0f);
		} else if (pass < max_passes && bombs_left > 0) {
			turning = true;
			turn_fade = 0.0f;
			message("Coming around for another pass", 3.0f);
		} else {
			message("Target still standing. Heading home.", 4.0f, ui::RED);
			finish(false, 4.0f);
		}
	} else if (objective_met() && primary >= 0) {
		result.score += 500 + bombs_left * 40;
		finish(true, 5.0f);
	}
}

void Bombing::on_finish() {
	result.targets_destroyed = ground.destroyed();
	result.targets_total = ground.total();
	if (primary < 0) {
		// Report against what was actually asked of the pilot.
		result.targets_total = required();
	}
}

// ---------------------------------------------------------------------------
// HUD
// ---------------------------------------------------------------------------

void Bombing::draw_hud(Canvas *c) {
	Vector2 size = hud_size();
	Ref<Font> title = ui::font_title();
	const Color white(1, 1, 1, 0.9f);
	const Color dim(1, 1, 1, 0.6f);
	const Color red(1.0f, 0.25f, 0.18f, 0.95f);

	// Bombs, bottom-left.
	float x = 60.0f;
	float y = size.y - 60.0f;
	c->text_shadowed(title, Vector2(x, y - 86), "BOMBS", 22, dim);
	for (int i = 0; i < bombs_total; i++) {
		Vector2 at(x + 12.0f + i * 26.0f, y - 40.0f);
		Color col = i < bombs_left ? ui::AMBER : Color(1, 1, 1, 0.15f);
		c->draw_rect(Rect2(at + Vector2(-7, -22), Vector2(14, 36)), col, true);
		PackedVector2Array nose;
		nose.push_back(at + Vector2(-7, 14));
		nose.push_back(at + Vector2(7, 14));
		nose.push_back(at + Vector2(0, 28));
		c->draw_colored_polygon(nose, col);
		c->draw_rect(Rect2(at + Vector2(-10, -30), Vector2(20, 8)), col, true);
	}

	// Airframe, bottom-right.
	float rx = size.x - 400.0f;
	c->text_shadowed(title, Vector2(rx, y - 86), "AIRFRAME", 22, dim);
	float h = saturate(hp / 100.0f);
	draw_bar(c, Vector2(rx, y - 66), Vector2(340, 30), h, h > 0.5f ? Color(0.5f, 0.85f, 0.45f) : (h > 0.25f ? ui::AMBER : red), "");

	// Objective, top-right.
	if (primary >= 0) {
		const GroundTarget &b = ground.targets[primary];
		c->text_shadowed(title, Vector2(size.x - 560, 70), b.alive ? "BRIDGE STANDING" : "BRIDGE DESTROYED", 40,
				b.alive ? red : ui::AMBER, 2, 500);
		draw_bar(c, Vector2(size.x - 400, 86), Vector2(340, 16), b.alive ? b.hp / b.max_hp : 0.0f, red, "");
		c->text_shadowed(title, Vector2(size.x - 560, 140),
				String("OTHER TARGETS  ") + String::num_int64(ground.destroyed() - (b.alive ? 0 : 1)), 26, dim, 2, 500);
	} else {
		int got = ground.destroyed();
		int need = required();
		c->text_shadowed(title, Vector2(size.x - 560, 70),
				String("TARGETS  ") + String::num_int64(got) + String(" / ") + String::num_int64(need), 40,
				got >= need ? ui::AMBER : white, 2, 500);
		draw_bar(c, Vector2(size.x - 400, 86), Vector2(340, 16), (float)got / MAX(need, 1), ui::AMBER, "");
	}

	// Run progress, left edge.
	float progress = saturate(-anchor_z / run_length);
	Vector2 p0(44.0f, size.y * 0.28f);
	Vector2 p1(44.0f, size.y * 0.72f);
	c->draw_line(p0, p1, Color(1, 1, 1, 0.35f), 3.0f);
	Vector2 pp = p1.lerp(p0, progress);
	PackedVector2Array tri;
	tri.push_back(pp + Vector2(-10, 8));
	tri.push_back(pp + Vector2(10, 8));
	tri.push_back(pp + Vector2(0, -12));
	c->draw_colored_polygon(tri, white);
	c->text_shadowed(title, Vector2(24, p0.y - 20.0f), String("PASS ") + String::num_int64(pass), 20, dim);

	if (flak_warning > 0.0f && !dead) {
		float blink = std::fmod(clock, 0.6f) < 0.4f ? 1.0f : 0.4f;
		c->text_shadowed(title, Vector2(0, size.y - 170.0f), "FLAK - KEEP MOVING", 34, Color(1, 0.3f, 0.2f, blink), 1, size.x);
	}

	if (turning) {
		float k = turn_fade < 1.2f ? turn_fade / 1.0f : (2.4f - turn_fade) / 1.0f;
		c->draw_rect(Rect2(Vector2(), size), Color(0, 0, 0, saturate(k)), true);
	}
}

} // namespace ww2
