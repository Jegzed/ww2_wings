#include "missions/strafing.h"

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
const float LATERAL_LIMIT = 52.0f;
const float ALT_MIN = 7.0f;
const float ALT_MAX = 42.0f;
const float PLANE_SCALE = 1.6f;
const float GUN_ANGLE = 14.0f; // degrees below the horizon
const float BULLET_SPEED = 430.0f;
const float FLAK_RANGE = 330.0f;
const float FLAK_SPEED = 260.0f;
const float PLANE_Z = 0.0f; // plane position relative to the anchor
} // namespace

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------

void Strafing::add_soldiers(Rng &rng, const Vector3 &at, int count) {
	for (int i = 0; i < count; i++) {
		Vector3 p = at + Vector3(rng.range(-2.0f, 2.0f), 0, i * 2.4f + rng.range(-0.6f, 0.6f));
		int idx = ground.add("soldier", p, 180.0f + rng.range(-10.0f, 10.0f), false);
		ground.targets[idx].speed = 1.4f;
	}
}

void Strafing::scatter_scenery(Rng &rng, float length, float clear_half_width) {
	std::vector<Transform3D> trees, bushes, pines;
	int count = (int)(length * 0.3f);
	for (int i = 0; i < count; i++) {
		Vector3 p(rng.range(-420.0f, 420.0f), 0.0f, rng.range(-length - 400.0f, 300.0f));
		// Only scenery outside the playable corridor: inside it, trees are real obstacles.
		if (std::fabs(p.x) < LATERAL_LIMIT + 12.0f || std::fabs(p.x) < clear_half_width) {
			continue;
		}
		if (def->variant == ST_BEACH && p.x < 60.0f) {
			continue;
		}
		bool blocked = false;
		for (const GroundTarget &t : ground.targets) {
			float reach = MAX(t.hx, t.hz) + 6.0f;
			if (std::fabs(t.pos.x - p.x) < reach && std::fabs(t.pos.z - p.z) < reach) {
				blocked = true;
				break;
			}
		}
		if (blocked) {
			continue;
		}
		int clump = rng.irange(1, 6);
		for (int k = 0; k < clump; k++) {
			Vector3 q = p + Vector3(rng.range(-12.0f, 12.0f), 0, rng.range(-12.0f, 12.0f)) * (k > 0 ? 1.0f : 0.0f);
			if (std::fabs(q.x) < LATERAL_LIMIT + 10.0f || std::fabs(q.x) < clear_half_width) {
				continue;
			}
			float s = rng.range(0.8f, 1.4f);
			Basis b = Basis(Vector3(0, 1, 0), rng.range(0, TAU_F)).scaled(Vector3(s, s * rng.range(0.9f, 1.2f), s));
			float pick = rng.f();
			if (pick < 0.6f) {
				trees.push_back(Transform3D(b, q));
			} else if (pick < 0.8f) {
				pines.push_back(Transform3D(b, q));
			} else {
				bushes.push_back(Transform3D(b, q));
			}
		}
	}
	scatter(this, "tree", trees);
	scatter(this, "pine", pines);
	scatter(this, "bush", bushes);
}

void Strafing::layout_convoy(Rng &rng) {
	run_length = 3700.0f;

	const int groups = 6;
	for (int g = 0; g < groups; g++) {
		float z = -380.0f - g * 560.0f + rng.range(-60.0f, 60.0f);
		int n = rng.irange(4, 7);
		for (int i = 0; i < n; i++) {
			const char *kind = "truck";
			float pick = rng.f();
			if (pick < 0.12f + 0.03f * def->difficulty) {
				kind = "tank";
			} else if (pick < 0.32f) {
				kind = "halftrack";
			} else if (pick < 0.42f) {
				kind = "staff_car";
			}
			int idx = ground.add(kind, Vector3(rng.range(-1.0f, 1.0f), 0, z - i * rng.range(15.0f, 19.0f)), 0.0f, true);
			ground.targets[idx].speed = 6.0f;
		}
		if (rng.chance(0.75f)) {
			float side = rng.chance(0.5f) ? 1.0f : -1.0f;
			add_soldiers(rng, Vector3(side * 5.5f, 0, z + 30.0f), rng.irange(6, 11));
		}
	}

	// Poplars lining stretches of the road.
	for (int s = 0; s < 5; s++) {
		float z0 = -200.0f - s * 720.0f - rng.range(0.0f, 150.0f);
		int n = rng.irange(7, 13);
		for (int i = 0; i < n; i++) {
			for (int side = -1; side <= 1; side += 2) {
				if (rng.chance(0.85f)) {
					ground.add("poplar", Vector3(side * 15.0f + rng.range(-0.8f, 0.8f), 0, z0 - i * 24.0f), rng.range(0, 360.0f),
							false);
				}
			}
		}
	}
	// Farms and a village crowding the road.
	for (int i = 0; i < 5; i++) {
		float side = (i % 2 == 0) ? 1.0f : -1.0f;
		float z = -650.0f - i * 640.0f + rng.range(-80.0f, 80.0f);
		ground.add("house", Vector3(side * rng.range(30.0f, 46.0f), 0, z), rng.range(-20.0f, 20.0f), false);
		ground.add("barn", Vector3(side * rng.range(34.0f, 48.0f), 0, z - 30.0f), 90.0f + rng.range(-20.0f, 20.0f), false);
		ground.add("tree", Vector3(side * rng.range(24.0f, 40.0f), 0, z + 22.0f), 0.0f, false);
	}
	ground.add("church", Vector3(-34.0f, 0, -2050.0f), 0.0f, false);

	int flak = 1 + def->difficulty;
	for (int i = 0; i < flak; i++) {
		float side = (i % 2 == 0) ? 1.0f : -1.0f;
		float z = -700.0f - i * (run_length - 900.0f) / flak + rng.range(-60.0f, 60.0f);
		ground.add("flak", Vector3(side * rng.range(24.0f, 34.0f), 0, z), rng.range(0, 360.0f), true);
	}

	Ref<ShaderMaterial> mat = terrain->get_mesh()->surface_get_material(0);
	mat->set_shader_parameter("strip_a", Vector4(0.0f, 0.0f, 0.0f, -1.0f));
	mat->set_shader_parameter("strip_a_width", 4.2f);
	mat->set_shader_parameter("strip_a_kind", 1);
	mat->set_shader_parameter("strip_b", Vector4(0.0f, -2300.0f, 1.0f, 0.25f));
	mat->set_shader_parameter("strip_b_width", 3.0f);
	mat->set_shader_parameter("strip_b_kind", 1);
	mat->set_shader_parameter("river_amount", 0.0f);
}

void Strafing::layout_airfield(Rng &rng) {
	run_length = 3000.0f;
	const float field0 = 650.0f;
	const float field1 = 2550.0f;

	// Dispersal: fighters parked either side of the runway.
	int parked = 0;
	for (float z = -field0 - 120.0f; z > -field1 + 150.0f; z -= rng.range(55.0f, 90.0f)) {
		for (int side = -1; side <= 1; side += 2) {
			if (!rng.chance(0.7f)) {
				continue;
			}
			float x = side * rng.range(30.0f, 44.0f);
			float yaw = side > 0 ? 90.0f : -90.0f;
			ground.add(rng.chance(0.35f) ? "plane_fw190" : "plane_bf109", Vector3(x, 0, z), yaw + rng.range(-25.0f, 25.0f),
					true);
			parked++;
			if (rng.chance(0.3f)) {
				ground.add("truck", Vector3(x + side * 9.0f, 0, z + 8.0f), rng.range(0, 360.0f), true);
			}
		}
	}
	// A pair caught taxiing out.
	for (int i = 0; i < 2; i++) {
		int idx = ground.add("plane_bf109", Vector3(i == 0 ? -6.0f : 7.0f, 0, -field0 - 600.0f - i * 40.0f), 0.0f, true);
		ground.targets[idx].speed = 9.0f;
	}

	for (int i = 0; i < 3; i++) {
		float side = (i % 2 == 0) ? 1.0f : -1.0f;
		ground.add("hangar", Vector3(side * 92.0f, 0, -field0 - 350.0f - i * 600.0f), side > 0 ? -90.0f : 90.0f, true);
	}
	for (int i = 0; i < 3; i++) {
		ground.add("fuel_tank", Vector3(-58.0f - (i % 2) * 14.0f, 0, -field0 - 980.0f - i * 15.0f), 0.0f, true);
	}
	ground.add("tower", Vector3(60.0f, 0, -field0 - 900.0f), 0.0f, true);
	for (int i = 0; i < 4; i++) {
		ground.add("tent", Vector3(64.0f + (i % 2) * 8.0f, 0, -field0 - 1500.0f - i * 9.0f), 90.0f, true);
	}
	ground.add("crate_stack", Vector3(-52.0f, 0, -field0 - 1600.0f), 30.0f, true);
	ground.add("crate_stack", Vector3(-50.0f, 0, -field0 - 1612.0f), 80.0f, true);
	add_soldiers(rng, Vector3(22.0f, 0, -field0 - 300.0f), 8);
	add_soldiers(rng, Vector3(-24.0f, 0, -field0 - 1300.0f), 8);

	int flak = 2 + def->difficulty;
	for (int i = 0; i < flak; i++) {
		float side = (i % 2 == 0) ? 1.0f : -1.0f;
		float z = -field0 + 60.0f - i * (field1 - field0) / flak;
		ground.add("flak", Vector3(side * rng.range(54.0f, 70.0f), 0, z), rng.range(0, 360.0f), true);
	}
	// Approach: woods and a farm before the wire.
	for (int i = 0; i < 9; i++) {
		ground.add(rng.chance(0.5f) ? "tree" : "poplar", Vector3(rng.range(-50.0f, 50.0f), 0, -120.0f - i * 55.0f),
				rng.range(0, 360.0f), false);
	}
	ground.add("house", Vector3(38.0f, 0, -420.0f), 10.0f, false);

	Ref<ShaderMaterial> mat = terrain->get_mesh()->surface_get_material(0);
	mat->set_shader_parameter("strip_a", Vector4(0.0f, 0.0f, 0.0f, -1.0f));
	mat->set_shader_parameter("strip_a_width", 20.0f);
	mat->set_shader_parameter("strip_a_kind", 3);
	mat->set_shader_parameter("strip_a_range", Vector2(field0, field1));
	mat->set_shader_parameter("strip_b", Vector4(37.0f, 0.0f, 0.0f, -1.0f));
	mat->set_shader_parameter("strip_b_width", 5.0f);
	mat->set_shader_parameter("strip_b_kind", 3);
	mat->set_shader_parameter("strip_b_range", Vector2(field0 + 60.0f, field1 - 60.0f));
	mat->set_shader_parameter("strip_c", Vector4(-37.0f, 0.0f, 0.0f, -1.0f));
	mat->set_shader_parameter("strip_c_width", 5.0f);
	mat->set_shader_parameter("strip_c_kind", 3);
	mat->set_shader_parameter("strip_c_range", Vector2(field0 + 60.0f, field1 - 60.0f));
	mat->set_shader_parameter("field_rect", Vector4(0.0f, -(field0 + field1) * 0.5f, 210.0f, (field1 - field0) * 0.5f + 60.0f));
	mat->set_shader_parameter("river_amount", 0.0f);
	(void)parked;
}


void Strafing::layout_train(Rng &rng) {
	run_length = 3400.0f;
	// The train runs the same way you fly, so you overtake it slowly.
	const float train_speed = 13.0f;
	float z = -620.0f;
	loco = ground.add("loco", Vector3(0, 0, z), 0.0f, true);
	ground.targets[loco].speed = train_speed;
	const char *cars[] = { "boxcar", "flatcar", "flak_wagon", "tanker", "boxcar", "boxcar", "tanker", "flatcar", "boxcar" };
	for (int i = 0; i < 9; i++) {
		int idx = ground.add(cars[i], Vector3(0, 0, z + 12.4f + i * 10.6f), 0.0f, true);
		ground.targets[idx].speed = train_speed;
	}
	// A halt with a goods shed and a road crossing.
	ground.add("warehouse", Vector3(26.0f, 0, -1900.0f), 0.0f, true);
	ground.add("house", Vector3(-30.0f, 0, -1860.0f), 90.0f, false);
	ground.add("truck", Vector3(22.0f, 0, -1950.0f), 0.0f, true);
	ground.add("truck", Vector3(22.0f, 0, -1962.0f), 0.0f, true);
	ground.add("flak", Vector3(-28.0f, 0, -1920.0f), rng.range(0, 360.0f), true);
	for (int s = 0; s < 4; s++) {
		float z0 = -300.0f - s * 800.0f;
		for (int i = 0; i < 10; i++) {
			if (rng.chance(0.8f)) {
				ground.add("poplar", Vector3(16.0f + rng.range(-0.5f, 0.5f), 0, z0 - i * 26.0f), rng.range(0, 360.0f), false);
			}
			if (rng.chance(0.5f)) {
				ground.add("tree", Vector3(-19.0f + rng.range(-3.0f, 3.0f), 0, z0 - i * 26.0f - 10.0f), rng.range(0, 360.0f), false);
			}
		}
	}
	for (int i = 0; i < 3; i++) {
		float side = (i % 2 == 0) ? 1.0f : -1.0f;
		ground.add("house", Vector3(side * rng.range(34.0f, 48.0f), 0, -900.0f - i * 700.0f), rng.range(-20.0f, 20.0f), false);
	}
	int flak = def->difficulty / 2;
	for (int i = 0; i < flak; i++) {
		ground.add("flak", Vector3((i % 2 == 0 ? 1.0f : -1.0f) * rng.range(26.0f, 36.0f), 0, -1200.0f - i * 900.0f), rng.range(0, 360.0f), true);
	}

	Ref<ShaderMaterial> mat = terrain->get_mesh()->surface_get_material(0);
	mat->set_shader_parameter("strip_a", Vector4(0.0f, 0.0f, 0.0f, -1.0f));
	mat->set_shader_parameter("strip_a_width", 2.6f);
	mat->set_shader_parameter("strip_a_kind", 2);
	mat->set_shader_parameter("strip_b", Vector4(0.0f, -1930.0f, 1.0f, 0.0f));
	mat->set_shader_parameter("strip_b_width", 3.5f);
	mat->set_shader_parameter("strip_b_kind", 1);
	mat->set_shader_parameter("river_amount", 0.0f);
}

void Strafing::layout_beach(Rng &rng) {
	run_length = 3000.0f;
	// Sea to the west of the flight line, bunkers in the dunes to the east.
	for (int i = 0; i < 6; i++) {
		float z = -400.0f - i * 440.0f + rng.range(-40.0f, 40.0f);
		ground.add("bunker", Vector3(rng.range(30.0f, 40.0f), 0, z), -90.0f + rng.range(-15.0f, 15.0f), false);
		ground.add("coastal_gun", Vector3(rng.range(44.0f, 52.0f), 0, z + rng.range(40.0f, 70.0f)), rng.range(0, 360.0f), true);
		ground.add("mg_nest", Vector3(rng.range(8.0f, 20.0f), 0, z + rng.range(-60.0f, 60.0f)), 0.0f, true);
		if (rng.chance(0.6f)) {
			ground.add("mg_nest", Vector3(rng.range(14.0f, 26.0f), 0, z - rng.range(80.0f, 140.0f)), 0.0f, true);
		}
		ground.add("truck", Vector3(rng.range(52.0f, 62.0f), 0, z + rng.range(100.0f, 140.0f)), rng.range(0, 360.0f), true);
		if (rng.chance(0.5f)) {
			ground.add("tent", Vector3(rng.range(56.0f, 66.0f), 0, z + rng.range(150.0f, 190.0f)), rng.range(0, 360.0f), true);
		}
		add_soldiers(rng, Vector3(rng.range(4.0f, 12.0f), 0, z + 20.0f), rng.irange(4, 8));
	}
	// Wrecked landing craft in the surf, and the rest of the fleet beyond.
	for (int i = 0; i < 10; i++) {
		ground.add("landing_craft", Vector3(rng.range(-70.0f, -40.0f), -0.6f, -200.0f - i * 300.0f + rng.range(-80.0f, 80.0f)),
				rng.range(-60.0f, 60.0f), false);
	}
	int flak = 1 + def->difficulty / 2;
	for (int i = 0; i < flak; i++) {
		ground.add("flak", Vector3(rng.range(48.0f, 60.0f), 0, -700.0f - i * 1000.0f), rng.range(0, 360.0f), true);
	}

	Ref<ShaderMaterial> mat = terrain->get_mesh()->surface_get_material(0);
	mat->set_shader_parameter("coast_x", -24.0f);
	mat->set_shader_parameter("river_amount", 0.0f);
	mat->set_shader_parameter("strip_a", Vector4(70.0f, 0.0f, 0.0f, -1.0f));
	mat->set_shader_parameter("strip_a_width", 3.2f);
	mat->set_shader_parameter("strip_a_kind", 1);
}

void Strafing::layout_barges(Rng &rng) {
	run_length = 3400.0f;
	// Traffic on the river coming up toward you, and ferries tied up under the banks.
	for (int i = 0; i < 9; i++) {
		float z = -500.0f - i * 330.0f + rng.range(-60.0f, 60.0f);
		int idx = ground.add("barge", Vector3(rng.range(-9.0f, 9.0f), 0, z), 180.0f + rng.range(-6.0f, 6.0f), true);
		ground.targets[idx].speed = -rng.range(2.5f, 4.5f);
		if (rng.chance(0.5f)) {
			ground.add("barge", Vector3(rng.chance(0.5f) ? -15.0f : 15.0f, 0, z - 120.0f), rng.range(-8.0f, 8.0f), true);
		}
	}
	// Crossing points: trucks queued on the banks.
	for (int i = 0; i < 4; i++) {
		float side = (i % 2 == 0) ? 1.0f : -1.0f;
		float z = -800.0f - i * 700.0f;
		for (int k = 0; k < 3; k++) {
			ground.add("truck", Vector3(side * (28.0f + k * 7.0f), 0, z + k * 3.0f), side > 0 ? -90.0f : 90.0f, true);
		}
		ground.add("mg_nest", Vector3(side * 26.0f, 0, z - 40.0f), 0.0f, true);
	}
	// Trees down to the water on both banks.
	for (int s = 0; s < 6; s++) {
		float z0 = -200.0f - s * 560.0f - rng.range(0.0f, 120.0f);
		for (int i = 0; i < 8; i++) {
			for (int side = -1; side <= 1; side += 2) {
				if (rng.chance(0.65f)) {
					ground.add(rng.chance(0.5f) ? "tree" : "poplar", Vector3(side * rng.range(24.0f, 34.0f), 0, z0 - i * 28.0f),
							rng.range(0, 360.0f), false);
				}
			}
		}
	}
	int flak = 1 + def->difficulty / 2;
	for (int i = 0; i < flak; i++) {
		float side = (i % 2 == 0) ? 1.0f : -1.0f;
		ground.add("flak", Vector3(side * rng.range(36.0f, 46.0f), 0, -1100.0f - i * 900.0f), rng.range(0, 360.0f), true);
	}

	Ref<ShaderMaterial> mat = terrain->get_mesh()->surface_get_material(0);
	mat->set_shader_parameter("strip_a", Vector4(0.0f, 0.0f, 0.0f, -1.0f));
	mat->set_shader_parameter("strip_a_width", 21.0f);
	mat->set_shader_parameter("strip_a_kind", 5);
	mat->set_shader_parameter("strip_b", Vector4(40.0f, 0.0f, 0.0f, -1.0f));
	mat->set_shader_parameter("strip_b_width", 3.0f);
	mat->set_shader_parameter("strip_b_kind", 1);
	mat->set_shader_parameter("river_amount", 0.0f);
}

void Strafing::layout_village(Rng &rng) {
	run_length = 3200.0f;
	const float hq_z = -1900.0f;
	primary = ground.add("chateau", Vector3(44.0f, 0, hq_z), -90.0f, true);
	ground.targets[primary].primary = true;
	// Staff cars and radio trucks in the yard, sentries in the gardens.
	for (int i = 0; i < 3; i++) {
		ground.add("staff_car", Vector3(24.0f + i * 5.0f, 0, hq_z + 18.0f + (i % 2) * 6.0f), rng.range(-20.0f, 20.0f), true);
	}
	for (int i = 0; i < 3; i++) {
		ground.add("truck", Vector3(26.0f + i * 6.0f, 0, hq_z - 22.0f), 90.0f, true);
	}
	ground.add("halftrack", Vector3(20.0f, 0, hq_z + 40.0f), 0.0f, true);
	ground.add("mg_nest", Vector3(18.0f, 0, hq_z - 40.0f), 0.0f, true);
	ground.add("mg_nest", Vector3(30.0f, 0, hq_z + 60.0f), 0.0f, true);
	add_soldiers(rng, Vector3(14.0f, 0, hq_z + 10.0f), 6);
	// The village: houses crowding the road, the church at the far end.
	for (int i = 0; i < 12; i++) {
		float side = (i % 2 == 0) ? 1.0f : -1.0f;
		float z = -1500.0f - i * 55.0f;
		if (side > 0 && z < hq_z + 80.0f && z > hq_z - 80.0f) {
			continue; // the chateau grounds
		}
		ground.add(rng.chance(0.75f) ? "house" : "barn", Vector3(side * rng.range(22.0f, 34.0f), 0, z), rng.range(-15.0f, 15.0f), false);
	}
	ground.add("church", Vector3(-38.0f, 0, -2250.0f), 0.0f, false);
	for (int i = 0; i < 3; i++) {
		ground.add("tree", Vector3(rng.range(-46.0f, -36.0f), 0, -1400.0f - i * 300.0f), rng.range(0, 360.0f), false);
	}
	// Traffic on the road in and out.
	for (int i = 0; i < 4; i++) {
		int idx = ground.add(rng.chance(0.3f) ? "staff_car" : "truck", Vector3(rng.range(-1.0f, 1.0f), 0, -600.0f - i * 90.0f), 0.0f, true);
		ground.targets[idx].speed = 6.0f;
	}
	for (int i = 0; i < 3; i++) {
		ground.add("truck", Vector3(rng.range(-1.0f, 1.0f), 0, -2600.0f - i * 40.0f), 0.0f, true);
	}
	add_soldiers(rng, Vector3(6.0f, 0, -1150.0f), 8);
	int flak = 1 + def->difficulty / 2;
	for (int i = 0; i < flak; i++) {
		ground.add("flak", Vector3((i % 2 == 0 ? -1.0f : 1.0f) * rng.range(26.0f, 40.0f), 0, -1300.0f - i * 1000.0f), rng.range(0, 360.0f), true);
	}

	Ref<ShaderMaterial> mat = terrain->get_mesh()->surface_get_material(0);
	mat->set_shader_parameter("strip_a", Vector4(0.0f, 0.0f, 0.0f, -1.0f));
	mat->set_shader_parameter("strip_a_width", 4.2f);
	mat->set_shader_parameter("strip_a_kind", 1);
	mat->set_shader_parameter("strip_b", Vector4(0.0f, hq_z + 90.0f, 1.0f, 0.0f));
	mat->set_shader_parameter("strip_b_width", 3.0f);
	mat->set_shader_parameter("strip_b_kind", 1);
	mat->set_shader_parameter("river_amount", 0.0f);
}

void Strafing::build() {
	GameState &gs = GameState::get();

	WorldOptions opt;
	opt.time = def->time;
	opt.shadow_distance = 620.0f;
	opt.fog_density = 0.00014f;
	opt.sun_azimuth_deg = def->time == TOD_DUSK ? 250.0f : 110.0f;
	opt.sun_elevation_deg = def->time == TOD_DUSK ? 34.0f : 50.0f;
	opt.sun_scatter = 0.0f;
	world = build_world(this, opt);

	Ref<ShaderMaterial> terrain_mat;
	terrain = build_terrain(this, terrain_mat, 12000.0f);
	terrain_mat->set_shader_parameter("field_scale", 95.0f);
	terrain_mat->set_shader_parameter("wood_amount", 0.0f);
	opt.cloud_cover = def->cloud_cover;

	ground.setup(this);
	ground.fx_scale = 1.0f;
	ground.on_destroyed = [this](int index, bool by_player) {
		const GroundTarget &t = ground.targets[index];
		result.score += t.score * 2 / 5;
		if (t.counts) {
			result.ground_kills++;
		}
		if (t.kind != "soldier") {
			shake(0.3f);
		}
		if (t.kind == "tank") {
			message("Tank knocked out!", 2.5f, ui::AMBER);
		} else if (t.kind == "fuel_tank") {
			message("Fuel dump burning!", 2.5f, ui::AMBER);
		} else if (t.kind == "plane_bf109" || t.kind == "plane_fw190") {
			message("Fighter destroyed on the ground", 2.0f, ui::AMBER);
		} else if (t.kind == "flak") {
			message("Flak position silenced", 2.0f);
		} else if (t.kind == "hangar") {
			message("Hangar destroyed!", 2.5f, ui::AMBER);
		}
	};

	Rng rng(211 + def->variant * 23);
	float clear = 0.0f;
	switch (def->variant) {
		case ST_AIRFIELD:
			layout_airfield(rng);
			clear = 200.0f;
			break;
		case ST_TRAIN:
			layout_train(rng);
			break;
		case ST_BEACH:
			layout_beach(rng);
			clear = 90.0f;
			break;
		case ST_BARGES:
			layout_barges(rng);
			break;
		case ST_VILLAGE:
			layout_village(rng);
			break;
		case ST_CONVOY:
		default:
			layout_convoy(rng);
			break;
	}
	scatter_scenery(rng, run_length, clear);

	bullets.setup(this, 400, 7.0f, 0.14f);

	plane_paint = mats::plane_paint(mats::PAINT_P47_SILVER);
	plane_paint->set_shader_parameter("metal", 0.3f);
	plane_paint->set_shader_parameter("rough", 0.55f);
	plane_paint->set_shader_parameter("top_color", Color(0.62f, 0.64f, 0.66f));
	plane = models::make_plane(models::PLANE_P47, plane_paint, true);
	add_child(plane);

	{
		Ref<PlaneMesh> pm;
		pm.instantiate();
		pm->set_size(Vector2(1, 1));
		Ref<StandardMaterial3D> m;
		m.instantiate();
		m->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
		m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
		m->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, mats::ring_sprite());
		m->set_albedo(Color(1.0f, 0.85f, 0.3f, 0.9f));
		m->set_flag(BaseMaterial3D::FLAG_DISABLE_FOG, true);
		m->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
		m->set_render_priority(10);
		pm->set_material(m);
		marker = memnew(MeshInstance3D);
		marker->set_mesh(pm);
		marker->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
		add_child(marker);
	}

	camera = memnew(Camera3D);
	camera->set_near(5.0f);
	camera->set_far(5000.0f);
	camera->set_fov(30.0f);
	add_child(camera);
	camera->make_current();

	ammo = 1700 + gs.pilot.skills[SKILL_MECHANICAL] * 60;
	anchor_z = 120.0f;
	altitude = 28.0f;

	snd_engine = audio::make_loop(this, "engine", -10.0f);
	snd_engine->play();
	snd_wind = audio::make_loop(this, "wind", -18.0f);
	snd_wind->play();
	snd_gun = audio::make_loop(this, "gun", -5.0f);
}

// ---------------------------------------------------------------------------
// Rules
// ---------------------------------------------------------------------------

int Strafing::required() const {
	float share = (def->variant == ST_VILLAGE || def->variant == ST_TRAIN) ? 0.4f : 0.5f;
	return MAX(1, (int)std::ceil(ground.total() * share));
}

bool Strafing::objective_met() const {
	if (primary >= 0 && ground.targets[primary].alive) {
		return false;
	}
	if (loco >= 0 && ground.targets[loco].alive) {
		return false;
	}
	return ground.destroyed() >= required();
}

Vector3 Strafing::player_velocity() const {
	return Vector3(vx, vy, -scroll_speed);
}

Vector3 Strafing::aim_point() const {
	float a = deg2rad(GUN_ANGLE) - pitch * 0.5f;
	a = clampf(a, deg2rad(3.0f), deg2rad(25.0f));
	float reach = altitude / std::tan(a);
	Vector3 p = plane->get_position();
	return Vector3(p.x, 0.0f, p.z - reach);
}

void Strafing::fire_guns(float dt, bool firing) {
	GameState &gs = GameState::get();
	fire_timer -= dt;
	firing_now = firing && ammo > 0 && !dead && !turning;
	if (!firing_now) {
		fire_timer = MAX(fire_timer, 0.0f);
		return;
	}
	const models::PlaneInfo &info = models::plane_info(models::PLANE_P47);
	Rng &rng = grng();
	Vector3 aim = aim_point();
	Transform3D xf = plane->get_transform();
	while (fire_timer <= 0.0f && ammo > 0) {
		fire_timer += 1.0f / 26.0f;
		ammo--;
		result.shots_fired++;
		Vector3 muzzle = xf.xform((gun_side++ % 2 == 0) ? info.gun_left : info.gun_right);
		Vector3 dir = (aim - muzzle).normalized();
		// Dispersion grows with range, so low passes are much more accurate.
		float spread = 0.016f * gs.accuracy();
		dir = (dir + rng.in_sphere() * spread).normalized();
		float dmg = 8.0f * (0.8f + 0.04f * gs.pilot.skills[SKILL_SHOOTING]);
		bullets.fire(muzzle + dir * 3.0f, dir * BULLET_SPEED, 2.0f, dmg, 0, 0);
	}
	shake(0.1f);
}

void Strafing::update_bullets(float dt) {
	bullets.step(dt, 0.0f);
	puff_budget = MIN(puff_budget + dt * 40.0f, 6.0f);
	Vector3 ppos = plane->get_position();
	for (fx::Bullet &b : bullets.list()) {
		if (!b.alive) {
			continue;
		}
		if (b.team == 1) {
			// Flak rounds against the player.
			if (dead || turning) {
				continue;
			}
			Vector3 seg = b.pos - b.prev;
			float len = seg.length();
			if (len < 1e-4f) {
				continue;
			}
			Vector3 dir = seg / len;
			float t = clampf((ppos - b.prev).dot(dir), 0.0f, len);
			Vector3 closest = b.prev + dir * t;
			if (closest.distance_squared_to(ppos) < 5.5f * 5.5f) {
				b.alive = false;
				fx::spark(this, closest, 1.5f);
				hit_player(b.damage, String("Shot down by ground fire near ") +
											 String(def->location).get_slice(",", 0) + String("."));
			}
			continue;
		}
		// Player rounds: march along the segment so thin targets are not skipped.
		Vector3 seg = b.pos - b.prev;
		float len = seg.length();
		int steps = MAX(1, (int)std::ceil(len / 2.0f));
		for (int s = 1; s <= steps; s++) {
			Vector3 p = b.prev + seg * ((float)s / steps);
			if (p.y < 12.0f) {
				int hit = ground.hit_test(p, 0.5f);
				if (hit >= 0) {
					b.alive = false;
					GroundTarget &t = ground.targets[hit];
					bool hard = t.bullet_armor < 0.5f;
					if (puff_budget >= 1.0f) {
						puff_budget -= 1.0f;
						fx::spark(this, p, hard ? 1.0f : 1.6f);
					}
					if (t.max_hp < 1e8f) {
						result.shots_hit++;
						if (grng().chance(0.25f)) {
							audio::play(hard ? "ricochet" : "hit", -12.0f, grng().range(0.9f, 1.3f));
						}
						ground.damage(hit, b.damage, false);
					}
					break;
				}
			}
			if (p.y <= 0.0f) {
				b.alive = false;
				if (puff_budget >= 1.0f) {
					puff_budget -= 1.0f;
					fx::bullet_puff(this, Vector3(p.x, 0.2f, p.z), 1.3f, Color(0.45f, 0.38f, 0.28f));
				}
				break;
			}
		}
	}
	bullets.render();
}

void Strafing::update_soldiers(float dt) {
	Vector3 ppos = plane->get_position();
	for (GroundTarget &t : ground.targets) {
		if (!t.alive || t.kind != "soldier" || t.fleeing) {
			continue;
		}
		float ahead = ppos.z - t.pos.z;
		if (ahead > -10.0f && ahead < 170.0f && std::fabs(t.pos.x - ppos.x) < 60.0f) {
			t.fleeing = true;
			t.speed = 0.0f;
			float side = t.pos.x >= 0.0f ? 1.0f : -1.0f;
			t.flee = Vector3(side * grng().range(3.5f, 5.5f), 0, grng().range(-1.5f, 1.5f));
			t.yaw = std::atan2(-t.flee.x, -t.flee.z);
			t.node->set_rotation(Vector3(0, t.yaw, 0));
		}
	}
}

void Strafing::hit_player(float amount, const String &cause) {
	if (dead) {
		return;
	}
	GameState &gs = GameState::get();
	hp -= amount * gs.toughness();
	shake(0.7f);
	hurt_flash(0.5f);
	audio::play("ricochet", -5.0f, grng().range(0.8f, 1.1f));
	plane_paint->set_shader_parameter("damage", saturate(1.0f - hp / 100.0f));
	if (!plane_smoke && hp < 60.0f) {
		plane_smoke = fx::smoke_trail(plane, Color(0.1f, 0.1f, 0.1f, 0.7f), 4.0f, 2.0f, 100);
	}
	if (hp <= 0.0f) {
		dead = true;
		Vector3 p = plane->get_position();
		fx::explosion(this, p, 9.0f, false);
		fx::explosion(this, Vector3(p.x, 1.0f, p.z - 40.0f), 11.0f, true);
		fx::smoke_column(this, Vector3(p.x, 1.0f, p.z - 40.0f), 5.0f, true);
		audio::play("explosion_big", 0.0f);
		plane->set_visible(false);
		shake(1.5f);
		player_down(cause, 3.2f);
	}
}

void Strafing::update_flak(float dt) {
	Vector3 p = plane->get_position();
	Rng &rng = grng();
	flak_warning = MAX(flak_warning - dt, 0.0f);
	for (GroundTarget &t : ground.targets) {
		if (!t.alive || !t.is_flak) {
			continue;
		}
		if (dead || turning) {
			t.burst_left = 0;
			continue;
		}
		Vector3 rel = p - t.pos;
		float dist = rel.length();
		// Guns engage as the aircraft approaches and lose it once it is past.
		bool in_arc = dist < FLAK_RANGE && (t.pos.z < p.z + 60.0f);
		if (in_arc) {
			flak_warning = 0.5f;
		}
		if (t.burst_left > 0) {
			t.burst_timer -= dt;
			if (t.burst_timer <= 0.0f) {
				t.burst_timer = 0.09f;
				t.burst_left--;
				Vector3 muzzle = t.pos + Vector3(0, 2.6f, 0);
				Vector3 dir = (t.burst_aim - muzzle).normalized();
				dir = (dir + rng.in_sphere() * 0.02f).normalized();
				bullets.fire(muzzle, dir * FLAK_SPEED, 1.6f, 2.5f + def->difficulty * 0.6f, 1, -1);
				if (t.burst_left % 2 == 0) {
					audio::play("flak_gun", -18.0f - clampf(dist / 40.0f, 0.0f, 12.0f), rng.range(1.2f, 1.5f));
				}
			}
			continue;
		}
		if (!in_arc) {
			continue;
		}
		t.flak_timer -= dt;
		if (t.flak_timer <= 0.0f) {
			t.flak_timer = rng.range(1.4f, 2.6f) - def->difficulty * 0.12f;
			float flight = dist / FLAK_SPEED;
			float err = lerpf(16.0f, 7.0f, saturate(def->difficulty / 5.0f));
			t.burst_aim = p + player_velocity() * flight + Vector3(rng.range(-err, err), rng.range(-err, err) * 0.5f, rng.range(-err, err));
			t.burst_left = 4;
			t.burst_timer = 0.0f;
		}
	}
}

void Strafing::check_obstacles(float dt) {
	collision_cooldown = MAX(collision_cooldown - dt, 0.0f);
	obstacle_warning = MAX(obstacle_warning - dt, 0.0f);
	if (dead || turning) {
		return;
	}
	Vector3 p = plane->get_position();
	for (const GroundTarget &t : ground.targets) {
		if (!t.is_obstacle) {
			continue;
		}
		float dz = p.z - t.pos.z;
		if (dz < -30.0f || dz > 190.0f) {
			continue;
		}
		if (std::fabs(t.pos.x - p.x) > MAX(t.hx, t.hz) + 12.0f) {
			continue;
		}
		float top = t.pos.y + t.height;
		float d = ground.footprint_distance(t, Vector3(p.x, 0, p.z));
		if (dz > 12.0f && altitude < top + 3.0f && std::fabs(t.pos.x - p.x) < MAX(t.hx, t.hz) + 7.0f) {
			obstacle_warning = 0.3f;
		}
		if (d < 5.0f && altitude < top + 0.5f && collision_cooldown <= 0.0f) {
			collision_cooldown = 1.2f;
			bool tree = t.kind == "tree" || t.kind == "poplar";
			message(tree ? "You clipped a tree!" : "You hit a building!", 2.5f, ui::RED);
			fx::bullet_puff(this, Vector3(p.x, altitude, p.z), 4.0f, tree ? Color(0.2f, 0.35f, 0.12f) : Color(0.5f, 0.45f, 0.4f));
			audio::play("explosion", -6.0f, 1.4f);
			// Bounce clear.
			altitude = MAX(altitude, top + 4.0f);
			vy = 14.0f;
			hit_player(tree ? 38.0f : 60.0f, tree ? String("Flew into the trees at zero feet.") : String("Flew into a building at zero feet."));
		}
	}
}

void Strafing::autopilot(float &ix, float &iy, bool &fire) {
	Vector3 p = plane->get_position();
	Vector3 aim = aim_point();
	ix = 0.0f;
	iy = 0.0f;
	fire = false;

	const GroundTarget *goal = nullptr;
	float best = 1e9f;
	for (const GroundTarget &t : ground.targets) {
		if (!t.alive || !t.counts) {
			continue;
		}
		float ahead = aim.z - t.pos.z;
		if (ahead < -6.0f || ahead > 260.0f || std::fabs(t.pos.x) > LATERAL_LIMIT) {
			continue;
		}
		float cost = ahead + std::fabs(t.pos.x - p.x) * 2.0f;
		if (cost < best) {
			best = cost;
			goal = &t;
		}
	}
	float want_alt = 15.0f;
	float want_x = std::sin(clock * 0.7f) * 8.0f;
	if (goal) {
		want_x = goal->pos.x;
		if (ground.footprint_distance(*goal, aim) < 4.0f + std::fabs(aim.z - goal->pos.z) * 0.02f || (std::fabs(aim.x - goal->pos.x) < 3.0f && std::fabs(aim.z - goal->pos.z) < 14.0f)) {
			fire = true;
		}
	}
	// Clear anything tall ahead.
	for (const GroundTarget &t : ground.targets) {
		if (!t.is_obstacle) {
			continue;
		}
		float dz = p.z - t.pos.z;
		if (dz > -20.0f && dz < 150.0f && std::fabs(t.pos.x - p.x) < MAX(t.hx, t.hz) + 9.0f) {
			want_alt = MAX(want_alt, t.pos.y + t.height + 7.0f);
		}
	}
	ix = clampf((want_x - p.x) * 0.12f, -1.0f, 1.0f);
	// W dives, S climbs: positive input climbs.
	iy = clampf((want_alt - altitude) * 0.25f, -1.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void Strafing::tick(float dt) {
	Input *in = Input::get_singleton();
	float ix = 0.0f;
	float iy = 0.0f;
	bool fire = false;
	if (demo) {
		autopilot(ix, iy, fire);
	} else if (!dead && !turning) {
		ix = input_x();
		iy = input_y(); // S (positive) pulls up
		fire = in->is_action_pressed("fire");
	}

	GameState &gs = GameState::get();
	float agility = gs.handling();

	vx = damp(vx, ix * 40.0f * agility, 3.5f, dt);
	vy = damp(vy, iy * 24.0f * agility, 3.5f, dt);
	if (!dead) {
		px += vx * dt;
		altitude += vy * dt;
		anchor_z -= scroll_speed * dt;
	}
	if (std::fabs(px) > LATERAL_LIMIT) {
		px = clampf(px, -LATERAL_LIMIT, LATERAL_LIMIT);
		vx = 0.0f;
	}
	if (altitude < ALT_MIN) {
		altitude = ALT_MIN;
		vy = MAX(vy, 0.0f);
	}
	if (altitude > ALT_MAX) {
		altitude = ALT_MAX;
		vy = MIN(vy, 0.0f);
	}

	bank = damp(bank, -vx / 40.0f * deg2rad(40.0f), 5.0f, dt);
	pitch = damp(pitch, vy / 24.0f * deg2rad(14.0f), 5.0f, dt);
	Vector3 ppos(px, altitude, anchor_z + PLANE_Z);
	plane->set_position(ppos);
	plane->set_rotation(Vector3(pitch, 0.0f, bank));
	plane->set_scale(Vector3(PLANE_SCALE, PLANE_SCALE, PLANE_SCALE));

	if (loco >= 0 && !ground.targets[loco].alive) {
		for (GroundTarget &t : ground.targets) {
			if (t.speed > 0.0f && t.kind != "soldier") {
				t.speed = MAX(t.speed - 5.0f * dt, 0.0f);
			}
		}
	}
	ground.update(dt);
	update_soldiers(dt);
	fire_guns(dt, fire);
	update_flak(dt);
	update_bullets(dt);
	check_obstacles(dt);

	Vector3 aim = aim_point();
	float ring = 5.0f + altitude * 0.22f;
	marker->set_position(aim + Vector3(0, 0.5f, 0));
	marker->set_scale(Vector3(ring, 1.0f, ring));
	marker->set_visible(!dead && !turning);

	terrain->set_position(Vector3(0, 0, std::floor(anchor_z / 500.0f) * 500.0f));

	// Isometric-style camera riding behind and to the left.
	cam_x = damp(cam_x, px * 0.6f, 3.0f, dt);
	cam_y = damp(cam_y, altitude * 0.5f, 2.0f, dt);
	Vector3 focus(cam_x + 10.0f, cam_y, anchor_z - 48.0f);
	const float yaw = deg2rad(-42.0f);
	const float elev = deg2rad(36.0f);
	Vector3 dir(std::sin(yaw) * std::cos(elev), std::sin(elev), std::cos(yaw) * std::cos(elev));
	Vector3 eye = focus + dir * 235.0f + shake_offset() * 3.0f;
	camera->look_at_from_position(eye, focus, Vector3(0, 1, 0));

	if (snd_engine) {
		snd_engine->set_pitch_scale(1.05f - vy * 0.004f + (dead ? 0.4f : 0.0f));
		snd_engine->set_volume_db(dead ? -60.0f : -10.0f);
	}
	if (snd_gun) {
		if (firing_now && !snd_gun->is_playing()) {
			snd_gun->play();
		} else if (!firing_now && snd_gun->is_playing()) {
			snd_gun->stop();
		}
	}

	if (dead || ending) {
		return;
	}
	if (turning) {
		turn_fade += dt;
		if (turn_fade >= 1.2f && anchor_z < 0.0f) {
			anchor_z = 120.0f;
			px = 0.0f;
			vx = 0.0f;
			altitude = 28.0f;
			pass++;
			// The column has had time to scatter and stop.
			for (GroundTarget &t : ground.targets) {
				if (t.kind != "soldier") {
					t.speed = 0.0f;
				}
			}
		}
		if (turn_fade >= 2.4f) {
			turning = false;
			turn_fade = 0.0f;
			message("Second pass - they are wide awake now", 3.0f, ui::AMBER);
		}
		return;
	}

	if (-anchor_z > run_length) {
		if (objective_met()) {
			message("Good shooting. Heading home.", 4.0f, ui::AMBER);
			result.score += 500;
			finish(true, 4.0f);
		} else if (pass < max_passes && ammo > 100) {
			turning = true;
			turn_fade = 0.0f;
			message("Coming around for another pass", 3.0f);
		} else {
			message("Too many got away. Heading home.", 4.0f, ui::RED);
			finish(false, 4.0f);
		}
	} else if (ammo <= 0 && bullets.list().empty()) {
		bool ok = objective_met();
		message(ok ? "Guns empty. Good shooting - heading home." : "Guns empty. Heading home.", 4.0f, ok ? ui::AMBER : ui::RED);
		if (ok) {
			result.score += 500;
		}
		finish(ok, 4.0f);
	}
}

void Strafing::on_finish() {
	result.targets_destroyed = ground.destroyed();
	result.targets_total = required();
}

// ---------------------------------------------------------------------------
// HUD
// ---------------------------------------------------------------------------

void Strafing::draw_hud(Canvas *c) {
	Vector2 size = hud_size();
	Ref<Font> title = ui::font_title();
	const Color white(1, 1, 1, 0.9f);
	const Color dim(1, 1, 1, 0.6f);
	const Color red(1.0f, 0.25f, 0.18f, 0.95f);

	float x = 60.0f;
	float y = size.y - 60.0f;
	c->text_shadowed(title, Vector2(x, y - 110), "AMMUNITION", 22, dim);
	c->text_shadowed(title, Vector2(x, y - 60), String::num_int64(ammo), 50, ammo < 250 ? red : white);
	c->text_shadowed(title, Vector2(x + 240, y - 110), "HEIGHT", 22, dim);
	c->text_shadowed(title, Vector2(x + 240, y - 60), String::num_int64((int64_t)(altitude * 3.28084f)) + String(" FT"), 50,
			altitude < 12.0f ? ui::AMBER : white);

	// Height tape.
	Vector2 t0(x + 470.0f, y - 120.0f);
	c->draw_rect(Rect2(t0, Vector2(14, 110)), Color(0, 0, 0, 0.45f), true);
	float k = saturate((altitude - ALT_MIN) / (ALT_MAX - ALT_MIN));
	c->draw_rect(Rect2(t0 + Vector2(2, 108.0f - 106.0f * k), Vector2(10, 106.0f * k)), ui::AMBER, true);
	c->draw_rect(Rect2(t0, Vector2(14, 110)), Color(1, 1, 1, 0.5f), false, 1.5f);

	float rx = size.x - 400.0f;
	c->text_shadowed(title, Vector2(rx, y - 86), "AIRFRAME", 22, dim);
	float h = saturate(hp / 100.0f);
	draw_bar(c, Vector2(rx, y - 66), Vector2(340, 30), h, h > 0.5f ? Color(0.5f, 0.85f, 0.45f) : (h > 0.25f ? ui::AMBER : red), "");

	int got = ground.destroyed();
	int need = required();
	c->text_shadowed(title, Vector2(size.x - 560, 70), String("TARGETS  ") + String::num_int64(got) + String(" / ") + String::num_int64(need),
			40, got >= need ? ui::AMBER : white, 2, 500);
	if (primary >= 0) {
		const GroundTarget &pt = ground.targets[primary];
		c->text_shadowed(title, Vector2(size.x - 560, 130), pt.alive ? "HEADQUARTERS STANDING" : "HEADQUARTERS DESTROYED", 26,
				pt.alive ? red : ui::AMBER, 2, 500);
		draw_bar(c, Vector2(size.x - 400, 140), Vector2(340, 10), pt.alive ? pt.hp / pt.max_hp : 0.0f, red, "");
	} else if (loco >= 0) {
		const GroundTarget &lt = ground.targets[loco];
		c->text_shadowed(title, Vector2(size.x - 560, 130), lt.alive ? "LOCOMOTIVE RUNNING" : "TRAIN STOPPED", 26,
				lt.alive ? red : ui::AMBER, 2, 500);
	}
	draw_bar(c, Vector2(size.x - 400, 86), Vector2(340, 16), (float)got / MAX(need, 1), ui::AMBER, "");

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

	if (!dead) {
		float blink = std::fmod(clock, 0.4f) < 0.26f ? 1.0f : 0.35f;
		if (obstacle_warning > 0.0f) {
			c->text_shadowed(title, Vector2(0, size.y - 190.0f), "PULL UP", 60, Color(1, 0.2f, 0.15f, blink), 1, size.x);
		} else if (flak_warning > 0.0f) {
			c->text_shadowed(title, Vector2(0, size.y - 170.0f), "GROUND FIRE", 34, Color(1, 0.3f, 0.2f, blink), 1, size.x);
		}
	}

	if (turning) {
		float f = turn_fade < 1.2f ? turn_fade / 1.0f : (2.4f - turn_fade) / 1.0f;
		c->draw_rect(Rect2(Vector2(), size), Color(0, 0, 0, saturate(f)), true);
	}
}

} // namespace ww2
