#pragma once

#include "core/util.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>

#include <vector>

namespace ww2 {

using godot::ArrayMesh;
using godot::Material;
using godot::Ref;

typedef std::vector<Vector3> Ring;

// Accumulates triangles for one mesh surface. Triangles are given in
// counter-clockwise order (seen from outside); normals are generated with an
// angle threshold so curved parts shade smooth and hard edges stay crisp.
class MeshBuilder {
public:
	void set_color(const Color &p_color) { color = p_color; }
	void set_transform(const Transform3D &p_xf) { xf = p_xf; }
	void reset_transform() { xf = Transform3D(); }
	const Transform3D &get_transform() const { return xf; }

	void tri(const Vector3 &a, const Vector3 &b, const Vector3 &c);
	void quad(const Vector3 &a, const Vector3 &b, const Vector3 &c, const Vector3 &d);

	// Skins a sequence of closed rings. Orientation is detected automatically.
	void loft(const std::vector<Ring> &rings, bool cap_start, bool cap_end);

	void box(const Vector3 &center, const Vector3 &size);
	// Box with a narrower top, handy for roofs, hulls and turrets.
	void tapered_box(const Vector3 &center, const Vector3 &size, float top_scale_x, float top_scale_z);
	void cylinder(const Vector3 &p0, const Vector3 &p1, float r0, float r1, int segments, bool caps = true);
	void sphere(const Vector3 &center, const Vector3 &radii, int segments = 12, int rings = 8);
	// Gabled roof prism along Z, sitting on top of `base_y`.
	void gable_roof(const Vector3 &center, const Vector3 &size);

	bool is_empty() const { return verts.empty(); }

	void add_surface(const Ref<ArrayMesh> &mesh, const Ref<Material> &material, float smooth_angle_deg = 50.0f) const;
	Ref<ArrayMesh> build(const Ref<Material> &material, float smooth_angle_deg = 50.0f) const;

	static Ring ellipse_ring(const Vector3 &center, const Vector3 &axis_u, const Vector3 &axis_v, float ru, float rv,
			int segments, float exponent = 2.0f);

private:
	std::vector<Vector3> verts; // 3 per triangle, CCW
	std::vector<Color> colors;
	Color color = Color(1, 1, 1, 1);
	Transform3D xf;
};

} // namespace ww2
