#pragma once

#include "core/fx.h"
#include "core/models.h"
#include "missions/mission_base.h"

namespace ww2 {

// 3D air combat: the player's Thunderbolt and wingmen against German fighters.
class Dogfight : public MissionBase {
	GDCLASS(Dogfight, MissionBase)

protected:
	static void _bind_methods() {}

	void build() override;
	void tick(float dt) override;
	void draw_hud(Canvas *c) override;
	void on_finish() override;

private:
	struct Aircraft {
		godot::Node3D *node = nullptr;
		godot::Ref<godot::ShaderMaterial> paint;
		models::PlaneType type = models::PLANE_P47;
		int team = 0;
		bool player = false;
		String callsign;
		int role = 0; // 0 fighter, 1 bomber, 2 flying bomb
		Vector3 waypoint; // bombers and flying bombs fly straight for this
		float gun_timer = 0.0f; // rear gunner
		bool through = false; // reached the objective line

		Vector3 pos;
		Basis basis;
		float speed = 110.0f;
		float throttle = 0.8f;
		float roll_rate = 0.0f;
		float pitch_rate = 0.0f;
		float max_speed = 160.0f;
		float roll_max = 110.0f;
		float pitch_max = 42.0f;

		float hp = 100.0f;
		float max_hp = 100.0f;
		bool alive = true;
		bool dying = false;
		float dying_time = 0.0f;
		float spin = 1.0f;
		int last_attacker = -1;

		float fire_timer = 0.0f;
		bool firing = false;
		int gun_side = 0;
		godot::GPUParticles3D *smoke = nullptr;
		godot::GPUParticles3D *fire = nullptr;
		int smoke_level = 0;

		// AI
		float skill = 0.6f;
		int target = -1;
		float retarget = 0.0f;
		float evade_time = 0.0f;
		Vector3 evade_dir;
		float burst = 0.0f;
		Vector3 assist; // direction the AI "rudders" toward
	};

	struct Controls {
		float pitch = 0.0f;
		float roll = 0.0f;
		float throttle = 0.8f;
		bool fire = false;
	};

	int spawn(models::PlaneType type, mats::PaintScheme paint, int team, const Vector3 &pos, float yaw_deg, float skill,
			bool player, const String &callsign);
	Controls control_player(Aircraft &a, float dt);
	Controls control_ai(int index, float dt);
	void simulate(Aircraft &a, const Controls &c, float dt);
	void fire_guns(int index, float dt);
	void update_bullets(float dt);
	void damage(int victim, float amount, int attacker);
	void kill(int victim, int attacker);
	void update_dying(int index, float dt);
	void update_smoke(Aircraft &a);
	void update_camera(float dt);
	void update_audio(float dt);
	void update_gunners(float dt);
	void update_objective(float dt);
	void spawn_wave(int count, const Vector3 &around, bool fw_mix);
	String objective_text() const;
	int count_alive(int team) const;
	float distance_gain(const Vector3 &pos) const;

	std::vector<Aircraft> planes;
	fx::Bullets bullets;
	godot::MeshInstance3D *terrain = nullptr;
	godot::AudioStreamPlayer *snd_engine = nullptr;
	godot::AudioStreamPlayer *snd_wind = nullptr;
	godot::AudioStreamPlayer *snd_gun = nullptr;
	godot::AudioStreamPlayer *snd_enemy_gun = nullptr;
	godot::AudioStreamPlayer *snd_alarm = nullptr;

	int ammo = 1600;
	int initial_enemies = 0;
	Vector3 cam_pos;
	Vector3 cam_up = Vector3(0, 1, 0);
	float cam_fov = 62.0f;
	bool cam_ready = false;
	float enemy_fire_heard = 0.0f;
	bool player_crashed = false;
	float victory_delay = 0.0f;
	float empty_time = 0.0f;

	// Variant bookkeeping.
	int bombers_total = 0;
	int bombers_through = 0;
	int missiles_total = 0;
	int missiles_spawned = 0;
	int missiles_down = 0;
	int missiles_gone = 0;
	float spawn_timer = 0.0f;
	int wave = 0;
	int waves_total = 1;
	float objective_z = -1e9f;
	bool objective_done = false;
};

} // namespace ww2
