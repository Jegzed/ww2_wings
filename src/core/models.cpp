#include "core/models.h"

#include <godot_cpp/classes/geometry_instance3d.hpp>

#include <map>
#include <string>

using namespace godot;

namespace ww2 {
namespace models {

namespace {

const float PART_FUSELAGE = 1.0f;
const float PART_WING = 0.8f;
const float PART_STAB = 0.6f;
const float PART_FIN = 0.5f;

struct Station {
	float z, hw, hh, y, exp;
};

struct WingSpec {
	float root_x, tip_x;
	float root_chord, tip_chord;
	float qc_z; // quarter-chord line at the root
	float sweep; // quarter-chord z offset at the tip
	float dihedral_deg;
	float y;
	float t_root, t_tip;
	bool elliptical;
};

struct PlaneSpec {
	std::vector<Station> fuselage;
	std::vector<Station> canopy;
	WingSpec wing;
	WingSpec stab;
	WingSpec fin;
	PlaneInfo info;
	float nose_z;
	float spinner_len;
	float spinner_r;
	Color spinner_color;
	Color prop_tip;
	int blades;
	bool radial;
	Vector3 pilot;
	float canopy_frame_exp;

	// Optional features for bombers and the flying bomb.
	struct Nacelle {
		float x, y, z0, z1, r;
		bool prop;
	};
	std::vector<Nacelle> nacelles;
	std::vector<Station> nose_glass; // glazed nose, lofted like the canopy
	bool nose_prop = true;
	bool has_pilot = true;
	bool has_canopy = true;
	bool fighter_guns = true;
	bool turret = false;
};

PlaneSpec spec_p47() {
	PlaneSpec s;
	s.fuselage = {
		{ -4.62f, 0.78f, 0.88f, -0.02f, 2.0f },
		{ -4.45f, 0.90f, 1.00f, -0.02f, 2.0f },
		{ -3.90f, 0.96f, 1.06f, -0.03f, 2.1f },
		{ -2.80f, 0.95f, 1.08f, -0.05f, 2.2f },
		{ -1.20f, 0.88f, 1.06f, -0.05f, 2.3f },
		{ 0.40f, 0.76f, 0.98f, 0.0f, 2.3f },
		{ 1.80f, 0.60f, 0.82f, 0.08f, 2.2f },
		{ 3.20f, 0.42f, 0.62f, 0.18f, 2.1f },
		{ 4.60f, 0.22f, 0.40f, 0.30f, 2.0f },
		{ 5.70f, 0.07f, 0.20f, 0.40f, 2.0f },
		{ 6.05f, 0.02f, 0.06f, 0.44f, 2.0f },
	};
	s.canopy = {
		{ -1.25f, 0.05f, 0.05f, 0.80f, 2.0f },
		{ -0.95f, 0.30f, 0.34f, 0.86f, 2.4f },
		{ -0.45f, 0.40f, 0.52f, 0.88f, 2.2f },
		{ 0.30f, 0.42f, 0.56f, 0.88f, 2.0f },
		{ 1.00f, 0.38f, 0.48f, 0.86f, 2.0f },
		{ 1.55f, 0.24f, 0.28f, 0.84f, 2.0f },
		{ 1.90f, 0.04f, 0.05f, 0.80f, 2.0f },
	};
	s.wing = { 0.35f, 6.2f, 2.95f, 1.0f, -1.25f, 0.0f, 5.5f, -0.48f, 0.15f, 0.10f, true };
	s.stab = { 0.1f, 2.5f, 1.55f, 0.7f, 4.95f, 0.1f, 0.0f, 0.42f, 0.10f, 0.08f, true };
	s.fin = { 0.35f, 2.15f, 1.9f, 0.85f, 5.1f, 0.25f, 0.0f, 0.0f, 0.10f, 0.08f, true };
	s.info = { 11.0f, 12.4f, 1.9f, 5.5f, Vector3(-2.9f, -0.22f, -2.4f), Vector3(2.9f, -0.22f, -2.4f),
		Vector3(0.0f, -0.9f, 2.0f) };
	s.nose_z = -4.62f;
	s.spinner_len = 0.55f;
	s.spinner_r = 0.28f;
	s.spinner_color = Color(0.72, 0.06, 0.05);
	s.prop_tip = Color(0.9, 0.75, 0.1);
	s.blades = 4;
	s.radial = true;
	s.pilot = Vector3(0.0f, 1.12f, 0.35f);
	s.canopy_frame_exp = 2.2f;
	return s;
}

PlaneSpec spec_bf109() {
	PlaneSpec s;
	s.fuselage = {
		{ -3.70f, 0.30f, 0.34f, 0.0f, 2.0f },
		{ -3.40f, 0.40f, 0.48f, -0.02f, 2.4f },
		{ -2.60f, 0.45f, 0.58f, -0.04f, 2.8f },
		{ -1.20f, 0.46f, 0.64f, -0.02f, 2.8f },
		{ 0.20f, 0.43f, 0.62f, 0.0f, 2.5f },
		{ 1.60f, 0.33f, 0.50f, 0.05f, 2.2f },
		{ 3.00f, 0.21f, 0.34f, 0.12f, 2.0f },
		{ 4.20f, 0.10f, 0.20f, 0.20f, 2.0f },
		{ 4.95f, 0.02f, 0.06f, 0.26f, 2.0f },
	};
	s.canopy = {
		{ -1.05f, 0.05f, 0.05f, 0.50f, 3.0f },
		{ -0.75f, 0.30f, 0.34f, 0.54f, 3.2f },
		{ -0.30f, 0.34f, 0.44f, 0.56f, 3.4f },
		{ 0.50f, 0.34f, 0.44f, 0.56f, 3.4f },
		{ 1.10f, 0.28f, 0.34f, 0.52f, 3.0f },
		{ 1.70f, 0.05f, 0.06f, 0.46f, 2.4f },
	};
	s.wing = { 0.25f, 4.95f, 2.15f, 1.05f, -0.75f, 0.12f, 6.0f, -0.42f, 0.14f, 0.10f, false };
	s.stab = { 0.05f, 1.55f, 1.15f, 0.65f, 4.1f, 0.12f, 0.0f, 0.62f, 0.10f, 0.08f, false };
	s.fin = { 0.2f, 1.45f, 1.45f, 0.75f, 4.2f, 0.3f, 0.0f, 0.0f, 0.10f, 0.08f, false };
	s.info = { 9.0f, 9.9f, 1.5f, 4.6f, Vector3(-0.22f, 0.42f, -3.2f), Vector3(0.22f, 0.42f, -3.2f),
		Vector3(0.45f, -0.1f, -2.4f) };
	s.nose_z = -3.70f;
	s.spinner_len = 0.75f;
	s.spinner_r = 0.30f;
	s.spinner_color = Color(0.06, 0.07, 0.06);
	s.prop_tip = Color(0.1, 0.1, 0.1);
	s.blades = 3;
	s.radial = false;
	s.pilot = Vector3(0.0f, 0.74f, 0.25f);
	s.canopy_frame_exp = 3.4f;
	return s;
}

PlaneSpec spec_fw190() {
	PlaneSpec s;
	s.fuselage = {
		{ -3.85f, 0.58f, 0.62f, 0.0f, 2.0f },
		{ -3.65f, 0.66f, 0.70f, 0.0f, 2.0f },
		{ -3.00f, 0.68f, 0.74f, -0.02f, 2.1f },
		{ -1.60f, 0.60f, 0.74f, -0.02f, 2.4f },
		{ 0.0f, 0.50f, 0.68f, 0.0f, 2.4f },
		{ 1.60f, 0.37f, 0.52f, 0.06f, 2.2f },
		{ 3.10f, 0.22f, 0.35f, 0.14f, 2.0f },
		{ 4.40f, 0.09f, 0.20f, 0.22f, 2.0f },
		{ 5.0f, 0.02f, 0.06f, 0.27f, 2.0f },
	};
	s.canopy = {
		{ -0.95f, 0.05f, 0.05f, 0.58f, 2.4f },
		{ -0.65f, 0.30f, 0.32f, 0.62f, 2.6f },
		{ -0.10f, 0.36f, 0.42f, 0.64f, 2.2f },
		{ 0.70f, 0.34f, 0.38f, 0.62f, 2.0f },
		{ 1.50f, 0.22f, 0.22f, 0.56f, 2.0f },
		{ 2.10f, 0.04f, 0.05f, 0.50f, 2.0f },
	};
	s.wing = { 0.3f, 5.25f, 2.3f, 1.2f, -0.85f, 0.1f, 5.0f, -0.45f, 0.15f, 0.10f, false };
	s.stab = { 0.05f, 1.8f, 1.25f, 0.7f, 4.2f, 0.12f, 0.0f, 0.45f, 0.10f, 0.08f, false };
	s.fin = { 0.2f, 1.6f, 1.6f, 0.8f, 4.3f, 0.3f, 0.0f, 0.0f, 0.10f, 0.08f, false };
	s.info = { 9.0f, 10.5f, 1.65f, 4.8f, Vector3(-0.8f, -0.3f, -2.6f), Vector3(0.8f, -0.3f, -2.6f),
		Vector3(0.6f, -0.3f, -2.4f) };
	s.nose_z = -3.85f;
	s.spinner_len = 0.6f;
	s.spinner_r = 0.32f;
	s.spinner_color = Color(0.06, 0.07, 0.06);
	s.prop_tip = Color(0.1, 0.1, 0.1);
	s.blades = 3;
	s.radial = true;
	s.pilot = Vector3(0.0f, 0.84f, 0.3f);
	s.canopy_frame_exp = 2.3f;
	return s;
}

PlaneSpec spec_b26() {
	PlaneSpec s;
	s.fuselage = {
		{ -8.6f, 0.30f, 0.34f, 0.0f, 2.0f },
		{ -7.9f, 0.72f, 0.80f, 0.0f, 2.0f },
		{ -6.4f, 1.02f, 1.10f, 0.0f, 2.2f },
		{ -3.0f, 1.12f, 1.22f, 0.0f, 2.3f },
		{ 0.5f, 1.10f, 1.20f, 0.0f, 2.3f },
		{ 3.5f, 0.92f, 1.05f, 0.08f, 2.2f },
		{ 6.2f, 0.62f, 0.78f, 0.22f, 2.0f },
		{ 8.4f, 0.36f, 0.50f, 0.36f, 2.0f },
		{ 9.3f, 0.10f, 0.20f, 0.42f, 2.0f },
	};
	s.canopy = {
		{ -6.1f, 0.05f, 0.05f, 1.10f, 2.0f },
		{ -5.6f, 0.70f, 0.42f, 1.14f, 2.6f },
		{ -4.6f, 0.78f, 0.52f, 1.16f, 2.6f },
		{ -3.4f, 0.74f, 0.44f, 1.16f, 2.6f },
		{ -2.6f, 0.05f, 0.05f, 1.12f, 2.0f },
	};
	s.nose_glass = {
		{ -8.55f, 0.30f, 0.34f, 0.0f, 2.0f },
		{ -8.0f, 0.70f, 0.78f, 0.0f, 2.0f },
		{ -7.1f, 0.94f, 1.02f, 0.0f, 2.1f },
		{ -6.5f, 1.00f, 1.08f, 0.0f, 2.1f },
	};
	s.wing = { 1.0f, 10.8f, 4.3f, 1.7f, -1.4f, 0.5f, 2.5f, 0.35f, 0.14f, 0.09f, false };
	s.stab = { 0.2f, 4.0f, 2.0f, 1.0f, 7.6f, 0.3f, 6.0f, 0.55f, 0.10f, 0.08f, false };
	s.fin = { 0.5f, 3.6f, 2.8f, 1.2f, 7.6f, 0.8f, 0.0f, 0.0f, 0.10f, 0.08f, false };
	s.nacelles = { { -3.7f, -0.1f, -4.8f, 1.2f, 0.88f, true }, { 3.7f, -0.1f, -4.8f, 1.2f, 0.88f, true } };
	s.info = { 17.8f, 21.6f, 2.0f, 9.5f, Vector3(-3.7f, -0.1f, -5.0f), Vector3(3.7f, -0.1f, -5.0f),
		Vector3(0.0f, -0.9f, 2.0f), Vector3(0.0f, 0.55f, 9.0f) };
	s.nose_z = -100.0f;
	s.spinner_len = 0.7f;
	s.spinner_r = 0.34f;
	s.spinner_color = Color(0.2, 0.22, 0.14);
	s.prop_tip = Color(0.9, 0.75, 0.1);
	s.blades = 4;
	s.radial = true;
	s.pilot = Vector3(0.0f, 1.42f, -4.4f);
	s.canopy_frame_exp = 2.6f;
	s.nose_prop = false;
	s.fighter_guns = false;
	s.turret = true;
	return s;
}

PlaneSpec spec_ju88() {
	PlaneSpec s;
	s.fuselage = {
		{ -7.0f, 0.55f, 0.62f, 0.05f, 2.2f },
		{ -6.3f, 0.78f, 0.90f, 0.05f, 2.3f },
		{ -5.0f, 0.86f, 0.98f, 0.02f, 2.3f },
		{ -2.0f, 0.86f, 0.96f, 0.0f, 2.3f },
		{ 1.5f, 0.78f, 0.86f, 0.0f, 2.2f },
		{ 4.5f, 0.52f, 0.62f, 0.08f, 2.0f },
		{ 6.8f, 0.26f, 0.36f, 0.18f, 2.0f },
		{ 7.5f, 0.08f, 0.16f, 0.22f, 2.0f },
	};
	s.canopy = {
		{ -6.8f, 0.05f, 0.05f, 0.62f, 2.0f },
		{ -6.2f, 0.62f, 0.50f, 0.72f, 2.8f },
		{ -5.0f, 0.74f, 0.62f, 0.80f, 2.8f },
		{ -3.8f, 0.72f, 0.56f, 0.80f, 2.8f },
		{ -3.0f, 0.05f, 0.05f, 0.70f, 2.0f },
	};
	s.nose_glass = {
		{ -7.4f, 0.20f, 0.22f, 0.05f, 2.0f },
		{ -7.0f, 0.56f, 0.62f, 0.05f, 2.2f },
		{ -6.4f, 0.74f, 0.84f, 0.05f, 2.3f },
	};
	s.wing = { 0.8f, 10.0f, 4.0f, 1.6f, -2.2f, 0.9f, 5.0f, -0.5f, 0.14f, 0.09f, false };
	s.stab = { 0.2f, 3.6f, 1.8f, 0.9f, 6.2f, 0.3f, 0.0f, 0.4f, 0.10f, 0.08f, false };
	s.fin = { 0.4f, 2.6f, 2.4f, 1.1f, 6.0f, 0.8f, 0.0f, 0.0f, 0.10f, 0.08f, false };
	s.nacelles = { { -3.3f, -0.35f, -5.4f, 0.4f, 0.78f, true }, { 3.3f, -0.35f, -5.4f, 0.4f, 0.78f, true } };
	s.info = { 14.4f, 20.0f, 1.75f, 8.5f, Vector3(-3.3f, -0.35f, -5.6f), Vector3(3.3f, -0.35f, -5.6f),
		Vector3(0.0f, -0.8f, 1.5f), Vector3(0.0f, 0.75f, -3.6f) };
	s.nose_z = -100.0f;
	s.spinner_len = 0.7f;
	s.spinner_r = 0.32f;
	s.spinner_color = Color(0.06, 0.07, 0.06);
	s.prop_tip = Color(0.1, 0.1, 0.1);
	s.blades = 3;
	s.radial = false;
	s.pilot = Vector3(-0.3f, 0.95f, -5.2f);
	s.canopy_frame_exp = 2.8f;
	s.nose_prop = false;
	s.fighter_guns = false;
	s.turret = true;
	return s;
}

PlaneSpec spec_v1() {
	PlaneSpec s;
	s.fuselage = {
		{ -4.2f, 0.04f, 0.04f, 0.0f, 2.0f },
		{ -3.4f, 0.30f, 0.30f, 0.0f, 2.0f },
		{ -2.0f, 0.42f, 0.42f, 0.0f, 2.0f },
		{ 1.2f, 0.42f, 0.42f, 0.0f, 2.0f },
		{ 3.2f, 0.26f, 0.26f, 0.0f, 2.0f },
		{ 4.1f, 0.06f, 0.06f, 0.0f, 2.0f },
	};
	s.canopy = {};
	s.wing = { 0.3f, 2.7f, 1.05f, 1.05f, -0.2f, 0.0f, 0.0f, 0.0f, 0.10f, 0.10f, false };
	s.stab = { 0.2f, 1.1f, 0.7f, 0.5f, 3.2f, 0.0f, 0.0f, 0.05f, 0.10f, 0.08f, false };
	s.fin = { 0.2f, 0.9f, 0.7f, 0.5f, 3.2f, 0.1f, 0.0f, 0.0f, 0.10f, 0.08f, false };
	s.nacelles = { { 0.0f, 0.78f, -0.8f, 3.6f, 0.33f, false } };
	s.info = { 8.3f, 5.4f, 0.0f, 4.0f, Vector3(), Vector3(), Vector3(0.0f, 0.78f, 3.7f), Vector3() };
	s.nose_z = -100.0f;
	s.spinner_len = 0.0f;
	s.spinner_r = 0.0f;
	s.spinner_color = Color(0.3, 0.3, 0.3);
	s.prop_tip = Color(0.1, 0.1, 0.1);
	s.blades = 0;
	s.radial = false;
	s.pilot = Vector3();
	s.canopy_frame_exp = 2.0f;
	s.nose_prop = false;
	s.has_pilot = false;
	s.has_canopy = false;
	s.fighter_guns = false;
	return s;
}

const PlaneSpec &get_spec(PlaneType type) {
	static PlaneSpec p47 = spec_p47();
	static PlaneSpec bf109 = spec_bf109();
	static PlaneSpec fw190 = spec_fw190();
	static PlaneSpec b26 = spec_b26();
	static PlaneSpec ju88 = spec_ju88();
	static PlaneSpec v1 = spec_v1();
	switch (type) {
		case PLANE_BF109:
			return bf109;
		case PLANE_FW190:
			return fw190;
		case PLANE_B26:
			return b26;
		case PLANE_JU88:
			return ju88;
		case PLANE_V1:
			return v1;
		default:
			return p47;
	}
}

Ring station_ring(const Station &s, int segments) {
	return MeshBuilder::ellipse_ring(Vector3(0, s.y, s.z), Vector3(1, 0, 0), Vector3(0, 1, 0), s.hw, s.hh, segments,
			s.exp);
}

// Thickness distribution of a NACA 4-digit section.
float naca(float x) {
	return 5.0f * (0.2969f * std::sqrt(x) - 0.126f * x - 0.3516f * x * x + 0.2843f * x * x * x -
						  0.1036f * x * x * x * x);
}

Ring airfoil_ring(float span_pos, float y, float le_z, float chord, float thickness, float side) {
	static const float xs[] = { 1.0f, 0.78f, 0.55f, 0.34f, 0.17f, 0.06f, 0.0f };
	Ring r;
	for (int i = 0; i < 7; i++) {
		float x = xs[i];
		float t = naca(x) * thickness * chord * 1.15f + 0.004f;
		r.push_back(Vector3(span_pos * side, y + t, le_z + x * chord));
	}
	for (int i = 5; i >= 1; i--) {
		float x = xs[i];
		float t = naca(x) * thickness * chord * 0.85f + 0.004f;
		r.push_back(Vector3(span_pos * side, y - t, le_z + x * chord));
	}
	return r;
}

void add_wing(MeshBuilder &b, const WingSpec &w, float part, bool both_sides) {
	const int steps = 9;
	for (int pass = 0; pass < (both_sides ? 2 : 1); pass++) {
		float side = pass == 0 ? 1.0f : -1.0f;
		std::vector<Ring> rings;
		for (int i = 0; i <= steps; i++) {
			float t = (float)i / (float)steps;
			float tt = std::sin(t * PI_F * 0.5f); // bunch stations toward the tip
			float x = lerpf(w.root_x, w.tip_x, tt);
			float chord;
			if (w.elliptical) {
				chord = w.root_chord * std::sqrt(MAX(1.0f - tt * tt * 0.985f, 0.0f));
				chord = MAX(chord, w.tip_chord * 0.35f);
			} else {
				chord = lerpf(w.root_chord, w.tip_chord, tt);
				// rounded tip
				if (tt > 0.93f) {
					float k = (tt - 0.93f) / 0.07f;
					chord *= std::sqrt(MAX(1.0f - k * k * 0.8f, 0.05f));
				}
			}
			float qc = w.qc_z + w.sweep * tt;
			float le = qc - chord * 0.3f;
			float y = w.y + (x - w.root_x) * std::tan(deg2rad(w.dihedral_deg));
			float thick = lerpf(w.t_root, w.t_tip, tt);
			rings.push_back(airfoil_ring(x, y, le, chord, thick, side));
		}
		b.set_color(Color(1, 1, 1, part));
		b.loft(rings, true, true);
	}
}

struct PlaneMeshes {
	Ref<ArrayMesh> body; // surface 0 painted, surface 1 detail
	Ref<ArrayMesh> canopy;
	Ref<ArrayMesh> prop_disc;
	Ref<ArrayMesh> prop_static;
	Ref<ArrayMesh> gear;
};

PlaneMeshes build_plane(PlaneType type) {
	const PlaneSpec &s = get_spec(type);
	PlaneMeshes out;

	MeshBuilder paint;
	{
		std::vector<Ring> rings;
		for (const Station &st : s.fuselage) {
			rings.push_back(station_ring(st, 20));
		}
		paint.set_color(Color(1, 1, 1, PART_FUSELAGE));
		paint.loft(rings, true, true);
	}
	add_wing(paint, s.wing, PART_WING, true);
	add_wing(paint, s.stab, PART_STAB, true);
	paint.set_transform(Transform3D(Basis(Vector3(0, 0, 1), PI_F * 0.5f), Vector3()));
	add_wing(paint, s.fin, PART_FIN, false);
	paint.reset_transform();

	// Engine nacelles (and the flying bomb's pulse jet), painted like the fuselage.
	for (const PlaneSpec::Nacelle &n : s.nacelles) {
		std::vector<Ring> rings;
		const int steps = 6;
		for (int i = 0; i <= steps; i++) {
			float t = (float)i / steps;
			float z = lerpf(n.z0, n.z1, t);
			float r = n.r * (i == 0 ? 0.82f : (t > 0.6f ? lerpf(1.0f, 0.55f, (t - 0.6f) / 0.4f) : 1.0f));
			rings.push_back(MeshBuilder::ellipse_ring(Vector3(n.x, n.y, z), Vector3(1, 0, 0), Vector3(0, 1, 0), r, r, 14));
		}
		paint.set_color(Color(1, 1, 1, PART_FUSELAGE));
		paint.loft(rings, true, true);
	}

	// Canopy framing, painted like the fuselage.
	if (s.has_canopy) {
		paint.set_color(Color(1, 1, 1, PART_FUSELAGE));
		for (size_t i = 1; i + 1 < s.canopy.size(); i++) {
			if (i == 3 && s.canopy.size() > 6) {
				continue; // keep the bubble clear
			}
			Station a = s.canopy[i];
			a.hw += 0.012f;
			a.hh += 0.012f;
			Station b0 = a;
			Station b1 = a;
			b0.z -= 0.035f;
			b1.z += 0.035f;
			paint.loft({ station_ring(b0, 14), station_ring(b1, 14) }, false, false);
		}
	}

	MeshBuilder detail;
	// Engine face / intake.
	float nz = s.nose_z;
	float front_r = s.fuselage[0].hw * 0.86f;
	if (s.radial && s.nose_prop) {
		detail.set_color(Color(0.05, 0.05, 0.055));
		detail.cylinder(Vector3(0, s.fuselage[0].y, nz - 0.01f), Vector3(0, s.fuselage[0].y, nz + 0.05f), front_r,
				front_r, 20);
		// Cylinder heads hinted as a ring of studs.
		detail.set_color(Color(0.22, 0.22, 0.23));
		for (int i = 0; i < 14; i++) {
			float a = TAU_F * i / 14.0f;
			Vector3 p(std::cos(a) * front_r * 0.68f, s.fuselage[0].y + std::sin(a) * front_r * 0.68f, nz - 0.03f);
			detail.cylinder(p, p + Vector3(0, 0, 0.06f), 0.09f, 0.09f, 6);
		}
	}
	// Spinner.
	detail.set_color(s.spinner_color);
	if (s.nose_prop) {
		std::vector<Ring> rings;
		const int n = 5;
		for (int i = 0; i <= n; i++) {
			float t = (float)i / n;
			float r = s.spinner_r * std::sqrt(MAX(1.0f - t * t, 0.0004f));
			rings.push_back(MeshBuilder::ellipse_ring(Vector3(0, s.fuselage[0].y, nz - t * s.spinner_len),
					Vector3(1, 0, 0), Vector3(0, 1, 0), r, r, 12));
		}
		detail.loft(rings, true, false);
	}
	// Nacelle spinners and engine faces.
	for (const PlaneSpec::Nacelle &n : s.nacelles) {
		if (!n.prop) {
			continue;
		}
		if (s.radial) {
			detail.set_color(Color(0.05, 0.05, 0.055));
			detail.cylinder(Vector3(n.x, n.y, n.z0 - 0.01f), Vector3(n.x, n.y, n.z0 + 0.05f), n.r * 0.8f, n.r * 0.8f, 14);
		}
		detail.set_color(s.spinner_color);
		std::vector<Ring> rings;
		const int k = 5;
		for (int i = 0; i <= k; i++) {
			float t = (float)i / k;
			float r = s.spinner_r * std::sqrt(MAX(1.0f - t * t, 0.0004f));
			rings.push_back(MeshBuilder::ellipse_ring(Vector3(n.x, n.y, n.z0 - t * s.spinner_len), Vector3(1, 0, 0),
					Vector3(0, 1, 0), r, r, 12));
		}
		detail.loft(rings, true, false);
	}
	// Rear gun position.
	if (s.turret) {
		detail.set_color(Color(0.07, 0.08, 0.1));
		detail.sphere(s.info.turret, Vector3(0.45f, 0.32f, 0.5f), 10, 6);
		detail.set_color(Color(0.08, 0.08, 0.09));
		detail.cylinder(s.info.turret, s.info.turret + Vector3(0, 0.25f, 1.1f), 0.04f, 0.03f, 6);
	}
	// Guns.
	detail.set_color(Color(0.08, 0.08, 0.09));
	if (!s.fighter_guns) {
		// Bombers and the flying bomb carry no fixed forward guns.
	} else if (type == PLANE_P47) {
		for (int side = -1; side <= 1; side += 2) {
			for (int g = 0; g < 4; g++) {
				float x = (2.55f + g * 0.24f) * side;
				float y = s.wing.y + (std::fabs(x) - s.wing.root_x) * std::tan(deg2rad(s.wing.dihedral_deg)) + 0.02f;
				float len = 0.75f - g * 0.12f;
				detail.cylinder(Vector3(x, y, -1.75f), Vector3(x, y, -1.95f - len), 0.035f, 0.03f, 6);
			}
		}
	} else {
		detail.cylinder(Vector3(-0.2f, s.fuselage[2].hh * 0.85f, -2.2f), Vector3(-0.2f, s.fuselage[1].hh * 0.9f, -3.3f),
				0.03f, 0.03f, 6);
		detail.cylinder(Vector3(0.2f, s.fuselage[2].hh * 0.85f, -2.2f), Vector3(0.2f, s.fuselage[1].hh * 0.9f, -3.3f),
				0.03f, 0.03f, 6);
		for (int side = -1; side <= 1; side += 2) {
			float x = 2.0f * side;
			float y = s.wing.y + (2.0f - s.wing.root_x) * std::tan(deg2rad(s.wing.dihedral_deg));
			detail.cylinder(Vector3(x, y, -0.9f), Vector3(x, y, -1.9f), 0.04f, 0.03f, 6);
		}
	}
	// Exhaust stubs for the inline engine.
	if (!s.radial) {
		detail.set_color(Color(0.12, 0.08, 0.06));
		for (int side = -1; side <= 1; side += 2) {
			for (int i = 0; i < 6; i++) {
				float z = -3.0f + i * 0.2f;
				detail.box(Vector3(0.45f * side, 0.12f, z), Vector3(0.08f, 0.07f, 0.12f));
			}
		}
	}
	// Pilot and headrest.
	if (s.has_pilot) {
		detail.set_color(Color(0.28, 0.17, 0.09));
		detail.sphere(s.pilot, Vector3(0.13f, 0.15f, 0.14f), 8, 6);
		detail.set_color(Color(0.25, 0.22, 0.15));
		detail.tapered_box(s.pilot + Vector3(0, -0.34f, 0.02f), Vector3(0.46f, 0.4f, 0.26f), 0.75f, 0.9f);
		detail.set_color(Color(0.07, 0.07, 0.07));
		detail.box(s.pilot + Vector3(0, -0.08f, 0.3f), Vector3(0.3f, 0.5f, 0.06f));
		// Gunsight.
		detail.box(s.pilot + Vector3(0, -0.06f, -0.8f), Vector3(0.08f, 0.12f, 0.08f));
	}
	// Tail wheel nub.
	detail.set_color(Color(0.05, 0.05, 0.05));
	detail.cylinder(Vector3(-0.05f, s.fuselage[s.fuselage.size() - 3].y - 0.45f, s.fuselage[s.fuselage.size() - 3].z),
			Vector3(0.05f, s.fuselage[s.fuselage.size() - 3].y - 0.45f, s.fuselage[s.fuselage.size() - 3].z), 0.13f,
			0.13f, 8);

	out.body.instantiate();
	paint.add_surface(out.body, Ref<Material>(), 55.0f);
	detail.add_surface(out.body, mats::vertex_color(0.6f, 0.2f), 50.0f);

	// Canopy glass.
	{
		MeshBuilder glass;
		std::vector<Ring> rings;
		for (const Station &st : s.canopy) {
			rings.push_back(station_ring(st, 14));
		}
		glass.loft(rings, false, false);
		if (!s.nose_glass.empty()) {
			std::vector<Ring> nose;
			for (const Station &st : s.nose_glass) {
				nose.push_back(station_ring(st, 16));
			}
			glass.loft(nose, true, false);
		}
		out.canopy = glass.build(mats::glass(), 60.0f);
	}

	// Spinning prop: a flat disc, the shader does the blur.
	{
		MeshBuilder disc;
		const int n = 32;
		for (int i = 0; i < n; i++) {
			float a0 = TAU_F * i / n;
			float a1 = TAU_F * (i + 1) / n;
			disc.tri(Vector3(), Vector3(std::cos(a0), std::sin(a0), 0), Vector3(std::cos(a1), std::sin(a1), 0));
		}
		out.prop_disc = disc.build(mats::prop_disc((float)s.blades, s.prop_tip), 50.0f);
	}
	// Stopped prop for parked aircraft.
	{
		MeshBuilder blades;
		blades.set_color(Color(0.06, 0.06, 0.06));
		for (int i = 0; i < s.blades && s.nose_prop; i++) {
			float a = TAU_F * i / s.blades + 0.4f;
			Transform3D t(Basis(Vector3(0, 0, 1), a), Vector3());
			blades.set_transform(t * Transform3D(Basis(Vector3(0, 1, 0), 0.5f), Vector3()));
			blades.tapered_box(Vector3(0, s.info.prop_radius * 0.5f, 0), Vector3(0.26f, s.info.prop_radius, 0.04f), 0.6f,
					1.0f);
		}
		out.prop_static = blades.build(mats::vertex_color(0.5f, 0.3f), 50.0f);
	}
	// Landing gear for parked aircraft.
	{
		MeshBuilder gear;
		for (int side = -1; side <= 1; side += 2) {
			float x = (type == PLANE_P47 ? 2.3f : 1.0f) * side;
			float top = s.wing.y;
			gear.set_color(Color(0.5, 0.5, 0.52));
			gear.cylinder(Vector3(x, top, -0.9f), Vector3(x, top - 1.35f, -1.2f), 0.07f, 0.07f, 8);
			gear.set_color(Color(0.04, 0.04, 0.04));
			gear.cylinder(Vector3(x - 0.12f, top - 1.45f, -1.22f), Vector3(x + 0.12f, top - 1.45f, -1.22f), 0.42f, 0.42f,
					12);
		}
		out.gear = gear.build(mats::vertex_color(0.7f, 0.2f), 50.0f);
	}
	return out;
}

struct ModelCache {
	std::map<int, PlaneMeshes> planes;
	std::map<std::string, Ref<ArrayMesh>> props;
	Ref<StandardMaterial3D> burnt;
};
ModelCache *cache = nullptr;

ModelCache &mc() {
	if (!cache) {
		cache = new ModelCache();
	}
	return *cache;
}

// ---------------------------------------------------------------------------
// Ground props
// ---------------------------------------------------------------------------

const Color C_TYRE(0.04, 0.04, 0.04);
const Color C_FELDGRAU(0.27, 0.30, 0.27);
const Color C_DUNKELGELB(0.52, 0.45, 0.27);
const Color C_CANVAS(0.33, 0.33, 0.24);
const Color C_STEEL(0.20, 0.21, 0.22);

void wheel(MeshBuilder &b, const Vector3 &c, float r, float w) {
	b.set_color(C_TYRE);
	b.cylinder(c - Vector3(w * 0.5f, 0, 0), c + Vector3(w * 0.5f, 0, 0), r, r, 10);
	b.set_color(Color(0.25, 0.25, 0.24));
	b.cylinder(c - Vector3(w * 0.52f, 0, 0), c + Vector3(w * 0.52f, 0, 0), r * 0.5f, r * 0.5f, 8);
}

void build_truck(MeshBuilder &b) {
	b.set_color(Color(0.12, 0.12, 0.12));
	b.box(Vector3(0, 0.7f, 0), Vector3(1.9f, 0.25f, 6.0f));
	b.set_color(C_FELDGRAU);
	b.tapered_box(Vector3(0, 1.25f, -2.45f), Vector3(1.7f, 0.85f, 1.5f), 0.85f, 0.9f); // hood
	b.box(Vector3(0, 1.6f, -1.2f), Vector3(2.1f, 1.6f, 1.4f)); // cab
	b.set_color(Color(0.08, 0.1, 0.12));
	b.box(Vector3(0, 1.95f, -1.92f), Vector3(1.8f, 0.6f, 0.04f)); // windscreen
	b.set_color(C_FELDGRAU * 0.9f);
	b.box(Vector3(0, 1.2f, 1.35f), Vector3(2.2f, 0.7f, 3.4f)); // bed
	b.set_color(C_CANVAS);
	b.tapered_box(Vector3(0, 2.2f, 1.35f), Vector3(2.2f, 1.35f, 3.4f), 0.72f, 0.98f); // tilt
	for (int side = -1; side <= 1; side += 2) {
		wheel(b, Vector3(1.0f * side, 0.48f, -2.2f), 0.48f, 0.3f);
		wheel(b, Vector3(1.0f * side, 0.48f, 1.7f), 0.48f, 0.45f);
		b.set_color(C_FELDGRAU);
		b.box(Vector3(1.0f * side, 1.0f, -2.2f), Vector3(0.4f, 0.1f, 1.2f)); // mudguard
	}
}

void build_staff_car(MeshBuilder &b) {
	b.set_color(C_DUNKELGELB);
	b.box(Vector3(0, 0.75f, 0), Vector3(1.7f, 0.6f, 3.8f));
	b.tapered_box(Vector3(0, 1.1f, -1.2f), Vector3(1.4f, 0.3f, 1.3f), 0.85f, 0.95f);
	b.set_color(C_CANVAS);
	b.tapered_box(Vector3(0, 1.4f, 0.55f), Vector3(1.6f, 0.7f, 2.0f), 0.85f, 0.8f);
	for (int side = -1; side <= 1; side += 2) {
		wheel(b, Vector3(0.85f * side, 0.38f, -1.25f), 0.38f, 0.22f);
		wheel(b, Vector3(0.85f * side, 0.38f, 1.25f), 0.38f, 0.22f);
	}
	wheel(b, Vector3(0, 1.0f, 2.0f), 0.36f, 0.2f);
}

void tracks(MeshBuilder &b, float x, float len, float h, float w) {
	for (int side = -1; side <= 1; side += 2) {
		b.set_color(Color(0.09, 0.09, 0.09));
		std::vector<Ring> rings;
		for (int k = 0; k < 2; k++) {
			float xx = x * side + (k == 0 ? -w * 0.5f : w * 0.5f);
			Ring r = {
				Vector3(xx, h, -len * 0.5f),
				Vector3(xx, h, len * 0.5f),
				Vector3(xx, h * 0.45f, len * 0.5f + 0.25f),
				Vector3(xx, 0.0f, len * 0.5f - 0.5f),
				Vector3(xx, 0.0f, -len * 0.5f + 0.5f),
				Vector3(xx, h * 0.45f, -len * 0.5f - 0.25f),
			};
			rings.push_back(r);
		}
		b.loft(rings, true, true);
		b.set_color(Color(0.2, 0.2, 0.19));
		int n = (int)(len / 0.85f);
		for (int i = 0; i < n; i++) {
			float z = -len * 0.5f + 0.6f + i * ((len - 1.2f) / MAX(n - 1, 1));
			float xo = x * side + w * 0.52f * side;
			b.cylinder(Vector3(xo - 0.03f, h * 0.38f, z), Vector3(xo + 0.03f, h * 0.38f, z), h * 0.36f, h * 0.36f, 8);
		}
	}
}

void build_tank(MeshBuilder &b) {
	tracks(b, 1.25f, 5.6f, 0.95f, 0.5f);
	b.set_color(C_DUNKELGELB);
	b.tapered_box(Vector3(0, 1.0f, 0), Vector3(2.1f, 0.9f, 5.7f), 1.0f, 0.92f); // hull
	b.box(Vector3(0, 1.5f, 0.1f), Vector3(2.9f, 0.12f, 5.2f)); // fenders
	b.set_color(C_DUNKELGELB * 0.92f);
	b.tapered_box(Vector3(0, 1.75f, -0.3f), Vector3(2.0f, 0.5f, 3.0f), 0.95f, 0.9f); // superstructure
	b.set_color(C_DUNKELGELB);
	b.tapered_box(Vector3(0, 2.3f, -0.2f), Vector3(1.7f, 0.65f, 2.2f), 0.8f, 0.8f); // turret
	b.cylinder(Vector3(0, 2.62f, 0.3f), Vector3(0, 2.85f, 0.3f), 0.32f, 0.3f, 10); // cupola
	b.set_color(C_STEEL);
	b.cylinder(Vector3(0, 2.3f, -1.2f), Vector3(0, 2.36f, -4.4f), 0.09f, 0.07f, 8); // gun
	b.cylinder(Vector3(0, 2.36f, -4.4f), Vector3(0, 2.365f, -4.75f), 0.12f, 0.12f, 8); // muzzle brake
	b.set_color(Color(0.18, 0.22, 0.12));
	b.box(Vector3(0.6f, 2.0f, 1.9f), Vector3(0.8f, 0.4f, 0.6f)); // stowage
}

void build_halftrack(MeshBuilder &b) {
	tracks(b, 0.95f, 3.0f, 0.75f, 0.35f);
	b.set_color(C_DUNKELGELB);
	b.set_transform(Transform3D(Basis(), Vector3(0, 0, 0.6f)));
	b.tapered_box(Vector3(0, 1.3f, 0.3f), Vector3(2.0f, 1.1f, 3.6f), 0.8f, 1.0f);
	b.tapered_box(Vector3(0, 1.15f, -2.3f), Vector3(1.8f, 0.8f, 1.8f), 0.75f, 0.8f);
	b.set_color(Color(0.05, 0.05, 0.05));
	b.box(Vector3(0, 1.84f, 0.3f), Vector3(1.5f, 0.04f, 3.3f));
	b.reset_transform();
	for (int side = -1; side <= 1; side += 2) {
		wheel(b, Vector3(0.95f * side, 0.45f, -2.0f), 0.45f, 0.28f);
	}
	b.set_color(C_STEEL);
	b.cylinder(Vector3(0, 2.0f, -0.3f), Vector3(0, 2.4f, -1.4f), 0.04f, 0.03f, 6);
}

void build_sandbags(MeshBuilder &b, float radius) {
	const int n = 14;
	for (int layer = 0; layer < 3; layer++) {
		for (int i = 0; i < n; i++) {
			float a = TAU_F * (i + (layer % 2) * 0.5f) / n;
			float tone = 0.9f + 0.2f * (float)((i * 7 + layer * 3) % 5) / 5.0f;
			b.set_color(Color(0.48f * tone, 0.42f * tone, 0.30f * tone));
			Transform3D t(Basis(Vector3(0, 1, 0), -a), Vector3(std::cos(a) * radius, 0.2f + layer * 0.34f, std::sin(a) * radius));
			b.set_transform(t);
			b.sphere(Vector3(), Vector3(0.3f, 0.2f, 0.62f), 6, 4);
		}
	}
	b.reset_transform();
}

void build_flak(MeshBuilder &b) {
	build_sandbags(b, 3.0f);
	b.set_color(C_STEEL);
	b.cylinder(Vector3(0, 0, 0), Vector3(0, 0.35f, 0), 1.3f, 1.1f, 12);
	b.set_color(C_FELDGRAU);
	b.tapered_box(Vector3(0, 0.95f, 0.2f), Vector3(1.3f, 1.1f, 1.5f), 0.8f, 0.7f);
	// gun shield
	b.box(Vector3(0, 1.5f, -0.55f), Vector3(1.9f, 1.2f, 0.06f));
	// four barrels raised skyward
	b.set_color(Color(0.1, 0.1, 0.11));
	Transform3D t(Basis(Vector3(1, 0, 0), deg2rad(55.0f)), Vector3(0, 1.5f, -0.2f));
	b.set_transform(t);
	for (int i = 0; i < 4; i++) {
		float x = (i % 2 == 0 ? -0.22f : 0.22f);
		float y = (i / 2 == 0 ? -0.12f : 0.12f);
		b.cylinder(Vector3(x, y, 0.3f), Vector3(x, y, -2.6f), 0.045f, 0.035f, 6);
		b.cylinder(Vector3(x, y, -2.6f), Vector3(x, y, -2.85f), 0.06f, 0.07f, 6);
	}
	b.reset_transform();
}

void windows_row(MeshBuilder &b, float x, float y, float z0, float z1, int count, const Vector3 &size) {
	b.set_color(Color(0.07, 0.08, 0.1));
	for (int i = 0; i < count; i++) {
		float z = count == 1 ? (z0 + z1) * 0.5f : lerpf(z0, z1, (float)i / (count - 1));
		b.box(Vector3(x, y, z), size);
	}
}

void build_house(MeshBuilder &b) {
	b.set_color(Color(0.72, 0.68, 0.58));
	b.box(Vector3(0, 2.6f, 0), Vector3(7.5f, 5.2f, 11.0f));
	b.set_color(Color(0.36, 0.20, 0.14));
	b.gable_roof(Vector3(0, 6.7f, 0), Vector3(8.3f, 3.0f, 11.8f));
	b.set_color(Color(0.45, 0.30, 0.25));
	b.box(Vector3(1.6f, 7.4f, 3.2f), Vector3(0.8f, 2.4f, 0.8f));
	for (int side = -1; side <= 1; side += 2) {
		windows_row(b, 3.76f * side, 3.7f, -3.6f, 3.6f, 3, Vector3(0.06f, 1.3f, 0.9f));
		windows_row(b, 3.76f * side, 1.4f, -3.6f, 3.6f, 3, Vector3(0.06f, 1.3f, 0.9f));
	}
	b.set_color(Color(0.22, 0.14, 0.08));
	b.box(Vector3(0, 1.1f, -5.52f), Vector3(1.2f, 2.2f, 0.06f));
}

void build_barn(MeshBuilder &b) {
	b.set_color(Color(0.42, 0.30, 0.20));
	b.box(Vector3(0, 2.2f, 0), Vector3(9.0f, 4.4f, 16.0f));
	b.set_color(Color(0.25, 0.22, 0.20));
	b.gable_roof(Vector3(0, 6.2f, 0), Vector3(9.8f, 3.6f, 16.8f));
	b.set_color(Color(0.18, 0.12, 0.08));
	b.box(Vector3(0, 1.7f, -8.02f), Vector3(3.6f, 3.4f, 0.06f));
}

void build_warehouse(MeshBuilder &b) {
	b.set_color(Color(0.50, 0.33, 0.27));
	b.box(Vector3(0, 3.5f, 0), Vector3(16.0f, 7.0f, 36.0f));
	b.set_color(Color(0.24, 0.25, 0.27));
	b.gable_roof(Vector3(0, 8.6f, 0), Vector3(17.0f, 3.2f, 37.0f));
	for (int side = -1; side <= 1; side += 2) {
		windows_row(b, 8.02f * side, 5.0f, -15.0f, 15.0f, 8, Vector3(0.06f, 1.6f, 2.2f));
		b.set_color(Color(0.2, 0.18, 0.15));
		for (int i = 0; i < 3; i++) {
			b.box(Vector3(8.03f * side, 1.8f, -11.0f + i * 11.0f), Vector3(0.06f, 3.6f, 4.0f));
		}
	}
	// roof vents
	b.set_color(Color(0.3, 0.3, 0.3));
	for (int i = 0; i < 4; i++) {
		b.box(Vector3(0, 10.4f, -13.5f + i * 9.0f), Vector3(1.6f, 0.8f, 3.0f));
	}
}

void build_factory(MeshBuilder &b) {
	b.set_color(Color(0.47, 0.30, 0.25));
	b.box(Vector3(0, 4.0f, 0), Vector3(30.0f, 8.0f, 44.0f));
	// saw-tooth roof
	for (int i = 0; i < 5; i++) {
		float z = -17.6f + i * 8.8f;
		b.set_color(Color(0.26, 0.27, 0.29));
		Vector3 a(-15.0f, 8.0f, z - 4.4f), bb(15.0f, 8.0f, z - 4.4f), c(15.0f, 8.0f, z + 4.4f), d(-15.0f, 8.0f, z + 4.4f);
		Vector3 r0(-15.0f, 11.0f, z - 4.4f), r1(15.0f, 11.0f, z - 4.4f);
		b.quad(r0, d, c, r1); // long slope
		b.set_color(Color(0.55, 0.68, 0.72));
		b.quad(a, r0, r1, bb); // glazing
		b.set_color(Color(0.47, 0.30, 0.25));
		b.tri(a, d, r0);
		b.tri(bb, r1, c);
	}
	// chimney stacks
	b.set_color(Color(0.42, 0.26, 0.22));
	b.cylinder(Vector3(-10.0f, 0, 26.0f), Vector3(-10.0f, 30.0f, 26.0f), 1.9f, 1.2f, 12);
	b.cylinder(Vector3(8.0f, 0, 27.0f), Vector3(8.0f, 24.0f, 27.0f), 1.6f, 1.0f, 12);
	b.set_color(Color(0.07, 0.07, 0.07));
	b.cylinder(Vector3(-10.0f, 30.0f, 26.0f), Vector3(-10.0f, 30.4f, 26.0f), 1.3f, 1.3f, 12);
	for (int side = -1; side <= 1; side += 2) {
		windows_row(b, 15.03f * side, 5.2f, -19.0f, 19.0f, 9, Vector3(0.06f, 2.6f, 2.6f));
	}
}

void build_church(MeshBuilder &b) {
	b.set_color(Color(0.62, 0.60, 0.55));
	b.box(Vector3(0, 4.0f, 3.0f), Vector3(9.0f, 8.0f, 20.0f));
	b.set_color(Color(0.20, 0.22, 0.26));
	b.gable_roof(Vector3(0, 10.0f, 3.0f), Vector3(9.8f, 4.0f, 20.6f));
	b.set_color(Color(0.60, 0.58, 0.53));
	b.box(Vector3(0, 9.0f, -9.0f), Vector3(5.5f, 18.0f, 5.5f));
	b.set_color(Color(0.20, 0.22, 0.26));
	b.tapered_box(Vector3(0, 24.0f, -9.0f), Vector3(5.9f, 12.0f, 5.9f), 0.04f, 0.04f);
	windows_row(b, 4.52f, 5.0f, -2.0f, 10.0f, 4, Vector3(0.06f, 3.4f, 1.1f));
	windows_row(b, -4.52f, 5.0f, -2.0f, 10.0f, 4, Vector3(0.06f, 3.4f, 1.1f));
}

void build_hangar(MeshBuilder &b) {
	std::vector<Ring> rings;
	for (int k = 0; k < 2; k++) {
		float z = k == 0 ? -20.0f : 20.0f;
		Ring r;
		const int n = 12;
		for (int i = 0; i <= n; i++) {
			float a = PI_F * i / n;
			r.push_back(Vector3(std::cos(a) * 17.0f, std::sin(a) * 9.5f, z));
		}
		rings.push_back(r);
	}
	b.set_color(Color(0.30, 0.34, 0.27));
	b.loft(rings, true, true);
	b.set_color(Color(0.05, 0.05, 0.06));
	b.box(Vector3(0, 3.4f, -20.05f), Vector3(24.0f, 6.8f, 0.1f));
	// camouflage bands
	b.set_color(Color(0.17, 0.22, 0.14));
	for (int i = 0; i < 4; i++) {
		std::vector<Ring> band;
		for (int k = 0; k < 2; k++) {
			float z = -15.0f + i * 10.0f + k * 3.5f;
			Ring r;
			const int n = 12;
			for (int j = 0; j <= n; j++) {
				float a = PI_F * j / n;
				r.push_back(Vector3(std::cos(a) * 17.06f, std::sin(a) * 9.56f, z + std::sin(a * 3.0f) * 1.5f));
			}
			band.push_back(r);
		}
		b.loft(band, false, false);
	}
}

void build_tower(MeshBuilder &b) {
	b.set_color(Color(0.55, 0.53, 0.48));
	b.box(Vector3(0, 4.0f, 0), Vector3(6.0f, 8.0f, 6.0f));
	b.set_color(Color(0.10, 0.14, 0.17));
	b.box(Vector3(0, 9.2f, 0), Vector3(5.4f, 2.0f, 5.4f));
	b.set_color(Color(0.3, 0.3, 0.3));
	b.box(Vector3(0, 10.4f, 0), Vector3(6.6f, 0.4f, 6.6f));
	b.cylinder(Vector3(1.5f, 10.6f, 1.5f), Vector3(1.5f, 15.0f, 1.5f), 0.06f, 0.04f, 6);
}

void build_fuel_tank(MeshBuilder &b) {
	b.set_color(Color(0.52, 0.54, 0.55));
	b.cylinder(Vector3(0, 0, 0), Vector3(0, 7.0f, 0), 5.5f, 5.5f, 20);
	b.set_color(Color(0.42, 0.44, 0.45));
	b.cylinder(Vector3(0, 7.0f, 0), Vector3(0, 8.0f, 0), 5.5f, 0.6f, 20);
	b.set_color(Color(0.30, 0.22, 0.16));
	b.cylinder(Vector3(0, 3.4f, 0), Vector3(0, 3.8f, 0), 5.56f, 5.56f, 20, false);
	b.set_color(Color(0.2, 0.2, 0.2));
	b.box(Vector3(5.7f, 3.5f, 0), Vector3(0.3f, 7.0f, 0.9f));
}

void rail_wheels(MeshBuilder &b, float z0, float z1, int count, float r) {
	for (int side = -1; side <= 1; side += 2) {
		for (int i = 0; i < count; i++) {
			float z = count == 1 ? (z0 + z1) * 0.5f : lerpf(z0, z1, (float)i / (count - 1));
			b.set_color(Color(0.07, 0.07, 0.07));
			b.cylinder(Vector3(0.62f * side, r, z), Vector3(0.82f * side, r, z), r, r, 10);
		}
	}
}

void build_loco(MeshBuilder &b) {
	rail_wheels(b, -3.4f, 2.8f, 5, 0.75f);
	b.set_color(Color(0.10, 0.10, 0.11));
	b.box(Vector3(0, 1.3f, 0), Vector3(2.4f, 0.5f, 11.0f));
	b.cylinder(Vector3(0, 2.7f, -5.2f), Vector3(0, 2.7f, 2.0f), 1.25f, 1.25f, 14);
	b.set_color(Color(0.13, 0.13, 0.14));
	b.box(Vector3(0, 3.0f, 3.6f), Vector3(2.8f, 3.2f, 3.2f)); // cab
	b.set_color(Color(0.08, 0.08, 0.08));
	b.box(Vector3(0, 4.7f, 3.6f), Vector3(3.0f, 0.2f, 3.6f));
	b.cylinder(Vector3(0, 3.8f, -4.3f), Vector3(0, 4.9f, -4.3f), 0.35f, 0.42f, 10); // funnel
	b.cylinder(Vector3(0, 3.8f, -1.5f), Vector3(0, 4.4f, -1.5f), 0.5f, 0.4f, 10); // dome
	b.set_color(Color(0.45, 0.08, 0.06));
	b.box(Vector3(0, 1.0f, -5.6f), Vector3(2.6f, 0.5f, 0.3f)); // buffer beam
	b.set_color(Color(0.07, 0.09, 0.11));
	windows_row(b, 1.42f, 3.6f, 3.0f, 4.2f, 2, Vector3(0.05f, 0.8f, 0.8f));
	windows_row(b, -1.42f, 3.6f, 3.0f, 4.2f, 2, Vector3(0.05f, 0.8f, 0.8f));
}

void build_boxcar(MeshBuilder &b) {
	rail_wheels(b, -3.2f, 3.2f, 2, 0.5f);
	b.set_color(Color(0.12, 0.12, 0.12));
	b.box(Vector3(0, 1.0f, 0), Vector3(2.5f, 0.3f, 9.4f));
	b.set_color(Color(0.36, 0.20, 0.14));
	b.box(Vector3(0, 2.5f, 0), Vector3(2.8f, 2.7f, 9.0f));
	b.set_color(Color(0.28, 0.27, 0.26));
	b.tapered_box(Vector3(0, 4.05f, 0), Vector3(3.0f, 0.4f, 9.2f), 0.6f, 1.0f);
	b.set_color(Color(0.28, 0.15, 0.10));
	b.box(Vector3(1.42f, 2.4f, 0), Vector3(0.06f, 2.3f, 2.2f));
	b.box(Vector3(-1.42f, 2.4f, 0), Vector3(0.06f, 2.3f, 2.2f));
}

void build_tanker(MeshBuilder &b) {
	rail_wheels(b, -3.2f, 3.2f, 2, 0.5f);
	b.set_color(Color(0.12, 0.12, 0.12));
	b.box(Vector3(0, 1.0f, 0), Vector3(2.5f, 0.3f, 9.4f));
	b.set_color(Color(0.16, 0.17, 0.18));
	b.cylinder(Vector3(0, 2.5f, -4.2f), Vector3(0, 2.5f, 4.2f), 1.35f, 1.35f, 14);
	b.set_color(Color(0.3, 0.3, 0.3));
	b.cylinder(Vector3(0, 3.8f, 0), Vector3(0, 4.3f, 0), 0.45f, 0.45f, 10);
}

void build_flatcar(MeshBuilder &b) {
	rail_wheels(b, -3.2f, 3.2f, 2, 0.5f);
	b.set_color(Color(0.22, 0.17, 0.12));
	b.box(Vector3(0, 1.05f, 0), Vector3(2.7f, 0.35f, 9.4f));
	b.set_color(Color(0.3, 0.32, 0.22));
	b.tapered_box(Vector3(0, 1.9f, -1.5f), Vector3(2.2f, 1.4f, 3.4f), 0.8f, 0.9f);
	b.set_color(Color(0.4, 0.3, 0.18));
	b.box(Vector3(0, 1.7f, 2.6f), Vector3(2.0f, 1.0f, 2.4f));
}

void build_tree(MeshBuilder &b) {
	b.set_color(Color(0.22, 0.15, 0.09));
	b.cylinder(Vector3(0, 0, 0), Vector3(0, 4.0f, 0), 0.35f, 0.22f, 6, false);
	b.set_color(Color(0.13, 0.27, 0.08));
	b.sphere(Vector3(0, 5.6f, 0), Vector3(3.0f, 2.6f, 3.0f), 7, 5);
	b.set_color(Color(0.17, 0.33, 0.10));
	b.sphere(Vector3(1.2f, 6.8f, 0.6f), Vector3(2.1f, 1.9f, 2.1f), 7, 5);
	b.set_color(Color(0.10, 0.23, 0.07));
	b.sphere(Vector3(-1.3f, 5.0f, -0.8f), Vector3(2.0f, 1.7f, 2.0f), 7, 5);
}

void build_poplar(MeshBuilder &b) {
	b.set_color(Color(0.22, 0.15, 0.09));
	b.cylinder(Vector3(0, 0, 0), Vector3(0, 3.0f, 0), 0.3f, 0.2f, 6, false);
	b.set_color(Color(0.14, 0.28, 0.09));
	b.sphere(Vector3(0, 8.5f, 0), Vector3(1.6f, 6.5f, 1.6f), 7, 6);
}

void build_pine(MeshBuilder &b) {
	b.set_color(Color(0.20, 0.13, 0.08));
	b.cylinder(Vector3(0, 0, 0), Vector3(0, 3.0f, 0), 0.3f, 0.2f, 6, false);
	b.set_color(Color(0.07, 0.19, 0.10));
	b.cylinder(Vector3(0, 2.0f, 0), Vector3(0, 6.5f, 0), 2.6f, 0.6f, 7, true);
	b.set_color(Color(0.09, 0.22, 0.11));
	b.cylinder(Vector3(0, 5.0f, 0), Vector3(0, 9.0f, 0), 2.0f, 0.4f, 7, true);
	b.cylinder(Vector3(0, 8.0f, 0), Vector3(0, 11.5f, 0), 1.3f, 0.0f, 7, true);
}

void build_bush(MeshBuilder &b) {
	b.set_color(Color(0.11, 0.24, 0.07));
	b.sphere(Vector3(0, 0.9f, 0), Vector3(1.8f, 1.3f, 1.5f), 6, 4);
	b.set_color(Color(0.14, 0.29, 0.09));
	b.sphere(Vector3(1.0f, 0.8f, 0.5f), Vector3(1.2f, 1.0f, 1.2f), 6, 4);
}

void build_soldier(MeshBuilder &b) {
	b.set_color(C_FELDGRAU);
	b.tapered_box(Vector3(0, 1.1f, 0), Vector3(0.5f, 0.7f, 0.3f), 0.9f, 0.9f);
	b.set_color(Color(0.22, 0.24, 0.22));
	b.box(Vector3(-0.13f, 0.4f, 0), Vector3(0.2f, 0.8f, 0.24f));
	b.box(Vector3(0.13f, 0.4f, 0), Vector3(0.2f, 0.8f, 0.24f));
	b.set_color(Color(0.75, 0.58, 0.46));
	b.sphere(Vector3(0, 1.62f, 0), Vector3(0.13f, 0.15f, 0.13f), 6, 4);
	b.set_color(Color(0.2, 0.23, 0.2));
	b.sphere(Vector3(0, 1.7f, 0), Vector3(0.17f, 0.11f, 0.18f), 6, 3);
	b.set_color(Color(0.1, 0.08, 0.06));
	b.box(Vector3(0.28f, 1.2f, -0.2f), Vector3(0.05f, 0.05f, 1.0f));
}

void build_bomb(MeshBuilder &b) {
	b.set_color(Color(0.20, 0.23, 0.14));
	b.sphere(Vector3(), Vector3(0.23f, 0.23f, 0.85f), 10, 8);
	b.set_color(Color(0.85, 0.72, 0.1));
	b.cylinder(Vector3(0, 0, -0.55f), Vector3(0, 0, -0.62f), 0.19f, 0.17f, 10, false);
	b.set_color(Color(0.16, 0.18, 0.11));
	b.box(Vector3(0, 0, 0.85f), Vector3(0.5f, 0.03f, 0.4f));
	b.box(Vector3(0, 0, 0.85f), Vector3(0.03f, 0.5f, 0.4f));
}

void build_bridge(MeshBuilder &b) {
	// Spans along Z, 120 m long, deck at y = 0.
	b.set_color(Color(0.30, 0.30, 0.31));
	b.box(Vector3(0, -0.6f, 0), Vector3(11.0f, 1.2f, 120.0f));
	b.set_color(Color(0.50, 0.48, 0.44));
	for (int i = -1; i <= 1; i++) {
		b.tapered_box(Vector3(0, -8.0f, i * 40.0f), Vector3(13.0f, 14.0f, 6.0f), 0.85f, 0.7f);
	}
	b.set_color(Color(0.22, 0.24, 0.25));
	for (int side = -1; side <= 1; side += 2) {
		float x = 5.6f * side;
		for (int span = 0; span < 3; span++) {
			float z0 = -60.0f + span * 40.0f;
			const int n = 8;
			Vector3 prev_top;
			for (int i = 0; i <= n; i++) {
				float t = (float)i / n;
				float z = z0 + t * 40.0f;
				float h = std::sin(t * PI_F) * 8.0f + 0.4f;
				Vector3 top(x, h, z);
				b.cylinder(Vector3(x, 0, z), top, 0.18f, 0.18f, 5, false);
				if (i > 0) {
					b.cylinder(prev_top, top, 0.28f, 0.28f, 5, false);
					b.cylinder(prev_top, Vector3(x, 0, z), 0.12f, 0.12f, 5, false);
				}
				prev_top = top;
			}
		}
	}
}

void build_tent(MeshBuilder &b) {
	b.set_color(Color(0.36, 0.37, 0.25));
	b.box(Vector3(0, 0.7f, 0), Vector3(4.4f, 1.4f, 6.0f));
	b.set_color(Color(0.31, 0.33, 0.22));
	b.gable_roof(Vector3(0, 2.2f, 0), Vector3(4.8f, 1.6f, 6.3f));
}

void build_crates(MeshBuilder &b) {
	Rng rng(991);
	for (int i = 0; i < 9; i++) {
		float tone = rng.range(0.8f, 1.1f);
		b.set_color(Color(0.42f * tone, 0.32f * tone, 0.18f * tone));
		int layer = i / 4;
		b.box(Vector3((i % 2) * 1.3f - 0.65f + rng.range(-0.1f, 0.1f), 0.5f + layer * 1.0f,
					  ((i / 2) % 2) * 1.3f - 0.65f + rng.range(-0.1f, 0.1f)),
				Vector3(1.15f, 1.0f, 1.15f));
	}
	b.set_color(Color(0.2, 0.25, 0.15));
	for (int i = 0; i < 5; i++) {
		b.cylinder(Vector3(2.0f + (i % 3) * 0.75f, 0, -0.6f + (i / 3) * 0.8f),
				Vector3(2.0f + (i % 3) * 0.75f, 1.0f, -0.6f + (i / 3) * 0.8f), 0.33f, 0.33f, 8);
	}
}


void build_flak_wagon(MeshBuilder &b) {
	rail_wheels(b, -3.2f, 3.2f, 2, 0.5f);
	b.set_color(Color(0.22, 0.17, 0.12));
	b.box(Vector3(0, 1.05f, 0), Vector3(2.7f, 0.35f, 9.4f));
	// Low armoured sides and a gun on a pedestal.
	b.set_color(C_FELDGRAU);
	b.box(Vector3(1.25f, 1.6f, 0), Vector3(0.12f, 0.8f, 8.6f));
	b.box(Vector3(-1.25f, 1.6f, 0), Vector3(0.12f, 0.8f, 8.6f));
	b.set_color(C_STEEL);
	b.cylinder(Vector3(0, 1.2f, 0), Vector3(0, 1.7f, 0), 0.9f, 0.8f, 10);
	b.set_color(C_FELDGRAU);
	b.tapered_box(Vector3(0, 2.1f, 0.1f), Vector3(1.2f, 0.9f, 1.3f), 0.8f, 0.7f);
	b.set_color(Color(0.1, 0.1, 0.11));
	Transform3D t(Basis(Vector3(1, 0, 0), deg2rad(50.0f)), Vector3(0, 2.5f, -0.1f));
	b.set_transform(t);
	for (int i = 0; i < 4; i++) {
		float x = (i % 2 == 0 ? -0.2f : 0.2f);
		float y = (i / 2 == 0 ? -0.1f : 0.1f);
		b.cylinder(Vector3(x, y, 0.2f), Vector3(x, y, -2.4f), 0.045f, 0.035f, 6);
	}
	b.reset_transform();
	b.set_color(C_CANVAS);
	b.box(Vector3(0, 1.45f, 3.6f), Vector3(2.2f, 0.5f, 1.4f)); // ammunition boxes
}

void build_barge(MeshBuilder &b) {
	// Rhine-style barge, 26 m, hull sitting in the water.
	std::vector<Ring> rings;
	const float zs[] = { -13.0f, -11.0f, -5.0f, 5.0f, 11.0f, 13.0f };
	const float ws[] = { 1.2f, 2.8f, 3.1f, 3.1f, 2.8f, 1.4f };
	for (int i = 0; i < 6; i++) {
		Ring r = {
			Vector3(-ws[i], 1.3f, zs[i]),
			Vector3(ws[i], 1.3f, zs[i]),
			Vector3(ws[i] * 0.85f, -0.6f, zs[i]),
			Vector3(-ws[i] * 0.85f, -0.6f, zs[i]),
		};
		rings.push_back(r);
	}
	b.set_color(Color(0.12, 0.12, 0.13));
	b.loft(rings, true, true);
	b.set_color(Color(0.35, 0.28, 0.18));
	b.box(Vector3(0, 1.32f, -1.0f), Vector3(5.6f, 0.1f, 20.0f)); // deck
	b.set_color(Color(0.30, 0.32, 0.24));
	b.tapered_box(Vector3(0, 1.9f, -2.0f), Vector3(4.8f, 1.1f, 15.0f), 0.85f, 0.95f); // hatch covers
	b.set_color(Color(0.55, 0.52, 0.45));
	b.box(Vector3(0, 2.4f, 9.5f), Vector3(3.6f, 2.2f, 3.4f)); // wheelhouse
	b.set_color(Color(0.08, 0.1, 0.12));
	b.box(Vector3(0, 2.8f, 7.78f), Vector3(2.8f, 0.7f, 0.05f));
	b.set_color(Color(0.1, 0.1, 0.1));
	b.cylinder(Vector3(0.8f, 3.5f, 10.5f), Vector3(0.8f, 5.0f, 10.5f), 0.18f, 0.16f, 8); // funnel
}

void build_landing_craft(MeshBuilder &b) {
	b.set_color(Color(0.36, 0.38, 0.36));
	b.tapered_box(Vector3(0, 0.6f, 0), Vector3(3.2f, 1.6f, 11.0f), 1.0f, 1.0f);
	b.set_color(Color(0.3, 0.32, 0.3));
	b.box(Vector3(1.5f, 1.5f, 0.5f), Vector3(0.15f, 0.9f, 9.0f));
	b.box(Vector3(-1.5f, 1.5f, 0.5f), Vector3(0.15f, 0.9f, 9.0f));
	b.tapered_box(Vector3(0, 1.4f, -5.0f), Vector3(3.2f, 1.2f, 0.4f), 1.0f, 1.0f); // ramp
	b.set_color(Color(0.18, 0.2, 0.18));
	b.box(Vector3(0, 1.8f, 4.5f), Vector3(1.6f, 1.2f, 1.6f)); // coxswain's box
}

void build_bunker(MeshBuilder &b) {
	b.set_color(Color(0.52, 0.52, 0.48));
	b.tapered_box(Vector3(0, 1.9f, 0), Vector3(11.0f, 3.8f, 8.0f), 0.85f, 0.8f);
	b.set_color(Color(0.42, 0.42, 0.40));
	b.tapered_box(Vector3(0, 4.1f, 0), Vector3(9.4f, 0.6f, 6.4f), 0.8f, 0.8f);
	b.set_color(Color(0.05, 0.05, 0.05));
	b.box(Vector3(0, 2.4f, -4.02f), Vector3(5.5f, 0.5f, 0.1f)); // embrasure
	b.set_color(Color(0.3, 0.25, 0.18));
	b.tapered_box(Vector3(0, 0.6f, 4.8f), Vector3(12.0f, 1.2f, 3.0f), 0.7f, 0.5f); // earth bank
}

void build_coastal_gun(MeshBuilder &b) {
	b.set_color(Color(0.5, 0.5, 0.47));
	b.cylinder(Vector3(0, -0.2f, 0), Vector3(0, 1.4f, 0), 5.2f, 5.2f, 18, false); // pit wall
	b.set_color(Color(0.3, 0.27, 0.22));
	b.cylinder(Vector3(0, -0.2f, 0), Vector3(0, 0.05f, 0), 5.0f, 5.0f, 18); // floor
	b.set_color(C_STEEL);
	b.cylinder(Vector3(0, 0, 0), Vector3(0, 0.8f, 0), 1.6f, 1.4f, 12);
	b.set_color(C_FELDGRAU);
	b.tapered_box(Vector3(0, 1.7f, 0.4f), Vector3(2.6f, 1.6f, 3.0f), 0.8f, 0.7f); // mount / shield
	b.set_color(Color(0.1, 0.1, 0.11));
	Transform3D t(Basis(Vector3(1, 0, 0), deg2rad(25.0f)), Vector3(0, 2.2f, -0.5f));
	b.set_transform(t);
	b.cylinder(Vector3(0, 0, 0.5f), Vector3(0, 0, -6.5f), 0.22f, 0.16f, 10);
	b.reset_transform();
	build_sandbags(b, 6.0f);
}

void build_mg_nest(MeshBuilder &b) {
	// A small ring of sandbags with a machine gun.
	const int n = 9;
	for (int layer = 0; layer < 2; layer++) {
		for (int i = 0; i < n; i++) {
			float a = TAU_F * (i + (layer % 2) * 0.5f) / n;
			float tone = 0.9f + 0.2f * (float)((i * 5 + layer) % 4) / 4.0f;
			b.set_color(Color(0.48f * tone, 0.42f * tone, 0.30f * tone));
			b.set_transform(Transform3D(Basis(Vector3(0, 1, 0), -a), Vector3(std::cos(a) * 1.7f, 0.2f + layer * 0.34f, std::sin(a) * 1.7f)));
			b.sphere(Vector3(), Vector3(0.3f, 0.2f, 0.55f), 6, 4);
		}
	}
	b.reset_transform();
	b.set_color(Color(0.1, 0.1, 0.11));
	b.cylinder(Vector3(0, 0.9f, 0.2f), Vector3(0, 1.0f, -1.6f), 0.05f, 0.04f, 6);
	b.set_color(C_FELDGRAU);
	b.tapered_box(Vector3(0, 0.6f, 0.4f), Vector3(0.4f, 0.5f, 0.6f), 0.8f, 0.8f);
}

void build_chateau(MeshBuilder &b) {
	// Two storeys, mansard roof, a round tower at each front corner.
	b.set_color(Color(0.78, 0.74, 0.64));
	b.box(Vector3(0, 4.5f, 0), Vector3(14.0f, 9.0f, 22.0f));
	b.set_color(Color(0.20, 0.22, 0.26));
	b.tapered_box(Vector3(0, 10.6f, 0), Vector3(15.0f, 3.2f, 23.0f), 0.55f, 0.75f);
	b.box(Vector3(0, 12.6f, 0), Vector3(8.2f, 0.9f, 17.0f));
	for (int side = -1; side <= 1; side += 2) {
		windows_row(b, 7.02f * side, 6.6f, -9.0f, 9.0f, 6, Vector3(0.06f, 1.8f, 1.1f));
		windows_row(b, 7.02f * side, 2.6f, -9.0f, 9.0f, 6, Vector3(0.06f, 1.8f, 1.1f));
		Vector3 tc(7.2f * side, 0, -11.2f);
		b.set_color(Color(0.72, 0.68, 0.58));
		b.cylinder(tc, tc + Vector3(0, 11.0f, 0), 2.4f, 2.4f, 12);
		b.set_color(Color(0.20, 0.22, 0.26));
		b.cylinder(tc + Vector3(0, 11.0f, 0), tc + Vector3(0, 16.0f, 0), 2.7f, 0.1f, 12);
	}
	b.set_color(Color(0.22, 0.14, 0.08));
	b.box(Vector3(0, 1.4f, -11.03f), Vector3(2.4f, 2.8f, 0.06f));
	b.set_color(Color(0.55, 0.5, 0.42));
	b.box(Vector3(0, 0.15f, -13.0f), Vector3(6.0f, 0.3f, 4.0f)); // steps
}

void build_ramp(MeshBuilder &b) {
	// V-1 launching ramp: 48 m of inclined rail on trestles, rising toward -Z.
	b.set_color(Color(0.45, 0.45, 0.42));
	b.box(Vector3(0, 0.3f, 0), Vector3(4.5f, 0.6f, 52.0f)); // concrete base
	const float angle = deg2rad(6.5f);
	Transform3D t(Basis(Vector3(1, 0, 0), angle), Vector3(0, 1.2f, 0));
	b.set_transform(t);
	b.set_color(C_STEEL);
	b.box(Vector3(-0.6f, 0, 0), Vector3(0.3f, 0.5f, 48.0f));
	b.box(Vector3(0.6f, 0, 0), Vector3(0.3f, 0.5f, 48.0f));
	b.box(Vector3(0, 0.1f, 0), Vector3(1.6f, 0.15f, 48.0f));
	b.reset_transform();
	for (int i = 0; i < 7; i++) {
		float z = -21.0f + i * 7.0f;
		float h = 1.2f - z * std::tan(angle);
		b.set_color(Color(0.28, 0.29, 0.3));
		b.box(Vector3(-0.9f, h * 0.5f, z), Vector3(0.2f, h, 0.2f));
		b.box(Vector3(0.9f, h * 0.5f, z), Vector3(0.2f, h, 0.2f));
		b.box(Vector3(0, h * 0.5f, z), Vector3(2.0f, 0.15f, 0.15f));
	}
	b.set_color(Color(0.2, 0.22, 0.17));
	b.box(Vector3(0, 1.6f, 27.0f), Vector3(4.0f, 2.6f, 4.0f)); // launch shelter
}

void build_storage(MeshBuilder &b) {
	// Long concrete store with a kink at one end, the "ski" of the Noball sites.
	b.set_color(Color(0.50, 0.50, 0.46));
	b.tapered_box(Vector3(0, 2.4f, 4.0f), Vector3(6.5f, 4.8f, 52.0f), 0.85f, 1.0f);
	b.set_transform(Transform3D(Basis(Vector3(0, 1, 0), deg2rad(35.0f)), Vector3(3.5f, 0, -27.0f)));
	b.tapered_box(Vector3(0, 2.4f, -5.0f), Vector3(6.5f, 4.8f, 14.0f), 0.85f, 1.0f);
	b.reset_transform();
	b.set_color(Color(0.42, 0.42, 0.39));
	b.tapered_box(Vector3(0, 5.0f, 4.0f), Vector3(6.0f, 0.5f, 51.0f), 0.85f, 1.0f);
	b.set_color(Color(0.3, 0.25, 0.18));
	b.tapered_box(Vector3(0, 0.6f, 4.0f), Vector3(9.5f, 1.2f, 54.0f), 0.7f, 0.98f); // earth banking
}

void build_crater(MeshBuilder &b) {
	// Shallow scorched dish, radius 1.
	const int n = 18;
	std::vector<Ring> rings;
	const float radii[] = { 1.0f, 0.8f, 0.55f, 0.25f };
	const float heights[] = { 0.02f, 0.12f, 0.04f, 0.03f };
	for (int k = 0; k < 4; k++) {
		Ring r;
		for (int i = 0; i < n; i++) {
			float a = TAU_F * i / n;
			float wob = 1.0f + 0.12f * std::sin(a * 5.0f + k) + 0.08f * std::sin(a * 9.0f);
			r.push_back(Vector3(std::cos(a) * radii[k] * wob, heights[k], std::sin(a) * radii[k] * wob));
		}
		rings.push_back(r);
	}
	for (size_t k = 0; k + 1 < rings.size(); k++) {
		float tone = k == 0 ? 0.17f : (k == 1 ? 0.09f : 0.05f);
		b.set_color(Color(tone * 1.2f, tone, tone * 0.8f));
		for (int i = 0; i < n; i++) {
			int j = (i + 1) % n;
			b.quad(rings[k][i], rings[k + 1][i], rings[k + 1][j], rings[k][j]);
		}
	}
	b.set_color(Color(0.04, 0.035, 0.03));
	Vector3 c(0, 0.03f, 0);
	for (int i = 0; i < n; i++) {
		int j = (i + 1) % n;
		b.tri(c, rings[3][j], rings[3][i]);
	}
}

Ref<ArrayMesh> build_prop(const String &name) {
	MeshBuilder b;
	float smooth = 50.0f;
	float rough = 0.85f;
	float metal = 0.0f;
	if (name == "truck") {
		build_truck(b);
	} else if (name == "staff_car") {
		build_staff_car(b);
	} else if (name == "tank") {
		build_tank(b);
		rough = 0.7f;
		metal = 0.2f;
	} else if (name == "halftrack") {
		build_halftrack(b);
		rough = 0.7f;
		metal = 0.2f;
	} else if (name == "flak") {
		build_flak(b);
	} else if (name == "sandbags") {
		build_sandbags(b, 3.0f);
	} else if (name == "house") {
		build_house(b);
	} else if (name == "barn") {
		build_barn(b);
	} else if (name == "warehouse") {
		build_warehouse(b);
	} else if (name == "factory") {
		build_factory(b);
	} else if (name == "church") {
		build_church(b);
	} else if (name == "hangar") {
		build_hangar(b);
		smooth = 30.0f;
	} else if (name == "tower") {
		build_tower(b);
	} else if (name == "fuel_tank") {
		build_fuel_tank(b);
		rough = 0.5f;
		metal = 0.5f;
	} else if (name == "loco") {
		build_loco(b);
		rough = 0.5f;
		metal = 0.4f;
	} else if (name == "boxcar") {
		build_boxcar(b);
	} else if (name == "tanker") {
		build_tanker(b);
		rough = 0.5f;
		metal = 0.4f;
	} else if (name == "flatcar") {
		build_flatcar(b);
	} else if (name == "tree") {
		build_tree(b);
		smooth = 20.0f;
	} else if (name == "poplar") {
		build_poplar(b);
		smooth = 20.0f;
	} else if (name == "pine") {
		build_pine(b);
		smooth = 20.0f;
	} else if (name == "bush") {
		build_bush(b);
		smooth = 20.0f;
	} else if (name == "soldier") {
		build_soldier(b);
	} else if (name == "bomb") {
		build_bomb(b);
		rough = 0.5f;
		metal = 0.3f;
	} else if (name == "bridge") {
		build_bridge(b);
	} else if (name == "tent") {
		build_tent(b);
	} else if (name == "crate_stack") {
		build_crates(b);
	} else if (name == "flak_wagon") {
		build_flak_wagon(b);
	} else if (name == "barge") {
		build_barge(b);
		rough = 0.75f;
	} else if (name == "landing_craft") {
		build_landing_craft(b);
		rough = 0.6f;
		metal = 0.3f;
	} else if (name == "bunker") {
		build_bunker(b);
		rough = 1.0f;
	} else if (name == "coastal_gun") {
		build_coastal_gun(b);
	} else if (name == "mg_nest") {
		build_mg_nest(b);
	} else if (name == "chateau") {
		build_chateau(b);
	} else if (name == "ramp") {
		build_ramp(b);
		rough = 0.6f;
		metal = 0.3f;
	} else if (name == "storage") {
		build_storage(b);
		rough = 1.0f;
	} else if (name == "crater") {
		build_crater(b);
		rough = 1.0f;
	} else {
		b.set_color(Color(1, 0, 1));
		b.box(Vector3(0, 1, 0), Vector3(2, 2, 2));
	}
	return b.build(mats::vertex_color(rough, metal), smooth);
}

} // namespace

const PlaneInfo &plane_info(PlaneType type) {
	return get_spec(type).info;
}

Node3D *make_plane(PlaneType type, const Ref<ShaderMaterial> &paint, bool spinning_prop) {
	auto it = mc().planes.find((int)type);
	if (it == mc().planes.end()) {
		it = mc().planes.emplace((int)type, build_plane(type)).first;
	}
	const PlaneMeshes &m = it->second;
	const PlaneSpec &s = get_spec(type);

	Node3D *root = memnew(Node3D);
	root->set_name("Plane");

	MeshInstance3D *body = memnew(MeshInstance3D);
	body->set_name("Body");
	body->set_mesh(m.body);
	body->set_surface_override_material(0, paint);
	root->add_child(body);

	if (s.has_canopy || !s.nose_glass.empty()) {
		MeshInstance3D *canopy = memnew(MeshInstance3D);
		canopy->set_name("Canopy");
		canopy->set_mesh(m.canopy);
		canopy->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
		root->add_child(canopy);
	}

	// Multi-engine types spin a disc on every nacelle.
	int prop_index = 0;
	for (const PlaneSpec::Nacelle &n : s.nacelles) {
		if (!n.prop) {
			continue;
		}
		MeshInstance3D *disc = memnew(MeshInstance3D);
		disc->set_name(prop_index == 0 ? "Prop" : String("Prop") + String::num_int64(prop_index + 1));
		disc->set_mesh(m.prop_disc);
		disc->set_scale(Vector3(s.info.prop_radius, s.info.prop_radius, 1.0f));
		disc->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
		disc->set_position(Vector3(n.x, n.y, n.z0 - s.spinner_len * 0.45f));
		root->add_child(disc);
		prop_index++;
	}
	if (!s.nose_prop) {
		return root;
	}

	MeshInstance3D *prop = memnew(MeshInstance3D);
	prop->set_name("Prop");
	Vector3 hub(0, s.fuselage[0].y, s.nose_z - s.spinner_len * 0.45f);
	if (spinning_prop) {
		prop->set_mesh(m.prop_disc);
		prop->set_scale(Vector3(s.info.prop_radius, s.info.prop_radius, 1.0f));
		prop->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	} else {
		prop->set_mesh(m.prop_static);
		MeshInstance3D *gear = memnew(MeshInstance3D);
		gear->set_name("Gear");
		gear->set_mesh(m.gear);
		root->add_child(gear);
	}
	prop->set_position(hub);
	root->add_child(prop);
	return root;
}

Ref<ArrayMesh> mesh(const String &name) {
	std::string key = name.utf8().get_data();
	auto it = mc().props.find(key);
	if (it != mc().props.end()) {
		return it->second;
	}
	Ref<ArrayMesh> m = build_prop(name);
	mc().props[key] = m;
	return m;
}

MeshInstance3D *instance(const String &name) {
	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_mesh(mesh(name));
	return mi;
}

Ref<StandardMaterial3D> burnt_material() {
	if (mc().burnt.is_null()) {
		Ref<StandardMaterial3D> m;
		m.instantiate();
		m->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		m->set_flag(BaseMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
		m->set_albedo(Color(0.16, 0.14, 0.13));
		m->set_roughness(1.0f);
		mc().burnt = m;
	}
	return mc().burnt;
}

void clear_cache() {
	delete cache;
	cache = nullptr;
}

} // namespace models
} // namespace ww2
