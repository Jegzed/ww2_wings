#pragma once

#include "core/world_env.h"

#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/node3d.hpp>

#include <vector>

namespace ww2 {

// Cinematic 3D scene behind the menus: a flight of Thunderbolts above the clouds.
class Backdrop : public godot::Node3D {
	GDCLASS(Backdrop, godot::Node3D)

public:
	void _ready() override;
	void _process(double delta) override;
	void _exit_tree() override;

	void set_time(TimeOfDay t) { time = t; }
	void set_quiet(bool q) { quiet = q; }

protected:
	static void _bind_methods() {}

private:
	TimeOfDay time = TOD_DUSK;
	bool quiet = false;
	godot::Node3D *flight = nullptr;
	godot::Camera3D *camera = nullptr;
	godot::MeshInstance3D *terrain = nullptr;
	godot::AudioStreamPlayer *engine = nullptr;
	std::vector<godot::Node3D *> planes;
	std::vector<Vector3> offsets;
	float clock = 0.0f;
};

} // namespace ww2
