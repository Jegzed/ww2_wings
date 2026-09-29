#include "core/mesh_builder.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

#include <unordered_map>

using namespace godot;

namespace ww2 {

static Rng g_rng((uint64_t)0xC0FFEE1944ull);

Rng &grng() {
	return g_rng;
}

void MeshBuilder::tri(const Vector3 &a, const Vector3 &b, const Vector3 &c) {
	Vector3 ta = xf.xform(a);
	Vector3 tb = xf.xform(b);
	Vector3 tc = xf.xform(c);
	if ((tb - ta).cross(tc - ta).length_squared() < 1e-12f) {
		return; // degenerate
	}
	verts.push_back(ta);
	verts.push_back(tb);
	verts.push_back(tc);
	colors.push_back(color);
	colors.push_back(color);
	colors.push_back(color);
}

void MeshBuilder::quad(const Vector3 &a, const Vector3 &b, const Vector3 &c, const Vector3 &d) {
	tri(a, b, c);
	tri(a, c, d);
}

void MeshBuilder::loft(const std::vector<Ring> &rings, bool cap_start, bool cap_end) {
	if (rings.size() < 2) {
		return;
	}
	const size_t n = rings[0].size();
	if (n < 3) {
		return;
	}

	Vector3 centroid;
	size_t count = 0;
	for (const Ring &r : rings) {
		for (const Vector3 &p : r) {
			centroid += p;
			count++;
		}
	}
	centroid /= (float)count;

	// Decide the winding by checking whether faces point away from the ring centres.
	float orient = 0.0f;
	for (size_t i = 0; i + 1 < rings.size(); i++) {
		Vector3 rc;
		for (size_t j = 0; j < n; j++) {
			rc += rings[i][j] + rings[i + 1][j];
		}
		rc /= (float)(n * 2);
		for (size_t j = 0; j < n; j++) {
			size_t k = (j + 1) % n;
			const Vector3 &a = rings[i][j];
			const Vector3 &b = rings[i][k];
			const Vector3 &c = rings[i + 1][k];
			const Vector3 &d = rings[i + 1][j];
			Vector3 nrm = (b - a).cross(d - a) + (d - c).cross(b - c);
			Vector3 fc = (a + b + c + d) * 0.25f;
			orient += nrm.dot(fc - rc);
		}
	}
	const bool flip = orient < 0.0f;

	for (size_t i = 0; i + 1 < rings.size(); i++) {
		for (size_t j = 0; j < n; j++) {
			size_t k = (j + 1) % n;
			const Vector3 &a = rings[i][j];
			const Vector3 &b = rings[i][k];
			const Vector3 &c = rings[i + 1][k];
			const Vector3 &d = rings[i + 1][j];
			if (flip) {
				quad(a, d, c, b);
			} else {
				quad(a, b, c, d);
			}
		}
	}

	auto cap = [&](const Ring &r) {
		Vector3 c;
		for (const Vector3 &p : r) {
			c += p;
		}
		c /= (float)n;
		Vector3 out = c - centroid;
		for (size_t j = 0; j < n; j++) {
			size_t k = (j + 1) % n;
			Vector3 nrm = (r[j] - c).cross(r[k] - c);
			if (nrm.dot(out) >= 0.0f) {
				tri(c, r[j], r[k]);
			} else {
				tri(c, r[k], r[j]);
			}
		}
	};
	if (cap_start) {
		cap(rings.front());
	}
	if (cap_end) {
		cap(rings.back());
	}
}

Ring MeshBuilder::ellipse_ring(const Vector3 &center, const Vector3 &axis_u, const Vector3 &axis_v, float ru, float rv,
		int segments, float exponent) {
	Ring r;
	r.reserve(segments);
	for (int i = 0; i < segments; i++) {
		float a = TAU_F * (float)i / (float)segments;
		float c = std::cos(a);
		float s = std::sin(a);
		// Superellipse: exponent 2 is a plain ellipse, higher is boxier.
		float e = 2.0f / exponent;
		float u = (c < 0 ? -1.0f : 1.0f) * std::pow(std::fabs(c), e);
		float v = (s < 0 ? -1.0f : 1.0f) * std::pow(std::fabs(s), e);
		r.push_back(center + axis_u * (u * ru) + axis_v * (v * rv));
	}
	return r;
}

void MeshBuilder::box(const Vector3 &center, const Vector3 &size) {
	tapered_box(center, size, 1.0f, 1.0f);
}

void MeshBuilder::tapered_box(const Vector3 &center, const Vector3 &size, float top_scale_x, float top_scale_z) {
	Vector3 h = size * 0.5f;
	Ring bottom = {
		center + Vector3(-h.x, -h.y, -h.z),
		center + Vector3(h.x, -h.y, -h.z),
		center + Vector3(h.x, -h.y, h.z),
		center + Vector3(-h.x, -h.y, h.z),
	};
	float tx = h.x * top_scale_x;
	float tz = h.z * top_scale_z;
	Ring top = {
		center + Vector3(-tx, h.y, -tz),
		center + Vector3(tx, h.y, -tz),
		center + Vector3(tx, h.y, tz),
		center + Vector3(-tx, h.y, tz),
	};
	loft({ bottom, top }, true, true);
}

void MeshBuilder::gable_roof(const Vector3 &center, const Vector3 &size) {
	Vector3 h = size * 0.5f;
	Vector3 a = center + Vector3(-h.x, -h.y, -h.z);
	Vector3 b = center + Vector3(h.x, -h.y, -h.z);
	Vector3 c = center + Vector3(h.x, -h.y, h.z);
	Vector3 d = center + Vector3(-h.x, -h.y, h.z);
	Vector3 r0 = center + Vector3(0, h.y, -h.z);
	Vector3 r1 = center + Vector3(0, h.y, h.z);
	quad(a, d, r1, r0); // -X slope
	quad(c, b, r0, r1); // +X slope
	tri(b, a, r0); // -Z gable
	tri(d, c, r1); // +Z gable
	quad(a, b, c, d); // underside
}

void MeshBuilder::cylinder(const Vector3 &p0, const Vector3 &p1, float r0, float r1, int segments, bool caps) {
	Vector3 axis = (p1 - p0);
	if (axis.length_squared() < 1e-10f) {
		return;
	}
	axis.normalize();
	Vector3 ref = std::fabs(axis.y) > 0.9f ? Vector3(1, 0, 0) : Vector3(0, 1, 0);
	Vector3 u = axis.cross(ref).normalized();
	Vector3 v = axis.cross(u).normalized();
	const float tiny = 0.0005f;
	Ring a = ellipse_ring(p0, u, v, MAX(r0, tiny), MAX(r0, tiny), segments);
	Ring b = ellipse_ring(p1, u, v, MAX(r1, tiny), MAX(r1, tiny), segments);
	loft({ a, b }, caps && r0 > tiny, caps && r1 > tiny);
}

void MeshBuilder::sphere(const Vector3 &center, const Vector3 &radii, int segments, int ring_count) {
	std::vector<Ring> rings;
	for (int i = 0; i <= ring_count; i++) {
		float t = (float)i / (float)ring_count;
		float phi = -PI_F * 0.5f + PI_F * t;
		float y = std::sin(phi);
		float r = MAX(std::cos(phi), 0.001f);
		rings.push_back(ellipse_ring(center + Vector3(0, y * radii.y, 0), Vector3(1, 0, 0), Vector3(0, 0, 1),
				r * radii.x, r * radii.z, segments));
	}
	loft(rings, false, false);
}

namespace {
struct Key {
	int32_t x, y, z;
	bool operator==(const Key &o) const { return x == o.x && y == o.y && z == o.z; }
};
struct KeyHash {
	size_t operator()(const Key &k) const {
		return (size_t)((uint32_t)k.x * 73856093u) ^ ((uint32_t)k.y * 19349663u) ^ ((uint32_t)k.z * 83492791u);
	}
};
inline Key make_key(const Vector3 &p) {
	return Key{ (int32_t)std::lround(p.x * 2000.0f), (int32_t)std::lround(p.y * 2000.0f),
		(int32_t)std::lround(p.z * 2000.0f) };
}
} // namespace

void MeshBuilder::add_surface(const Ref<ArrayMesh> &mesh, const Ref<Material> &material, float smooth_angle_deg) const {
	if (verts.empty()) {
		return;
	}
	const size_t tri_count = verts.size() / 3;
	std::vector<Vector3> face_normals(tri_count); // area weighted
	std::unordered_map<Key, std::vector<uint32_t>, KeyHash> shared;
	shared.reserve(verts.size());
	for (size_t t = 0; t < tri_count; t++) {
		const Vector3 &a = verts[t * 3];
		const Vector3 &b = verts[t * 3 + 1];
		const Vector3 &c = verts[t * 3 + 2];
		face_normals[t] = (b - a).cross(c - a);
		for (int k = 0; k < 3; k++) {
			shared[make_key(verts[t * 3 + k])].push_back((uint32_t)t);
		}
	}

	const float cos_limit = std::cos(deg2rad(smooth_angle_deg));

	PackedVector3Array out_verts;
	PackedVector3Array out_normals;
	PackedColorArray out_colors;
	out_verts.resize(verts.size());
	out_normals.resize(verts.size());
	out_colors.resize(verts.size());
	Vector3 *vw = out_verts.ptrw();
	Vector3 *nw = out_normals.ptrw();
	Color *cw = out_colors.ptrw();

	for (size_t t = 0; t < tri_count; t++) {
		Vector3 fn = face_normals[t].normalized();
		// Godot's front faces are clockwise, so emit a, c, b.
		static const int order[3] = { 0, 2, 1 };
		for (int k = 0; k < 3; k++) {
			size_t src = t * 3 + order[k];
			Vector3 n;
			const std::vector<uint32_t> &faces = shared[make_key(verts[src])];
			for (uint32_t f : faces) {
				Vector3 other = face_normals[f];
				float len = other.length();
				if (len > 0.0f && (other / len).dot(fn) >= cos_limit) {
					n += other;
				}
			}
			n = n.length_squared() > 0.0f ? n.normalized() : fn;
			size_t dst = t * 3 + k;
			vw[dst] = verts[src];
			nw[dst] = n;
			cw[dst] = colors[src];
		}
	}

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = out_verts;
	arrays[Mesh::ARRAY_NORMAL] = out_normals;
	arrays[Mesh::ARRAY_COLOR] = out_colors;
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	if (material.is_valid()) {
		mesh->surface_set_material(mesh->get_surface_count() - 1, material);
	}
}

Ref<ArrayMesh> MeshBuilder::build(const Ref<Material> &material, float smooth_angle_deg) const {
	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	add_surface(mesh, material, smooth_angle_deg);
	return mesh;
}

} // namespace ww2
