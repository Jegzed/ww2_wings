#include "missions/dogfight.h"

#include "core/audio.h"
#include "core/input_setup.h"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

using namespace godot;

namespace ww2 {

namespace {
const float BULLET_SPEED = 820.0f;
const float GUN_RANGE = 520.0f;
const float MS_TO_MPH = 2.23694f;
const float M_TO_FT = 3.28084f;

Vector3 forward_of(const Basis &b) {
	return -b.get_column(2);
}
Vector3 right_of(const Basis &b) {
	return b.get_column(0);
}
Vector3 up_of(const Basis &b) {
	return b.get_column(1);
}
} // namespace

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

int Dogfight::spawn(models::PlaneType type, mats::PaintScheme scheme, int team, const Vector3 &pos, float yaw_deg,
		float skill, bool player, const String &callsign) {
	Aircraft a;
	a.type = type;
	a.team = team;
	a.player = player;
	a.callsign = callsign;
	a.paint = mats::plane_paint(scheme);
	a.node = models::make_plane(type, a.paint, true);
	add_child(a.node);
	a.pos = pos;
	a.basis = Basis(Vector3(0, 1, 0), deg2rad(yaw_deg));
	a.skill = skill;
	a.retarget = grng().range(0.0f, 1.5f);

	switch (type) {
		case models::PLANE_BF109:
			a.max_speed = 152.0f;
			a.roll_max = 105.0f;
			a.pitch_max = 45.0f;
			a.max_hp = 70.0f;
			break;
		case models::PLANE_FW190:
			a.max_speed = 160.0f;
			a.roll_max = 150.0f;
			a.pitch_max = 42.0f;
			a.max_hp = 85.0f;
			break;
		case models::PLANE_B26:
			a.role = 1;
			a.max_speed = 118.0f;
			a.roll_max = 22.0f;
			a.pitch_max = 8.0f;
			a.max_hp = 240.0f;
			break;
		case models::PLANE_JU88:
			a.role = 1;
			a.max_speed = 112.0f;
			a.roll_max = 24.0f;
			a.pitch_max = 8.0f;
			a.max_hp = 190.0f;
			break;
		case models::PLANE_V1:
			a.role = 2;
			a.max_speed = 172.0f;
			a.roll_max = 10.0f;
			a.pitch_max = 5.0f;
			a.max_hp = 30.0f;
			break;
		default:
			a.max_speed = 162.0f;
			a.roll_max = 112.0f;
			a.pitch_max = 43.0f;
			a.max_hp = 120.0f;
			break;
	}
	if (player) {
		a.max_hp = 100.0f;
	}
	a.hp = a.max_hp;
	a.speed = a.role == 0 ? a.max_speed * 0.75f : a.max_speed;
	a.throttle = a.role == 0 ? 0.8f : 1.0f;
	a.node->set_transform(Transform3D(a.basis, a.pos));
	planes.push_back(a);
	return (int)planes.size() - 1;
}

void Dogfight::build() {
	GameState &gs = GameState::get();

	WorldOptions opt;
	opt.time = def->time;
	opt.shadow_distance = 260.0f;
	opt.fog_density = 0.00011f;
	opt.cloud_cover = def->cloud_cover;
	opt.sun_azimuth_deg = 150.0f;
	world = build_world(this, opt);

	Ref<ShaderMaterial> terrain_mat;
	terrain = build_terrain(this, terrain_mat, 240000.0f);

	Rng rng(7 + def->variant * 31);
	build_clouds(this, rng, 150, 8000.0f, 900.0f, 2700.0f, 210.0f);

	const float alt = 2000.0f;
	const int variant = def->variant;
	const float skill = 0.42f + 0.08f * def->difficulty;
	const char *names[] = { "Mac", "Kowalski" };

	auto add_wingmen = [&](int count, const Vector3 &lead) {
		for (int i = 0; i < count; i++) {
			float side = i == 0 ? 1.0f : -1.0f;
			spawn(models::PLANE_P47, i == 0 ? mats::PAINT_P47_OLIVE : mats::PAINT_P47_SILVER, 0,
					lead + Vector3(45.0f * side, -12.0f, 50.0f + i * 25.0f), 0.0f, 0.62f, false, names[i]);
		}
	};

	switch (variant) {
		case DF_ESCORT: {
			// The box flies straight down the track; the fight comes to it.
			spawn(models::PLANE_P47, mats::PAINT_P47_SILVER, 0, Vector3(0, alt + 150.0f, 0), 0.0f, 0.9f, true, "You");
			add_wingmen(def->difficulty >= 3 ? 2 : 1, Vector3(0, alt + 150.0f, 0));
			bombers_total = 6;
			objective_z = -9500.0f;
			for (int i = 0; i < bombers_total; i++) {
				Vector3 slot((i % 2 == 0 ? -1.0f : 1.0f) * (30.0f + (i / 2) * 20.0f), alt - (i / 2) * 15.0f,
						-250.0f + (i / 2) * 60.0f);
				int b = spawn(models::PLANE_B26, mats::PAINT_B26, 0, slot, 0.0f, 0.3f, false, "Marauder");
				planes[b].waypoint = slot + Vector3(0, 0, -30000.0f);
			}
			waves_total = def->difficulty >= 3 ? 3 : 2;
			spawn_wave(2 + def->difficulty / 2, Vector3(0, alt, -250.0f), def->difficulty >= 3);
			wave = 1;
			spawn_timer = 38.0f;
			message("Little friends on station. Stay with the box.", 4.0f, ui::AMBER);
		} break;
		case DF_INTERCEPT: {
			spawn(models::PLANE_P47, mats::PAINT_P47_SILVER, 0, Vector3(0, 1700.0f, 0), 0.0f, 0.9f, true, "You");
			add_wingmen(def->difficulty >= 3 ? 2 : 1, Vector3(0, 1700.0f, 0));
			bombers_total = 4 + def->difficulty / 2;
			objective_z = 2600.0f; // the anchorage lies behind you
			for (int i = 0; i < bombers_total; i++) {
				Vector3 slot((i % 2 == 0 ? -1.0f : 1.0f) * (35.0f + (i / 2) * 25.0f), 950.0f - (i / 2) * 12.0f,
						-4600.0f - (i / 2) * 70.0f);
				int b = spawn(models::PLANE_JU88, mats::PAINT_JU88, 1, slot, 180.0f, 0.3f, false, "Junkers");
				planes[b].waypoint = slot + Vector3(0, 0, 30000.0f);
			}
			int escorts = def->difficulty >= 3 ? 2 : 0;
			for (int i = 0; i < escorts; i++) {
				spawn(models::PLANE_BF109, mats::PAINT_BF109_GREY, 1, Vector3((i == 0 ? -1.0f : 1.0f) * 220.0f, 1250.0f, -4300.0f),
						180.0f, skill, false, "Bandit");
			}
			initial_enemies = bombers_total + escorts;
			message("Bombers inbound, twelve o'clock low!", 4.0f, ui::AMBER);
		} break;
		case DF_DIVER: {
			spawn(models::PLANE_P47, mats::PAINT_P47_SILVER, 0, Vector3(0, 1750.0f, 0), 0.0f, 0.9f, true, "You");
			missiles_total = 4 + def->difficulty / 2;
			spawn_timer = 3.0f;
			message("Diver, diver! Flying bombs crossing the coast.", 4.0f, ui::AMBER);
		} break;
		case DF_AMBUSH: {
			spawn(models::PLANE_P47, mats::PAINT_P47_SILVER, 0, Vector3(0, alt, 0), 0.0f, 0.9f, true, "You");
			add_wingmen(1, Vector3(0, alt, 0));
			int enemies = 1 + def->difficulty;
			initial_enemies = enemies;
			for (int i = 0; i < enemies; i++) {
				bool fw = (i % 2 == 1);
				float x = (i - (enemies - 1) * 0.5f) * 90.0f;
				Vector3 pos(x, alt + 160.0f + (i % 2) * 40.0f, 700.0f + (i % 3) * 70.0f);
				int e = spawn(fw ? models::PLANE_FW190 : models::PLANE_BF109, fw ? mats::PAINT_FW190 : mats::PAINT_BF109_GREY,
						1, pos, 0.0f, skill + 0.05f, false, "Bandit");
				planes[e].target = i < 2 ? 0 : 1;
				planes[e].retarget = 6.0f;
				planes[e].burst = 0.0f;
			}
			message("Break! Bandits six o'clock high!", 4.0f, ui::RED);
		} break;
		case DF_ACE: {
			spawn(models::PLANE_P47, mats::PAINT_P47_SILVER, 0, Vector3(0, alt, 0), 0.0f, 0.9f, true, "You");
			initial_enemies = 1;
			int e = spawn(models::PLANE_BF109, mats::PAINT_BF109_ACE, 1, Vector3(300.0f, alt + 450.0f, -2200.0f), 180.0f, 0.97f,
					false, "Red Nose");
			planes[e].max_hp = planes[e].hp = 150.0f;
			planes[e].max_speed = 160.0f;
			planes[e].roll_max = 125.0f;
			planes[e].pitch_max = 50.0f;
			message("There he is. Red nose, high and to the right.", 4.0f, ui::AMBER);
		} break;
		case DF_SWEEP:
		default: {
			spawn(models::PLANE_P47, mats::PAINT_P47_SILVER, 0, Vector3(0, alt, 0), 0.0f, 0.9f, true, "You");
			add_wingmen(def->difficulty >= 3 ? 2 : 1, Vector3(0, alt, 0));
			int enemies = 2 + def->difficulty;
			if (demo) {
				enemies = MAX(enemies, 4);
			}
			initial_enemies = enemies;
			for (int i = 0; i < enemies; i++) {
				bool fw = def->difficulty >= 3 && (i % 2 == 1);
				bool ace = (i == 0 && def->difficulty >= 3);
				float x = (i - (enemies - 1) * 0.5f) * 140.0f;
				float dist = demo ? 900.0f : 2600.0f;
				Vector3 pos(x, alt + 150.0f + (i % 3) * 60.0f, -dist - (i % 2) * 180.0f);
				spawn(fw ? models::PLANE_FW190 : models::PLANE_BF109,
						fw ? mats::PAINT_FW190 : (ace ? mats::PAINT_BF109_ACE : mats::PAINT_BF109_GREY), 1, pos, 180.0f,
						ace ? skill + 0.15f : skill, false, "Bandit");
			}
			message("Bandits, twelve o'clock high!", 4.0f, ui::AMBER);
		} break;
	}

	bullets.setup(this, 500, 16.0f, 0.16f);

	camera = memnew(Camera3D);
	camera->set_near(1.0f);
	camera->set_far(40000.0f);
	camera->set_fov(cam_fov);
	add_child(camera);
	camera->make_current();

	ammo = 1500 + gs.pilot.skills[SKILL_MECHANICAL] * 50;

	snd_engine = audio::make_loop(this, "engine", -9.0f);
	snd_engine->play();
	snd_wind = audio::make_loop(this, "wind", -16.0f);
	snd_wind->play();
	snd_gun = audio::make_loop(this, "gun", -5.0f);
	snd_enemy_gun = audio::make_loop(this, "cannon", -12.0f);
	snd_alarm = audio::make_loop(this, "alarm", -14.0f);
}

void Dogfight::spawn_wave(int count, const Vector3 &around, bool fw_mix) {
	Rng &rng = grng();
	const float skill = 0.42f + 0.08f * def->difficulty;
	float side = rng.chance(0.5f) ? 1.0f : -1.0f;
	for (int i = 0; i < count; i++) {
		bool fw = fw_mix && (i % 2 == 1);
		Vector3 pos = around + Vector3(side * 1400.0f + i * 120.0f * side, 500.0f + (i % 2) * 90.0f, -2400.0f - i * 150.0f);
		int e = spawn(fw ? models::PLANE_FW190 : models::PLANE_BF109, fw ? mats::PAINT_FW190 : mats::PAINT_BF109_GREY, 1,
				pos, 180.0f + side * 25.0f, skill, false, "Bandit");
		planes[e].node->set_transform(Transform3D(planes[e].basis, planes[e].pos));
	}
	initial_enemies += count;
	if (wave > 0) {
		message("More bandits coming in!", 3.0f, ui::RED);
	}
}

// ---------------------------------------------------------------------------
// Control
// ---------------------------------------------------------------------------

Dogfight::Controls Dogfight::control_player(Aircraft &a, float dt) {
	Controls c;
	Input *in = Input::get_singleton();
	c.roll = input_x();
	c.pitch = input_y();
	// Hands off the stick, the ship eases back toward wings-level.
	if (std::fabs(c.roll) < 0.05f) {
		float bank = std::asin(clampf(-right_of(a.basis).y, -1.0f, 1.0f));
		if (up_of(a.basis).y > 0.2f) {
			c.roll = clampf(-bank * 1.2f, -0.3f, 0.3f);
		}
	}
	if (in->is_action_pressed("throttle_up")) {
		a.throttle = MIN(a.throttle + dt * 0.7f, 1.0f);
	}
	if (in->is_action_pressed("throttle_down")) {
		a.throttle = MAX(a.throttle - dt * 0.7f, 0.15f);
	}
	c.throttle = a.throttle;
	c.fire = in->is_action_pressed("fire");
	a.assist = Vector3();
	return c;
}

Dogfight::Controls Dogfight::control_ai(int index, float dt) {
	Aircraft &a = planes[index];
	Rng &rng = grng();
	Controls c;
	c.throttle = 0.9f;

	Vector3 fwd = forward_of(a.basis);
	Vector3 right = right_of(a.basis);

	// Bombers and flying bombs hold course for their waypoint and never fight back with the stick.
	if (a.role != 0) {
		c.throttle = 1.0f;
		Vector3 to = a.waypoint - a.pos;
		to.y = 0.0f;
		Vector3 goal = to.length_squared() > 1.0f ? to.normalized() : fwd;
		// Hold altitude.
		goal.y = clampf((a.waypoint.y - a.pos.y) * 0.002f, -0.08f, 0.08f);
		goal.normalize();
		Vector3 local = a.basis.xform_inv(goal);
		float bank = std::asin(clampf(-right.y, -1.0f, 1.0f));
		c.roll = clampf(local.x * 3.0f - bank * 1.5f, -1.0f, 1.0f);
		c.pitch = clampf(local.y * 6.0f, -1.0f, 1.0f);
		a.assist = goal;
		return c;
	}

	// Target selection.
	a.retarget -= dt;
	bool target_ok = a.target >= 0 && planes[a.target].alive && !planes[a.target].dying;
	if (!target_ok || a.retarget <= 0.0f) {
		a.retarget = rng.range(2.5f, 5.0f);
		float best = 1e12f;
		int best_i = -1;
		for (int i = 0; i < (int)planes.size(); i++) {
			const Aircraft &o = planes[i];
			if (i == index || o.team == a.team || !o.alive || o.dying) {
				continue;
			}
			float d = o.pos.distance_squared_to(a.pos);
			if (o.role == 1) {
				d *= 0.3f; // bombers are what everyone is here for
			}
			if (o.role == 2) {
				d *= 0.6f;
			}
			// Count how many friends already chase this one, to spread out.
			int chasers = 0;
			for (int k = 0; k < (int)planes.size(); k++) {
				if (k != index && planes[k].alive && planes[k].team == a.team && planes[k].target == i) {
					chasers++;
				}
			}
			d *= 1.0f + chasers * 1.2f;
			if (o.player && a.team == 1) {
				if (chasers >= 2) {
					continue; // never more than two on the player's tail
				}
				d *= 0.85f; // the player draws attention
			}
			if (d < best) {
				best = d;
				best_i = i;
			}
		}
		a.target = best_i;
	}

	Vector3 goal = fwd;
	a.assist = Vector3();
	bool recovering = false;

	if (a.pos.y < 260.0f && fwd.y < 0.15f) {
		Vector3 flat(fwd.x, 0.0f, fwd.z);
		if (flat.length_squared() < 0.01f) {
			flat = Vector3(0, 0, -1);
		}
		goal = (flat.normalized() + Vector3(0, 0.7f, 0)).normalized();
		c.throttle = 1.0f;
		recovering = true;
	} else if (a.pos.y > 3600.0f) {
		Vector3 flat(fwd.x, 0.0f, fwd.z);
		goal = (flat.normalized() + Vector3(0, -0.4f, 0)).normalized();
	} else if (a.evade_time > 0.0f) {
		a.evade_time -= dt;
		goal = a.evade_dir;
		c.throttle = 1.0f;
	} else if (a.target >= 0) {
		const Aircraft &t = planes[a.target];
		Vector3 to = t.pos - a.pos;
		float dist = to.length();
		Vector3 tvel = forward_of(t.basis) * t.speed;
		float lead = dist / BULLET_SPEED;
		Vector3 aim = t.pos + tvel * lead * (0.5f + 0.5f * a.skill);
		goal = (aim - a.pos).normalized();
		float off = std::acos(clampf(fwd.dot(goal), -1.0f, 1.0f));

		if (dist < GUN_RANGE * 0.85f && off < deg2rad(3.0f + (1.0f - a.skill) * 5.0f)) {
			a.burst = rng.range(0.4f, 0.9f);
		}
		if (off < deg2rad(12.0f)) {
			a.assist = goal;
		}
		if (dist < (t.role == 1 ? 60.0f : 95.0f)) {
			// Too close: break away rather than ram.
			a.evade_time = rng.range(1.5f, 3.0f);
			float side = rng.chance(0.5f) ? 1.0f : -1.0f;
			a.evade_dir = (fwd + right * side * 0.9f + Vector3(0, rng.range(0.1f, 0.6f), 0)).normalized();
		}
		if (dist < 350.0f && a.speed > t.speed + 5.0f && off < deg2rad(30.0f)) {
			c.throttle = 0.45f;
		}
		// A good pilot will not trade shots head-on.
		if (a.skill > 0.85f && dist < 900.0f && forward_of(t.basis).dot(fwd) < -0.6f && off < deg2rad(20.0f)) {
			a.evade_time = rng.range(1.2f, 2.0f);
			float side = rng.chance(0.5f) ? 1.0f : -1.0f;
			a.evade_dir = (right * side * 0.6f + fwd * 0.4f + Vector3(0, 0.7f, 0)).normalized();
		}

		// Someone on my tail?
		for (int i = 0; i < (int)planes.size(); i++) {
			const Aircraft &o = planes[i];
			if (o.team == a.team || !o.alive || o.dying) {
				continue;
			}
			Vector3 rel = a.pos - o.pos;
			float d = rel.length();
			if (d > 450.0f) {
				continue;
			}
			bool behind = fwd.dot(rel) > 0.0f;
			bool pointing = forward_of(o.basis).dot(rel / MAX(d, 1.0f)) > 0.96f;
			if (behind && pointing && rng.chance(dt * (0.5f + a.skill * 1.5f))) {
				a.evade_time = rng.range(2.0f, 3.5f);
				float side = rng.chance(0.5f) ? 1.0f : -1.0f;
				float vertical = a.pos.y > 700.0f ? rng.range(-0.6f, 0.5f) : rng.range(0.1f, 0.6f);
				a.evade_dir = (right * side + fwd * 0.25f + Vector3(0, vertical, 0)).normalized();
			}
		}
	} else {
		// Nothing to do: gentle orbit.
		goal = (fwd + right * 0.3f).normalized();
		goal.y = 0.0f;
		goal.normalize();
	}

	// Steer toward the goal: roll to put it "above" the nose, then pull.
	Vector3 local = a.basis.xform_inv(goal);
	float off = std::atan2(std::sqrt(local.x * local.x + local.y * local.y), -local.z);
	float roll_err = std::atan2(local.x, local.y);
	if (off < deg2rad(7.0f)) {
		// Fine tracking: keep wings roughly level with the horizon and nudge.
		float bank = std::asin(clampf(-right.y, -1.0f, 1.0f));
		c.roll = clampf(local.x * 8.0f - bank * 0.5f, -1.0f, 1.0f);
		c.pitch = clampf(local.y * 14.0f, -1.0f, 1.0f);
	} else {
		c.roll = clampf(roll_err * 1.8f, -1.0f, 1.0f);
		float align = std::cos(roll_err);
		c.pitch = align > 0.25f ? clampf(off * 3.0f, 0.0f, 1.0f) * align : 0.0f;
	}
	float agility = recovering ? 1.0f : (0.62f + 0.38f * a.skill);
	c.pitch *= agility;

	if (a.burst > 0.0f) {
		a.burst -= dt;
		c.fire = true;
	}
	return c;
}

void Dogfight::simulate(Aircraft &a, const Controls &c, float dt) {
	GameState &gs = GameState::get();
	float handling = a.player ? gs.handling() : (a.skill > 0.9f ? 1.12f : 1.0f);
	float eff = clampf(a.speed / 85.0f, 0.25f, 1.0f);
	// Controls stiffen at very high speed.
	eff *= 1.0f - 0.35f * saturate((a.speed - a.max_speed) / (a.max_speed * 0.35f));
	float damage_k = 0.65f + 0.35f * saturate(a.hp / a.max_hp);

	float max_roll = deg2rad(a.roll_max) * eff * handling * damage_k;
	float max_pitch = deg2rad(a.pitch_max) * eff * handling * damage_k;
	float roll_target = clampf(c.roll, -1.0f, 1.0f) * max_roll;
	float pitch_target = c.pitch > 0.0f ? c.pitch * max_pitch : c.pitch * max_pitch * 0.55f;
	a.roll_rate = damp(a.roll_rate, roll_target, 6.0f, dt);
	a.pitch_rate = damp(a.pitch_rate, pitch_target, 5.0f, dt);

	Basis b = a.basis;
	b = b * Basis(Vector3(1, 0, 0), a.pitch_rate * dt);
	b = b * Basis(Vector3(0, 0, 1), -a.roll_rate * dt);

	// Banked wings pull the nose around the horizon.
	Vector3 right = right_of(b);
	float yaw = right.y * 0.5f * eff;
	b = Basis(Vector3(0, 1, 0), yaw * dt) * b;

	// Stall: the nose falls through.
	Vector3 fwd = forward_of(b);
	if (a.speed < 52.0f) {
		Vector3 axis = fwd.cross(Vector3(0, -1, 0));
		if (axis.length_squared() > 1e-4f) {
			b = Basis(axis.normalized(), (1.0f - a.speed / 52.0f) * 1.4f * dt + 0.25f * dt) * b;
		}
	}

	// AI "rudder": small direct correction so it can actually aim.
	if (a.assist.length_squared() > 0.5f) {
		Vector3 f = forward_of(b);
		Vector3 axis = f.cross(a.assist);
		float ang = std::asin(clampf(axis.length(), 0.0f, 1.0f));
		if (ang > 1e-4f) {
			float step = MIN(ang, deg2rad(14.0f) * a.skill * dt);
			b = Basis(axis.normalized(), step) * b;
		}
	}
	b.orthonormalize();
	a.basis = b;

	fwd = forward_of(b);
	float target_speed = lerpf(58.0f, a.max_speed, clampf(c.throttle, 0.0f, 1.0f)) * (0.75f + 0.25f * damage_k);
	a.speed = damp(a.speed, target_speed, 0.35f, dt);
	a.speed += -fwd.y * 9.8f * 1.7f * dt;
	a.speed -= std::fabs(a.pitch_rate) * a.speed * 0.13f * dt;
	a.speed = clampf(a.speed, 28.0f, a.max_speed * 1.4f);
	a.pos += fwd * a.speed * dt;

	// Soft ceiling.
	if (a.pos.y > 4200.0f) {
		a.pos.y = 4200.0f;
	}
}

void Dogfight::fire_guns(int index, float dt) {
	Aircraft &a = planes[index];
	GameState &gs = GameState::get();
	a.fire_timer -= dt;
	if (!a.firing) {
		a.fire_timer = MAX(a.fire_timer, 0.0f);
		return;
	}
	const models::PlaneInfo &info = models::plane_info(a.type);
	float interval = a.player ? 1.0f / 24.0f : 1.0f / 11.0f;
	Rng &rng = grng();
	while (a.fire_timer <= 0.0f) {
		a.fire_timer += interval;
		if (a.player) {
			if (ammo <= 0) {
				break;
			}
			ammo--;
			result.shots_fired++;
		}
		Vector3 muzzle_local = (a.gun_side++ % 2 == 0) ? info.gun_left : info.gun_right;
		Vector3 muzzle = a.pos + a.basis.xform(muzzle_local);
		Vector3 fwd = forward_of(a.basis);
		// Guns are harmonised to converge ahead of the nose.
		Vector3 converge = a.pos + fwd * 320.0f;
		Vector3 dir = (converge - muzzle).normalized();
		float spread = a.player ? 0.0045f * gs.accuracy() : 0.011f + (1.0f - a.skill) * 0.02f;
		dir = (dir + rng.in_sphere() * spread).normalized();
		float dmg = a.player ? 9.0f * (0.8f + 0.04f * gs.pilot.skills[SKILL_SHOOTING])
							 : (a.team == 0 ? 3.5f : 2.0f + def->difficulty * 0.5f);
		bullets.fire(muzzle + dir * 4.0f, dir * BULLET_SPEED + fwd * a.speed, 1.1f, dmg, a.team, index);
	}
	if (a.player) {
		shake(0.12f);
	}
}

void Dogfight::update_bullets(float dt) {
	bullets.step(dt, 3.0f);
	GameState &gs = GameState::get();
	for (fx::Bullet &b : bullets.list()) {
		if (!b.alive) {
			continue;
		}
		if (b.pos.y <= 0.0f) {
			b.alive = false;
			continue;
		}
		Vector3 seg = b.pos - b.prev;
		float seg_len = seg.length();
		if (seg_len < 1e-4f) {
			continue;
		}
		Vector3 dir = seg / seg_len;
		for (int i = 0; i < (int)planes.size(); i++) {
			Aircraft &p = planes[i];
			if (!p.alive || p.team == b.team || i == b.owner) {
				continue;
			}
			float radius = models::plane_info(p.type).hit_radius;
			if (b.owner == 0) {
				radius *= 1.35f; // a little generosity for the player
			}
			if (b.owner >= 0 && planes[b.owner].role == 1 && p.player) {
				radius *= 0.6f; // gunners are not that good
			}
			Vector3 rel = p.pos - b.prev;
			float t = clampf(rel.dot(dir), 0.0f, seg_len);
			Vector3 closest = b.prev + dir * t;
			if (closest.distance_squared_to(p.pos) <= radius * radius) {
				b.alive = false;
				float dmg = b.damage;
				if (p.player) {
					dmg *= gs.toughness();
				}
				fx::spark(this, closest, 1.2f);
				if (b.owner == 0) {
					result.shots_hit++;
					audio::play("hit", -9.0f, grng().range(0.9f, 1.2f));
				}
				damage(i, dmg, b.owner);
				break;
			}
		}
	}
	bullets.render();
}

void Dogfight::damage(int victim, float amount, int attacker) {
	Aircraft &v = planes[victim];
	if (!v.alive || v.dying) {
		return;
	}
	v.hp -= amount;
	v.last_attacker = attacker;
	if (v.player) {
		shake(0.55f);
		hurt_flash(0.5f);
		audio::play("ricochet", -6.0f, grng().range(0.85f, 1.15f));
	}
	// A hit plane usually breaks hard.
	if (!v.player && v.evade_time <= 0.0f && grng().chance(0.25f + 0.4f * v.skill)) {
		v.evade_time = grng().range(1.5f, 3.0f);
		float side = grng().chance(0.5f) ? 1.0f : -1.0f;
		v.evade_dir = (right_of(v.basis) * side + Vector3(0, grng().range(-0.4f, 0.5f), 0)).normalized();
	}
	update_smoke(v);
	if (v.hp <= 0.0f) {
		kill(victim, attacker);
	}
}

void Dogfight::update_smoke(Aircraft &a) {
	float ratio = a.hp / a.max_hp;
	a.paint->set_shader_parameter("damage", saturate(1.0f - ratio));
	int level = ratio < 0.3f ? 2 : (ratio < 0.65f ? 1 : 0);
	if (a.dying) {
		level = 3;
	}
	if (level == a.smoke_level) {
		return;
	}
	a.smoke_level = level;
	if (a.smoke) {
		a.smoke->set_emitting(false);
		a.smoke = nullptr; // left to fade; freed with the plane node
	}
	const models::PlaneInfo &info = models::plane_info(a.type);
	if (level == 1) {
		a.smoke = fx::smoke_trail(a.node, Color(0.75f, 0.75f, 0.78f, 0.45f), 3.0f, 2.2f, 90);
	} else if (level == 2) {
		a.smoke = fx::smoke_trail(a.node, Color(0.12f, 0.12f, 0.13f, 0.75f), 4.5f, 3.0f, 130);
	} else if (level == 3) {
		a.smoke = fx::smoke_trail(a.node, Color(0.05f, 0.05f, 0.05f, 0.9f), 7.0f, 4.5f, 200);
		a.fire = fx::fire_trail(a.node, 3.5f);
		a.fire->set_position(Vector3(0, 0, -2.0f));
	}
	if (a.smoke) {
		a.smoke->set_position(info.exhaust);
	}
}

float Dogfight::distance_gain(const Vector3 &pos) const {
	float d = cam_pos.distance_to(pos);
	return -clampf(d / 60.0f, 0.0f, 30.0f);
}

void Dogfight::kill(int victim, int attacker) {
	Aircraft &v = planes[victim];
	v.dying = true;
	v.dying_time = 0.0f;
	v.spin = grng().chance(0.5f) ? 1.0f : -1.0f;
	v.hp = 0.0f;
	if (v.role == 2) {
		// A flying bomb goes off all at once, and takes anything close with it.
		fx::explosion(this, v.pos, 14.0f, false);
		audio::play("explosion_big", 2.0f + distance_gain(v.pos), grng().range(0.9f, 1.1f));
		v.dying = false;
		v.alive = false;
		v.node->queue_free();
		v.node = nullptr;
		missiles_down++;
		for (int i = 0; i < (int)planes.size(); i++) {
			Aircraft &o = planes[i];
			if (!o.alive || o.role == 2) {
				continue;
			}
			float d = o.pos.distance_to(v.pos);
			if (d < 110.0f) {
				damage(i, 45.0f * (1.0f - d / 110.0f), -1);
				if (o.player) {
					shake(1.0f);
				}
			}
		}
		if (attacker == 0) {
			result.air_kills++;
			result.score += 250;
			message("Diver destroyed!", 3.0f, ui::AMBER);
		} else {
			message("Flying bomb destroyed", 3.0f);
		}
		return;
	}
	update_smoke(v);
	fx::explosion(this, v.pos, v.role == 1 ? 8.0f : 5.0f, false);
	audio::play("explosion", -2.0f + distance_gain(v.pos), grng().range(0.9f, 1.1f));

	if (v.player) {
		message("You're hit! Going down!", 4.0f, ui::RED);
		return;
	}
	if (v.team == 1) {
		if (attacker == 0) {
			result.air_kills++;
			result.score += v.role == 1 ? 450 : 300;
			if (v.role == 1) {
				message("Bomber going down!", 3.0f, ui::AMBER);
				return;
			}
			static const char *lines[] = { "Splash one!", "Got him! He's going down!", "Scratch one bandit!",
				"He's burning!" };
			message(lines[grng().irange(0, 3)], 3.0f, ui::AMBER);
		} else if (attacker >= 0) {
			message(planes[attacker].callsign + String(" got one!"), 3.0f);
		}
	} else if (v.role == 1) {
		message("We lost a Marauder!", 4.0f, ui::RED);
	} else {
		message(v.callsign + String(" is hit! He's going down!"), 4.0f, ui::RED);
	}
}

void Dogfight::update_dying(int index, float dt) {
	Aircraft &a = planes[index];
	a.dying_time += dt;
	Basis b = a.basis;
	b = b * Basis(Vector3(0, 0, 1), a.spin * deg2rad(170.0f) * dt);
	Vector3 fwd = forward_of(b);
	Vector3 axis = fwd.cross(Vector3(0, -1, 0));
	if (axis.length_squared() > 1e-4f) {
		b = Basis(axis.normalized(), 0.45f * dt) * b;
	}
	b.orthonormalize();
	a.basis = b;
	a.speed = MIN(a.speed + 25.0f * dt, 190.0f);
	a.pos += forward_of(b) * a.speed * dt + Vector3(0, -12.0f * a.dying_time, 0) * dt;

	bool ground = a.pos.y <= 3.0f;
	if (ground || a.dying_time > 9.0f) {
		Vector3 p = a.pos;
		if (ground) {
			p.y = 1.0f;
		}
		fx::explosion(this, p, ground ? 10.0f : 6.0f, ground);
		if (ground) {
			fx::smoke_column(this, p, 4.0f, true);
		}
		audio::play("explosion_big", 0.0f + distance_gain(p), grng().range(0.85f, 1.05f));
		a.alive = false;
		a.dying = false;
		if (a.player) {
			a.node->set_visible(false);
			shake(1.2f);
			player_crashed = true;
			String where = String(def->location).get_slice(",", 0);
			player_down(String("Shot down in flames over ") + where + String("."), 3.0f);
		} else {
			a.node->queue_free();
			a.node = nullptr;
		}
	}
}

// Rear gunners on bombers shoot at fighters sitting behind them.
void Dogfight::update_gunners(float dt) {
	Rng &rng = grng();
	for (int i = 0; i < (int)planes.size(); i++) {
		Aircraft &b = planes[i];
		if (!b.alive || b.dying || b.role != 1) {
			continue;
		}
		b.gun_timer -= dt;
		if (b.gun_timer > 0.0f) {
			continue;
		}
		Vector3 fwd = forward_of(b.basis);
		int best = -1;
		float best_d = 420.0f;
		for (int k = 0; k < (int)planes.size(); k++) {
			const Aircraft &o = planes[k];
			if (!o.alive || o.dying || o.team == b.team || o.role != 0) {
				continue;
			}
			Vector3 rel = o.pos - b.pos;
			float d = rel.length();
			if (d < best_d && fwd.dot(rel / MAX(d, 1.0f)) < 0.25f) {
				best_d = d;
				best = k;
			}
		}
		if (best < 0) {
			b.gun_timer = 0.3f;
			continue;
		}
		b.gun_timer = rng.range(0.9f, 1.6f);
		const Aircraft &t = planes[best];
		Vector3 muzzle = b.pos + b.basis.xform(models::plane_info(b.type).turret);
		Vector3 aim = t.pos + forward_of(t.basis) * t.speed * (best_d / 600.0f);
		Vector3 dir = (aim - muzzle).normalized();
		for (int s = 0; s < 4; s++) {
			Vector3 d = (dir + rng.in_sphere() * 0.035f).normalized();
			bullets.fire(muzzle + d * (3.0f + s * 6.0f), d * 600.0f + forward_of(b.basis) * b.speed, 0.7f,
					b.team == 1 ? 3.0f + def->difficulty * 0.5f : 4.0f, b.team, i);
		}
		if (t.player) {
			enemy_fire_heard = 0.25f;
		}
	}
}

// Variant-specific win and loss conditions.
void Dogfight::update_objective(float dt) {
	if (ending || objective_done) {
		return;
	}
	Rng &rng = grng();
	const Aircraft &player = planes[0];
	switch (def->variant) {
		case DF_ESCORT: {
			// Later waves arrive as the box presses on.
			if (wave < waves_total) {
				spawn_timer -= dt;
				if (spawn_timer <= 0.0f) {
					Vector3 around;
					int n = 0;
					for (const Aircraft &a : planes) {
						if (a.role == 1 && a.alive) {
							around += a.pos;
							n++;
						}
					}
					around = n > 0 ? around / (float)n : player.pos;
					spawn_wave(2 + def->difficulty / 2 + (wave == 2 ? 1 : 0), around, def->difficulty >= 2);
					wave++;
					spawn_timer = 40.0f;
				}
			}
			int alive = 0;
			float lead_z = 1e9f;
			for (const Aircraft &a : planes) {
				if (a.role == 1 && a.alive && !a.dying) {
					alive++;
					lead_z = MIN(lead_z, a.pos.z);
				}
			}
			if (alive == 0) {
				objective_done = true;
				message("The box is gone. Nothing left to escort.", 5.0f, ui::RED);
				finish(false, 5.0f);
			} else if (lead_z < objective_z) {
				objective_done = true;
				bool ok = alive * 2 >= bombers_total;
				message(ok ? "Bombs away! The Marauders are through." : "Bombs away, but the box paid for it.", 5.0f,
						ok ? ui::AMBER : ui::RED);
				result.score += ok ? 500 + alive * 100 : alive * 60;
				finish(ok, 6.0f);
			} else if (wave >= waves_total && count_alive(1) == 0) {
				bool falling = false;
				for (const Aircraft &a : planes) {
					if (a.team == 1 && a.alive) {
						falling = true;
					}
				}
				if (!falling) {
					objective_done = true;
					bool ok = alive * 2 >= bombers_total;
					message("The box is clear all the way to the target.", 5.0f, ui::AMBER);
					result.score += ok ? 500 + alive * 100 : alive * 60;
					finish(ok, 5.0f);
				}
			}
		} break;
		case DF_INTERCEPT: {
			int pending = 0;
			for (Aircraft &a : planes) {
				if (a.role != 1) {
					continue;
				}
				if (a.alive && !a.dying && !a.through && a.pos.z > objective_z) {
					a.through = true;
					bombers_through++;
					fx::explosion(this, Vector3(a.pos.x, 2.0f, a.pos.z + 300.0f), 14.0f, true);
					fx::smoke_column(this, Vector3(a.pos.x, 2.0f, a.pos.z + 300.0f), 6.0f, true);
					message("A bomber got through to the ships!", 4.0f, ui::RED);
					a.alive = false;
					if (a.node) {
						a.node->queue_free();
						a.node = nullptr;
					}
				}
				if (a.alive) {
					pending++;
				}
			}
			if (pending == 0) {
				objective_done = true;
				bool ok = bombers_through * 3 <= bombers_total;
				message(ok ? "That's the last of them. The anchorage is safe." : "Too many got through.", 5.0f,
						ok ? ui::AMBER : ui::RED);
				result.score += ok ? 500 : 0;
				finish(ok, 5.0f);
			}
		} break;
		case DF_DIVER: {
			if (missiles_spawned < missiles_total) {
				spawn_timer -= dt;
				if (spawn_timer <= 0.0f) {
					spawn_timer = rng.range(11.0f, 16.0f);
					Vector3 pos(player.pos.x + rng.range(-400.0f, 400.0f), rng.range(550.0f, 750.0f), player.pos.z + 1500.0f);
					int m = spawn(models::PLANE_V1, mats::PAINT_V1, 1, pos, 0.0f, 0.3f, false, "Diver");
					planes[m].waypoint = pos + Vector3(rng.range(-600.0f, 600.0f), 0, -40000.0f);
					planes[m].node->set_transform(Transform3D(planes[m].basis, pos));
					missiles_spawned++;
					message("Diver coming through, below and behind!", 3.0f, ui::AMBER);
				}
			}
			for (Aircraft &a : planes) {
				if (a.role == 2 && a.alive && a.pos.z < player.pos.z - 4500.0f) {
					a.alive = false;
					missiles_gone++;
					if (a.node) {
						a.node->queue_free();
						a.node = nullptr;
					}
					message("One got away toward London.", 3.0f, ui::RED);
				}
			}
			if (missiles_spawned >= missiles_total && missiles_down + missiles_gone >= missiles_total) {
				objective_done = true;
				bool ok = missiles_down * 5 >= missiles_total * 3;
				message(ok ? "Patrol over. Good shooting." : "Patrol over. Not enough of them stopped.", 5.0f,
						ok ? ui::AMBER : ui::RED);
				result.score += ok ? 400 : 0;
				finish(ok, 5.0f);
			}
		} break;
		default:
			break;
	}
}

String Dogfight::objective_text() const {
	switch (def->variant) {
		case DF_ESCORT: {
			int alive = 0;
			for (const Aircraft &a : planes) {
				if (a.role == 1 && a.alive && !a.dying) {
					alive++;
				}
			}
			return String("MARAUDERS  ") + String::num_int64(alive) + String(" / ") + String::num_int64(bombers_total);
		}
		case DF_INTERCEPT: {
			int down = 0;
			for (const Aircraft &a : planes) {
				if (a.role == 1 && (!a.alive || a.dying) && !a.through) {
					down++;
				}
			}
			return String("BOMBERS DOWN ") + String::num_int64(down) + String("   THROUGH ") + String::num_int64(bombers_through) +
					String("   OF ") + String::num_int64(bombers_total);
		}
		case DF_DIVER:
			return String("DIVERS DOWN  ") + String::num_int64(missiles_down) + String(" / ") + String::num_int64(missiles_total);
		default:
			return String("BANDITS  ") + String::num_int64(count_alive(1));
	}
}

int Dogfight::count_alive(int team) const {
	int n = 0;
	for (const Aircraft &a : planes) {
		if (a.team == team && a.alive && !a.dying) {
			n++;
		}
	}
	return n;
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void Dogfight::tick(float dt) {
	bool any_enemy_fire = false;
	for (int i = 0; i < (int)planes.size(); i++) {
		Aircraft &a = planes[i];
		if (!a.alive) {
			continue;
		}
		if (a.dying) {
			a.firing = false;
			update_dying(i, dt);
		} else {
			Controls c = (a.player && !demo) ? control_player(a, dt) : control_ai(i, dt);
			if (a.player && demo) {
				a.throttle = c.throttle;
			}
			if (a.player && ending) {
				c.fire = false;
			}
			simulate(a, c, dt);
			a.firing = c.fire && (!a.player || ammo > 0);
			fire_guns(i, dt);
			if (a.firing && !a.player && a.team == 1 && a.pos.distance_to(planes[0].pos) < 700.0f) {
				any_enemy_fire = true;
			}

			// Flying into the ground.
			if (a.pos.y < 4.0f) {
				a.pos.y = 1.0f;
				fx::explosion(this, a.pos, 10.0f, true);
				fx::smoke_column(this, a.pos, 4.0f, true);
				audio::play("explosion_big", distance_gain(a.pos));
				a.alive = false;
				if (a.player) {
					a.node->set_visible(false);
					shake(1.4f);
					player_crashed = true;
					String where = String(def->location).get_slice(",", 0);
					player_down(String("Flew into the ground near ") + where + String("."), 3.0f);
				} else {
					if (a.team == 1 && a.last_attacker == 0) {
						result.air_kills++;
						result.score += 300;
						message("He flew into the ground!", 3.0f, ui::AMBER);
					}
					a.node->queue_free();
					a.node = nullptr;
				}
			}
		}
		if (a.node) {
			// Distant aircraft are drawn larger than life so they can be seen and fought.
			float scale = 1.0f;
			if (!a.player && cam_ready) {
				float d = a.pos.distance_to(cam_pos);
				scale = 1.0f + saturate((d - 120.0f) / 500.0f) * 1.3f;
			}
			a.node->set_transform(Transform3D(a.basis.scaled_local(Vector3(scale, scale, scale)), a.pos));
		}
	}

	update_gunners(dt);
	update_bullets(dt);

	enemy_fire_heard = any_enemy_fire ? 0.25f : enemy_fire_heard - dt;

	const Aircraft &player = planes[0];
	Vector3 snap(std::floor(player.pos.x / 1000.0f) * 1000.0f, 0.0f, std::floor(player.pos.z / 1000.0f) * 1000.0f);
	terrain->set_position(snap);

	update_camera(dt);
	update_audio(dt);

	if (!ending && def->variant != DF_ESCORT && def->variant != DF_INTERCEPT && ammo <= 0 && planes[0].alive && !planes[0].dying && count_alive(1) > 0) {
		empty_time += dt;
		if (empty_time > 5.0f) {
			bool ok = result.air_kills * 2 >= initial_enemies;
			message("Guns empty. Breaking off for home.", 4.0f, ok ? ui::AMBER : ui::RED);
			if (ok) {
				result.score += 200;
			}
			finish(ok, 4.0f);
		}
	}

	update_objective(dt);

	bool sweep_rules = def->variant != DF_ESCORT && def->variant != DF_INTERCEPT && def->variant != DF_DIVER;
	if (!ending && sweep_rules && count_alive(1) == 0) {
		bool all_gone = true;
		for (const Aircraft &a : planes) {
			if (a.team == 1 && a.alive) {
				all_gone = false; // still falling
			}
		}
		victory_delay += dt;
		if (all_gone || victory_delay > 4.0f) {
			message("The sky is ours. Heading home.", 5.0f, ui::AMBER);
			result.score += 400;
			finish(true, 5.0f);
		}
	}
}

void Dogfight::update_camera(float dt) {
	const Aircraft &p = planes[0];
	Vector3 fwd = forward_of(p.basis);
	Vector3 up = up_of(p.basis);

	Vector3 blend_up = (Vector3(0, 1, 0) * 0.6f + up * 0.4f);
	if (blend_up.length_squared() < 0.05f) {
		blend_up = Vector3(0, 1, 0);
	}
	blend_up.normalize();

	Vector3 want;
	Vector3 look;
	if (p.dying || !p.alive) {
		// Pull away and watch the fall.
		want = cam_pos + Vector3(0, 6.0f, 0) * dt;
		look = p.pos;
		cam_pos = damp(cam_pos, want, 1.0f, dt);
		cam_up = damp(cam_up, Vector3(0, 1, 0), 2.0f, dt).normalized();
	} else {
		want = p.pos - fwd * 17.5f + blend_up * 5.0f;
		look = p.pos + fwd * 60.0f + blend_up * 2.0f;
		if (!cam_ready) {
			cam_pos = want;
			cam_up = blend_up;
			cam_ready = true;
		}
		// Follow tightly along the flight path, loosely across it.
		cam_pos = damp(cam_pos, want, 9.0f, dt);
		cam_up = damp(cam_up, blend_up, 3.5f, dt).normalized();
	}
	if (cam_pos.y < 3.0f) {
		cam_pos.y = 3.0f;
	}
	Vector3 eye = cam_pos + shake_offset() * 1.2f;
	if (eye.distance_squared_to(look) > 0.01f) {
		Vector3 dir = (look - eye).normalized();
		Vector3 use_up = cam_up;
		if (std::fabs(dir.dot(use_up)) > 0.98f) {
			use_up = up;
		}
		camera->look_at_from_position(eye, look, use_up);
	}
	float want_fov = 56.0f + saturate((p.speed - 90.0f) / 100.0f) * 14.0f;
	cam_fov = damp(cam_fov, want_fov, 2.0f, dt);
	camera->set_fov(cam_fov);
}

void Dogfight::update_audio(float dt) {
	const Aircraft &p = planes[0];
	bool flying = p.alive;
	if (snd_engine) {
		float pitch = 0.7f + p.throttle * 0.3f + p.speed / 450.0f;
		if (p.dying) {
			pitch = 1.3f + p.dying_time * 0.08f;
		}
		snd_engine->set_pitch_scale(pitch);
		snd_engine->set_volume_db(flying ? -9.0f : -60.0f);
	}
	if (snd_wind) {
		snd_wind->set_volume_db(flying ? lerpf(-26.0f, -9.0f, saturate(p.speed / 200.0f)) : -60.0f);
		snd_wind->set_pitch_scale(0.8f + p.speed / 300.0f);
	}
	if (snd_gun) {
		bool want = p.firing && flying && !p.dying;
		if (want && !snd_gun->is_playing()) {
			snd_gun->play();
		} else if (!want && snd_gun->is_playing()) {
			snd_gun->stop();
		}
	}
	if (snd_enemy_gun) {
		bool want = enemy_fire_heard > 0.0f;
		if (want && !snd_enemy_gun->is_playing()) {
			snd_enemy_gun->play();
		} else if (!want && snd_enemy_gun->is_playing()) {
			snd_enemy_gun->stop();
		}
	}
	if (snd_alarm) {
		bool low = flying && !p.dying && p.pos.y < 180.0f && forward_of(p.basis).y < -0.05f;
		if (low && !snd_alarm->is_playing()) {
			snd_alarm->play();
		} else if (!low && snd_alarm->is_playing()) {
			snd_alarm->stop();
		}
	}
}

void Dogfight::on_finish() {
	if (result.shots_fired > 0 && result.success) {
		float acc = (float)result.shots_hit / (float)result.shots_fired;
		result.score += (int)(acc * 1000.0f);
	}
	int lost = 0;
	for (const Aircraft &a : planes) {
		if (a.team == 0 && !a.player && !a.alive) {
			lost++;
		}
	}
	if (result.success && lost == 0) {
		result.score += 200;
	}
}

// ---------------------------------------------------------------------------
// HUD
// ---------------------------------------------------------------------------

void Dogfight::draw_hud(Canvas *c) {
	if (planes.empty() || !camera) {
		return;
	}
	const Aircraft &p = planes[0];
	Vector2 size = hud_size();
	Ref<Font> title = ui::font_title();
	const Color amber = ui::AMBER;
	const Color white(1, 1, 1, 0.9f);
	const Color red(1.0f, 0.25f, 0.18f, 0.95f);
	const Color blue(0.45f, 0.75f, 1.0f, 0.9f);

	bool flying = p.alive && !p.dying;
	Vector3 fwd = forward_of(p.basis);

	// Gunsight.
	if (flying) {
		Vector3 aim = p.pos + fwd * 320.0f;
		if (!camera->is_position_behind(aim)) {
			Vector2 s = camera->unproject_position(aim);
			Color col = p.firing ? Color(1.0f, 0.9f, 0.6f, 1.0f) : Color(1.0f, 0.85f, 0.4f, 0.85f);
			c->draw_arc(s, 34.0f, 0.0f, TAU_F, 48, col, 2.0f, true);
			c->draw_circle(s, 2.5f, col);
			for (int i = 0; i < 4; i++) {
				float a = i * PI_F * 0.5f;
				Vector2 d(std::cos(a), std::sin(a));
				c->draw_line(s + d * 34.0f, s + d * 50.0f, col, 2.0f, true);
			}
		}
	}

	// Contacts.
	int nearest = -1;
	float nearest_d = 1e9f;
	for (int i = 1; i < (int)planes.size(); i++) {
		const Aircraft &a = planes[i];
		if (!a.alive) {
			continue;
		}
		float d = a.pos.distance_to(p.pos);
		bool enemy = a.team == 1;
		Color col = enemy ? red : blue;
		if (a.dying) {
			col.a *= 0.4f;
		}
		bool behind = camera->is_position_behind(a.pos);
		Vector2 s = camera->unproject_position(a.pos);
		bool on_screen = !behind && s.x > 40 && s.x < size.x - 40 && s.y > 40 && s.y < size.y - 40;
		if (on_screen) {
			float r = clampf(2600.0f / MAX(d, 1.0f), 14.0f, 60.0f);
			if (enemy) {
				// Corner brackets.
				for (int k = 0; k < 4; k++) {
					float sx = (k % 2 == 0) ? -1.0f : 1.0f;
					float sy = (k / 2 == 0) ? -1.0f : 1.0f;
					Vector2 corner = s + Vector2(sx * r, sy * r);
					c->draw_line(corner, corner - Vector2(sx * r * 0.45f, 0), col, 2.5f, true);
					c->draw_line(corner, corner - Vector2(0, sy * r * 0.45f), col, 2.5f, true);
				}
				if (!a.dying) {
					String label = String::num_int64((int64_t)(d * 1.09361f)) + String(" yd");
					c->text_shadowed(title, s + Vector2(-60, r + 24.0f), label, 20, col, 1, 120);
					if (d < nearest_d && fwd.dot((a.pos - p.pos).normalized()) > 0.5f) {
						nearest_d = d;
						nearest = i;
					}
				}
			} else {
				PackedVector2Array tri;
				tri.push_back(s + Vector2(0, -r * 0.7f - 8));
				tri.push_back(s + Vector2(-7, -r * 0.7f - 20));
				tri.push_back(s + Vector2(7, -r * 0.7f - 20));
				c->draw_colored_polygon(tri, col);
				c->text_shadowed(title, s + Vector2(-60, -r * 0.7f - 26.0f), a.callsign, 20, col, 1, 120);
				if (a.role == 1) {
					draw_bar(c, s + Vector2(-20, r * 0.7f + 10.0f), Vector2(40, 6), a.hp / a.max_hp, col, "");
				}
			}
		} else if (enemy && !a.dying && flying) {
			// Edge arrow pointing toward the bandit.
			Vector3 local = camera->get_global_transform().basis.xform_inv(a.pos - camera->get_global_position());
			Vector2 dir(local.x, -local.y);
			if (dir.length_squared() < 1e-6f) {
				dir = Vector2(0, 1);
			}
			dir.normalize();
			Vector2 centre = size * 0.5f;
			float rx = size.x * 0.5f - 70.0f;
			float ry = size.y * 0.5f - 70.0f;
			float k = MIN(rx / MAX(std::fabs(dir.x), 1e-4f), ry / MAX(std::fabs(dir.y), 1e-4f));
			Vector2 at = centre + dir * k;
			Vector2 n(-dir.y, dir.x);
			PackedVector2Array tri;
			tri.push_back(at + dir * 22.0f);
			tri.push_back(at - dir * 8.0f + n * 14.0f);
			tri.push_back(at - dir * 8.0f - n * 14.0f);
			c->draw_colored_polygon(tri, col);
		}
	}

	// Lead marker for the closest bandit ahead.
	if (flying && nearest >= 0 && nearest_d < 900.0f) {
		const Aircraft &t = planes[nearest];
		Vector3 lead = t.pos + forward_of(t.basis) * t.speed * (nearest_d / BULLET_SPEED) -
				fwd * p.speed * (nearest_d / BULLET_SPEED) * 0.0f;
		if (!camera->is_position_behind(lead)) {
			Vector2 s = camera->unproject_position(lead);
			Color col = nearest_d < GUN_RANGE ? Color(1.0f, 0.95f, 0.5f, 1.0f) : Color(1.0f, 0.95f, 0.5f, 0.45f);
			PackedVector2Array d;
			d.push_back(s + Vector2(0, -11));
			d.push_back(s + Vector2(11, 0));
			d.push_back(s + Vector2(0, 11));
			d.push_back(s + Vector2(-11, 0));
			d.push_back(s + Vector2(0, -11));
			c->draw_polyline(d, col, 2.5f, true);
			if (nearest_d < GUN_RANGE) {
				c->text_shadowed(title, s + Vector2(-60, -22), "IN RANGE", 20, col, 1, 120);
			}
		}
	}

	// Instruments, bottom-left.
	float x = 60.0f;
	float y = size.y - 60.0f;
	c->text_shadowed(title, Vector2(x, y - 110), "SPEED", 22, Color(1, 1, 1, 0.6f));
	c->text_shadowed(title, Vector2(x, y - 60), String::num_int64((int64_t)(p.speed * MS_TO_MPH)) + String(" MPH"), 50,
			p.speed < 58.0f ? red : white);
	c->text_shadowed(title, Vector2(x + 260, y - 110), "ALTITUDE", 22, Color(1, 1, 1, 0.6f));
	c->text_shadowed(title, Vector2(x + 260, y - 60),
			String::num_int64((int64_t)(p.pos.y * M_TO_FT / 10.0f) * 10) + String(" FT"), 50,
			p.pos.y < 200.0f ? red : white);
	draw_bar(c, Vector2(x, y - 26), Vector2(440, 16), p.throttle, amber, "");
	c->text_shadowed(title, Vector2(x + 450, y - 10), "THROTTLE", 20, Color(1, 1, 1, 0.6f));

	// Bottom-right: ammunition and airframe.
	float rx = size.x - 500.0f;
	c->text_shadowed(title, Vector2(rx, y - 110), "AMMUNITION", 22, Color(1, 1, 1, 0.6f));
	c->text_shadowed(title, Vector2(rx, y - 60), String::num_int64(ammo), 50, ammo < 200 ? red : white);
	c->text_shadowed(title, Vector2(rx + 220, y - 110), "AIRFRAME", 22, Color(1, 1, 1, 0.6f));
	float hp = saturate(p.hp / p.max_hp);
	draw_bar(c, Vector2(rx + 220, y - 92), Vector2(220, 30), hp, hp > 0.5f ? Color(0.5f, 0.85f, 0.45f) : (hp > 0.25f ? amber : red), "");

	// Top-right: bandit count and victories.
	c->text_shadowed(title, Vector2(size.x - 760, 70), objective_text(), 40, red, 2, 700);
	for (int i = 0; i < result.air_kills; i++) {
		Vector2 at(size.x - 80.0f - i * 40.0f, 110.0f);
		c->draw_rect(Rect2(at + Vector2(-14, -4), Vector2(28, 8)), white, true);
		c->draw_rect(Rect2(at + Vector2(-4, -14), Vector2(8, 28)), white, true);
		c->draw_rect(Rect2(at + Vector2(-11, -2), Vector2(22, 4)), Color(0, 0, 0), true);
		c->draw_rect(Rect2(at + Vector2(-2, -11), Vector2(4, 22)), Color(0, 0, 0), true);
	}

	// Radar, bottom centre.
	Vector2 rc(size.x * 0.5f, size.y - 130.0f);
	const float rr = 95.0f;
	const float range = 3000.0f;
	c->draw_circle(rc, rr, Color(0, 0, 0, 0.35f));
	c->draw_arc(rc, rr, 0.0f, TAU_F, 64, Color(1, 1, 1, 0.45f), 1.5f, true);
	c->draw_arc(rc, rr * 0.5f, 0.0f, TAU_F, 48, Color(1, 1, 1, 0.2f), 1.0f, true);
	c->draw_line(rc + Vector2(0, -rr), rc + Vector2(0, rr), Color(1, 1, 1, 0.15f), 1.0f);
	c->draw_line(rc + Vector2(-rr, 0), rc + Vector2(rr, 0), Color(1, 1, 1, 0.15f), 1.0f);
	Vector3 flat(fwd.x, 0.0f, fwd.z);
	if (flat.length_squared() < 1e-4f) {
		flat = Vector3(0, 0, -1);
	}
	flat.normalize();
	Vector3 flat_right(-flat.z, 0.0f, flat.x);
	for (int i = 1; i < (int)planes.size(); i++) {
		const Aircraft &a = planes[i];
		if (!a.alive || a.dying) {
			continue;
		}
		Vector3 rel = a.pos - p.pos;
		Vector2 r2(rel.dot(flat_right), -rel.dot(flat));
		float len = r2.length();
		if (len > range) {
			r2 = r2 / len * range;
		}
		Vector2 at = rc + r2 / range * (rr - 6.0f);
		Color col = a.team == 1 ? red : blue;
		c->draw_circle(at, 5.0f, col);
		// Altitude tick: above or below.
		float dy = a.pos.y - p.pos.y;
		if (std::fabs(dy) > 120.0f) {
			float s = dy > 0 ? -1.0f : 1.0f;
			c->draw_line(at + Vector2(0, s * 6.0f), at + Vector2(0, s * 13.0f), col, 2.0f);
		}
	}
	PackedVector2Array me;
	me.push_back(rc + Vector2(0, -9));
	me.push_back(rc + Vector2(7, 8));
	me.push_back(rc + Vector2(-7, 8));
	c->draw_colored_polygon(me, white);

	// Warnings.
	if (flying) {
		float blink = std::fmod(clock, 0.5f) < 0.3f ? 1.0f : 0.3f;
		if (p.pos.y < 180.0f && fwd.y < -0.05f) {
			c->text_shadowed(title, Vector2(0, size.y * 0.5f + 150.0f), "PULL UP", 60, Color(1, 0.2f, 0.15f, blink), 1, size.x);
		} else if (p.speed < 58.0f) {
			c->text_shadowed(title, Vector2(0, size.y * 0.5f + 150.0f), "STALL", 60, Color(1, 0.2f, 0.15f, blink), 1, size.x);
		} else if (ammo <= 0) {
			c->text_shadowed(title, Vector2(0, size.y * 0.5f + 150.0f), "GUNS EMPTY", 44, Color(1, 0.8f, 0.3f, blink), 1, size.x);
		}
	}
}

} // namespace ww2
