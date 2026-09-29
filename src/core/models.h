#pragma once

#include "core/materials.h"
#include "core/mesh_builder.h"

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>

#include <vector>

namespace ww2 {
namespace models {

using godot::MeshInstance3D;
using godot::Node3D;

enum PlaneType {
	PLANE_P47,
	PLANE_BF109,
	PLANE_FW190,
};

struct PlaneInfo {
	float length;
	float span;
	float prop_radius;
	float hit_radius;
	Vector3 gun_left;
	Vector3 gun_right;
	Vector3 exhaust;
};

const PlaneInfo &plane_info(PlaneType type);

// Builds a plane hierarchy: "Body" (painted + detail surfaces), "Canopy" and "Prop".
// Nose points down -Z, right wing along +X.
Node3D *make_plane(PlaneType type, const Ref<godot::ShaderMaterial> &paint, bool spinning_prop = true);

// Static props, cached by name. Known names:
// truck, tank, halftrack, flak, staff_car, house, barn, warehouse, factory, church,
// hangar, tower, fuel_tank, loco, boxcar, tanker, flatcar, tree, poplar, pine, bush,
// soldier, bomb, bridge, crater, sandbags, tent, crate_stack
Ref<ArrayMesh> mesh(const String &name);
MeshInstance3D *instance(const String &name);

// Darkened material for wrecks.
Ref<godot::StandardMaterial3D> burnt_material();

void clear_cache();

} // namespace models
} // namespace ww2
