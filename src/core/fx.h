#pragma once

#include "core/util.h"

#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>

#include <vector>

namespace ww2 {
namespace fx {

using godot::GPUParticles3D;
using godot::Node;
using godot::Node3D;

// One-shot effects free themselves.
void explosion(Node *parent, const Vector3 &pos, float size, bool ground = false);
void flak_burst(Node *parent, const Vector3 &pos, float size);
void bullet_puff(Node *parent, const Vector3 &pos, float size, const Color &color);
void spark(Node *parent, const Vector3 &pos, float size);

// Persistent emitters; caller owns them (they are children of `attach`).
GPUParticles3D *smoke_trail(Node3D *attach, const Color &color, float size, float lifetime, int amount);
GPUParticles3D *fire_trail(Node3D *attach, float size);
GPUParticles3D *smoke_column(Node *parent, const Vector3 &pos, float size, bool with_fire);

// Tracer rounds drawn with one MultiMesh.
struct Bullet {
	Vector3 pos;
	Vector3 prev;
	Vector3 vel;
	float life = 0.0f;
	float damage = 1.0f;
	int team = 0;
	int owner = -1;
	bool alive = true;
};

class Bullets {
public:
	void setup(Node *parent, int capacity, float tracer_length, float tracer_width);
	Bullet &fire(const Vector3 &pos, const Vector3 &vel, float life, float damage, int team, int owner);
	// Integrates motion. `gravity` pulls rounds down (m/s^2).
	void step(float dt, float gravity);
	void render();
	std::vector<Bullet> &list() { return bullets; }

private:
	std::vector<Bullet> bullets;
	godot::MultiMeshInstance3D *node = nullptr;
	int capacity = 0;
	float length = 12.0f;
	float width = 0.12f;
};

} // namespace fx
} // namespace ww2
