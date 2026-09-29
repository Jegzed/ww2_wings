#pragma once

#include "core/materials.h"
#include "core/util.h"

#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/world_environment.hpp>

#include <vector>

namespace ww2 {

enum TimeOfDay {
	TOD_MORNING,
	TOD_NOON,
	TOD_DUSK,
	TOD_OVERCAST,
};

struct WorldKit {
	godot::WorldEnvironment *env_node = nullptr;
	godot::DirectionalLight3D *sun = nullptr;
	godot::Ref<godot::Environment> environment;
	godot::Ref<godot::ShaderMaterial> sky_material;
	Color fog_color;
};

struct WorldOptions {
	TimeOfDay time = TOD_NOON;
	float shadow_distance = 400.0f;
	float fog_density = 0.00012f;
	float cloud_cover = 0.45f;
	float sun_azimuth_deg = 140.0f;
	float sun_elevation_deg = -1.0f; // < 0 keeps the time-of-day default
	float sun_scatter = 0.15f;
	bool ssao = true;
};

WorldKit build_world(godot::Node *parent, const WorldOptions &options);

// Large flat ground plane using the procedural bocage shader.
godot::MeshInstance3D *build_terrain(godot::Node *parent, godot::Ref<godot::ShaderMaterial> &r_material, float size);

// Puffy low-poly clouds in a box around the origin.
godot::MultiMeshInstance3D *build_clouds(godot::Node *parent, Rng &rng, int clusters, float area, float alt_min,
		float alt_max, float puff_size);

// Instances a cached prop mesh many times.
godot::MultiMeshInstance3D *scatter(godot::Node *parent, const String &mesh_name,
		const std::vector<Transform3D> &transforms, bool shadows = true);

} // namespace ww2
