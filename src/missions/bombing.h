#pragma once

#include "missions/ground.h"
#include "missions/mission_base.h"

namespace ww2 {

// Top-down bombing run: steer over the targets, lead them, dodge the flak.
class Bombing : public MissionBase {
	GDCLASS(Bombing, MissionBase)

protected:
	static void _bind_methods() {}

	void build() override;
	void tick(float dt) override;
	void draw_hud(Canvas *c) override;
	void on_finish() override;

private:
	struct Bomb {
		godot::Node3D *node = nullptr;
		Vector3 pos;
		Vector3 vel;
	};
	struct Shell {
		Vector3 at;
		float time;
	};

	void layout_rail_yard(Rng &rng);
	void layout_bridge(Rng &rng);
	void layout_convoy(Rng &rng);
	void layout_harbour(Rng &rng);
	void layout_launch_site(Rng &rng);
	void layout_airfield(Rng &rng);
	void scatter_scenery(Rng &rng, float length);
	void add_farm(Rng &rng, const Vector3 &at);
	void add_village(Rng &rng, const Vector3 &at, bool church);
	void drop_bomb();
	void explode_bomb(const Bomb &b);
	void update_flak(float dt);
	void hit_player(float amount, const String &cause);
	Vector3 predicted_impact() const;
	Vector3 player_velocity() const;
	void autopilot(float &ix, float &iy, bool &drop);
	bool objective_met() const;
	int required() const;

	GroundWorld ground;
	fx::Bullets tracers;
	std::vector<Bomb> bombs;
	std::vector<Shell> shells;

	godot::Node3D *plane = nullptr;
	godot::Ref<godot::ShaderMaterial> plane_paint;
	godot::GPUParticles3D *plane_smoke = nullptr;
	godot::MeshInstance3D *marker = nullptr;
	godot::MeshInstance3D *terrain = nullptr;
	godot::AudioStreamPlayer *snd_engine = nullptr;
	godot::AudioStreamPlayer *snd_wind = nullptr;

	float run_length = 4000.0f;
	float anchor_z = 150.0f;
	Vector3 offset; // player position relative to the anchor (x lateral, z along)
	Vector3 velocity; // relative velocity
	float altitude = 150.0f;
	float scroll_speed = 40.0f;
	float bank = 0.0f;
	float hp = 100.0f;
	int bombs_left = 14;
	int bombs_total = 14;
	float drop_cooldown = 0.0f;
	int pass = 1;
	int max_passes = 2;
	float turn_fade = 0.0f; // >0 while coming around
	bool turning = false;
	bool dead = false;
	float empty_timer = 0.0f;
	int primary = -1;
	float cam_x = 0.0f;
	float flak_warning = 0.0f;
};

} // namespace ww2
