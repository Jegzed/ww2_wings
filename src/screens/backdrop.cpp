#include "screens/backdrop.h"

#include "core/audio.h"
#include "core/models.h"

using namespace godot;

namespace ww2 {

void Backdrop::_ready() {
	WorldOptions opt;
	opt.time = time;
	opt.shadow_distance = 120.0f;
	opt.fog_density = 0.00006f;
	opt.sun_scatter = 0.1f;
	opt.cloud_cover = 0.35f;
	opt.sun_azimuth_deg = 205.0f;
	build_world(this, opt);

	Ref<ShaderMaterial> terrain_mat;
	terrain = build_terrain(this, terrain_mat, 240000.0f);

	Rng rng(1944);
	MultiMeshInstance3D *clouds = build_clouds(this, rng, 300, 9000.0f, 1450.0f, 2300.0f, 170.0f);
	clouds->set_position(Vector3(0, 0, -6000.0f));
	MultiMeshInstance3D *high = build_clouds(this, rng, 60, 9000.0f, 2600.0f, 3200.0f, 260.0f);
	high->set_position(Vector3(0, 0, -6000.0f));

	flight = memnew(Node3D);
	add_child(flight);
	flight->set_position(Vector3(0, 1900.0f, 2500.0f));

	const Vector3 slots[] = { Vector3(0, 0, 0), Vector3(-19, 3.0f, 17), Vector3(21, -2.0f, 20), Vector3(-5, 7.0f, 42) };
	for (int i = 0; i < 4; i++) {
		Node3D *p = models::make_plane(models::PLANE_P47,
				mats::plane_paint(i == 0 ? mats::PAINT_P47_SILVER : (i == 2 ? mats::PAINT_P47_OLIVE : mats::PAINT_P47_SILVER)),
				true);
		flight->add_child(p);
		p->set_position(slots[i]);
		planes.push_back(p);
		offsets.push_back(slots[i]);
	}

	camera = memnew(Camera3D);
	camera->set_fov(38.0f);
	camera->set_h_offset(-9.5f);
	camera->set_near(0.5f);
	camera->set_far(40000.0f);
	flight->add_child(camera);
	camera->make_current();

	if (!quiet) {
		engine = audio::make_loop(this, "engine", -22.0f);
		engine->set_pitch_scale(0.9f);
		engine->play();
	}
}

void Backdrop::_exit_tree() {
	if (engine) {
		engine->stop();
	}
}

void Backdrop::_process(double delta) {
	float dt = (float)delta;
	clock += dt;

	Vector3 pos = flight->get_position();
	pos.z -= 95.0f * dt;
	if (pos.z < -13000.0f) {
		pos.z = 2500.0f;
	}
	flight->set_position(pos);
	terrain->set_position(Vector3(0, 0, std::floor(pos.z / 2000.0f) * 2000.0f));

	for (size_t i = 0; i < planes.size(); i++) {
		float ph = clock * 0.6f + (float)i * 1.7f;
		Vector3 bob(std::sin(ph * 0.7f) * 0.8f, std::sin(ph) * 0.9f, std::cos(ph * 0.5f) * 0.8f);
		planes[i]->set_position(offsets[i] + bob);
		planes[i]->set_rotation(Vector3(deg2rad(1.5f) + std::sin(ph) * 0.015f, 0.0f, std::sin(ph * 0.8f) * 0.06f));
	}

	// Slow drift around the leader, kept on the right of the frame for the menu.
	float a = deg2rad(-52.0f) + std::sin(clock * 0.07f) * 0.22f;
	float dist = 27.0f + std::sin(clock * 0.05f) * 3.0f;
	Vector3 cam(std::sin(a) * dist, -1.5f + std::sin(clock * 0.09f) * 1.5f, -std::cos(a) * dist);
	Vector3 target(0.0f, 0.5f, 6.0f);
	camera->look_at_from_position(flight->get_position() + cam, flight->get_position() + target, Vector3(0, 1, 0));
}

} // namespace ww2
