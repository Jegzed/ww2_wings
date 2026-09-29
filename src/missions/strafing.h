#pragma once

#include "missions/ground.h"
#include "missions/mission_base.h"

namespace ww2 {

// Low-level attack seen from an isometric camera: slide, climb and dive
// along a road or airfield while gunning everything on it.
class Strafing : public MissionBase {
	GDCLASS(Strafing, MissionBase)

protected:
	static void _bind_methods() {}

	void build() override;
	void tick(float dt) override;
	void draw_hud(Canvas *c) override;
	void on_finish() override;

private:
	void layout_convoy(Rng &rng);
	void layout_airfield(Rng &rng);
	void scatter_scenery(Rng &rng, float length, float clear_half_width);
	void add_soldiers(Rng &rng, const Vector3 &at, int count);
	void fire_guns(float dt, bool firing);
	void update_bullets(float dt);
	void update_flak(float dt);
	void update_soldiers(float dt);
	void check_obstacles(float dt);
	void hit_player(float amount, const String &cause);
	void autopilot(float &ix, float &iy, bool &fire);
	Vector3 aim_point() const;
	Vector3 player_velocity() const;
	bool objective_met() const;
	int required() const;

	GroundWorld ground;
	fx::Bullets bullets;

	godot::Node3D *plane = nullptr;
	godot::Ref<godot::ShaderMaterial> plane_paint;
	godot::GPUParticles3D *plane_smoke = nullptr;
	godot::MeshInstance3D *marker = nullptr;
	godot::MeshInstance3D *terrain = nullptr;
	godot::AudioStreamPlayer *snd_engine = nullptr;
	godot::AudioStreamPlayer *snd_wind = nullptr;
	godot::AudioStreamPlayer *snd_gun = nullptr;

	float run_length = 3800.0f;
	float anchor_z = 120.0f;
	float px = 0.0f;
	float altitude = 26.0f;
	float vx = 0.0f;
	float vy = 0.0f;
	float bank = 0.0f;
	float pitch = 0.0f;
	float scroll_speed = 52.0f;
	float hp = 100.0f;
	int ammo = 1800;
	float fire_timer = 0.0f;
	int gun_side = 0;
	bool firing_now = false;
	bool dead = false;
	int pass = 1;
	int max_passes = 2;
	bool turning = false;
	float turn_fade = 0.0f;
	float collision_cooldown = 0.0f;
	float obstacle_warning = 0.0f;
	float flak_warning = 0.0f;
	float cam_x = 0.0f;
	float cam_y = 0.0f;
	float puff_budget = 0.0f;
};

} // namespace ww2
