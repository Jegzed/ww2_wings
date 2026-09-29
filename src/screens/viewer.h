#pragma once

#include "core/world_env.h"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/node3d.hpp>

namespace ww2 {

// Debug scene that lines up every procedural model for visual inspection.
class Viewer : public godot::Node3D {
	GDCLASS(Viewer, godot::Node3D)

public:
	void _ready() override;
	void _process(double delta) override;
	void set_shot(int index);

protected:
	static void _bind_methods();

private:
	godot::Camera3D *camera = nullptr;
	float clock = 0.0f;
	int shot = 0;
};

} // namespace ww2
