#pragma once

#include "core/fx.h"
#include "core/models.h"
#include "core/util.h"

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>

#include <functional>
#include <string>
#include <vector>

namespace ww2 {

struct GroundTarget {
	godot::Node3D *node = nullptr;
	godot::MeshInstance3D *mesh = nullptr;
	godot::Ref<godot::ShaderMaterial> paint; // parked aircraft only
	std::string kind;
	Vector3 pos;
	float yaw = 0.0f;
	float hx = 3.0f; // half extents in the target's own frame
	float hz = 3.0f;
	float height = 3.0f;
	float hp = 30.0f;
	float max_hp = 30.0f;
	float bullet_armor = 1.0f; // multiplier on bullet damage
	int score = 100;
	bool alive = true;
	bool counts = true; // part of the objective
	bool explosive = false;
	bool is_flak = false;
	bool is_obstacle = false; // can be flown into
	bool primary = false;
	float flak_timer = 0.0f;
	int burst_left = 0; // light flak fires in bursts
	float burst_timer = 0.0f;
	Vector3 burst_aim;
	float speed = 0.0f; // m/s along -Z (convoys)
	Vector3 flee; // soldiers scatter
	bool fleeing = false;
};

// Owns the destructible objects of a bombing or strafing mission.
class GroundWorld {
public:
	void setup(godot::Node3D *p_root) { root = p_root; }

	int add(const String &kind, const Vector3 &pos, float yaw_deg, bool counts);
	// Applies damage; returns true if this destroyed the target.
	bool damage(int index, float amount, bool from_bomb);
	// Blast damage with falloff around a ground point. Returns number destroyed;
	// r_touched counts the objective targets caught in the blast.
	int blast(const Vector3 &at, float radius, float amount, int *r_touched = nullptr);
	// Distance from a point to the target's footprint (0 when inside).
	float footprint_distance(const GroundTarget &t, const Vector3 &p) const;
	// First target whose box contains the point, or -1.
	int hit_test(const Vector3 &p, float margin) const;

	void update(float dt);

	int total() const;
	int destroyed() const;

	std::vector<GroundTarget> targets;
	std::function<void(int index, bool by_player)> on_destroyed;
	float fx_scale = 1.0f;

private:
	void destroy(int index);

	godot::Node3D *root = nullptr;
	std::vector<int> pending_blasts;
};

} // namespace ww2
