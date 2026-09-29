#include "screens/viewer.h"

#include "core/fx.h"
#include "core/models.h"

#include <godot_cpp/classes/input.hpp>

using namespace godot;

namespace ww2 {

void Viewer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_shot", "index"), &Viewer::set_shot);
}

void Viewer::_ready() {
	WorldOptions opt;
	opt.time = TOD_MORNING;
	opt.shadow_distance = 300.0f;
	opt.fog_density = 0.00035f;
	build_world(this, opt);

	Ref<ShaderMaterial> terrain_mat;
	build_terrain(this, terrain_mat, 30000.0f);
	terrain_mat->set_shader_parameter("strip_a", Vector4(0.0f, 140.0f, 1.0f, 0.0f));
	terrain_mat->set_shader_parameter("strip_a_width", 4.0f);
	terrain_mat->set_shader_parameter("strip_a_kind", 1);
	terrain_mat->set_shader_parameter("strip_b", Vector4(0.0f, 170.0f, 1.0f, 0.0f));
	terrain_mat->set_shader_parameter("strip_b_width", 2.2f);
	terrain_mat->set_shader_parameter("strip_b_kind", 2);

	Rng rng(42);
	build_clouds(this, rng, 40, 3000.0f, 500.0f, 900.0f, 120.0f);

	const models::PlaneType types[] = { models::PLANE_P47, models::PLANE_BF109, models::PLANE_FW190, models::PLANE_P47 };
	const mats::PaintScheme paints[] = { mats::PAINT_P47_SILVER, mats::PAINT_BF109_GREY, mats::PAINT_FW190,
		mats::PAINT_P47_OLIVE };
	for (int i = 0; i < 4; i++) {
		Node3D *p = models::make_plane(types[i], mats::plane_paint(paints[i]), true);
		add_child(p);
		p->set_position(Vector3(i * 22.0f, 60.0f, 0.0f));
		p->set_rotation_degrees(Vector3(4.0f, 0.0f, -12.0f));
	}
	// Parked examples.
	for (int i = 0; i < 2; i++) {
		Node3D *p = models::make_plane(i == 0 ? models::PLANE_BF109 : models::PLANE_FW190,
				mats::plane_paint(i == 0 ? mats::PAINT_BF109_ACE : mats::PAINT_FW190), false);
		add_child(p);
		p->set_position(Vector3(-30.0f - i * 16.0f, 1.85f, 100.0f));
		p->set_rotation_degrees(Vector3(11.0f, 200.0f, 0.0f));
	}

	const char *names[] = { "truck", "staff_car", "halftrack", "tank", "flak", "soldier", "tent", "crate_stack",
		"house", "barn", "church", "warehouse", "factory", "hangar", "tower", "fuel_tank",
		"tree", "poplar", "pine", "bush", "bomb", "crater" };
	float x = 0.0f;
	for (const char *n : names) {
		MeshInstance3D *mi = models::instance(n);
		add_child(mi);
		AABB box = mi->get_mesh()->get_aabb();
		x += box.size.x * 0.5f + 3.0f;
		mi->set_position(Vector3(x, 0.0f, 100.0f));
		mi->set_rotation_degrees(Vector3(0, 25.0f, 0));
		x += box.size.x * 0.5f + 3.0f;
	}
	const char *train[] = { "loco", "boxcar", "tanker", "flatcar", "boxcar" };
	float tx = -20.0f;
	for (const char *n : train) {
		MeshInstance3D *mi = models::instance(n);
		add_child(mi);
		mi->set_position(Vector3(tx, 0.0f, 170.0f));
		mi->set_rotation_degrees(Vector3(0, 90.0f, 0));
		tx += 11.5f;
	}
	MeshInstance3D *bridge = models::instance("bridge");
	add_child(bridge);
	bridge->set_position(Vector3(140.0f, 14.0f, 260.0f));
	bridge->set_rotation_degrees(Vector3(0, 90.0f, 0));

	fx::smoke_column(this, Vector3(60.0f, 0.0f, 140.0f), 3.0f, true);

	camera = memnew(Camera3D);
	camera->set_far(20000.0f);
	camera->set_near(0.3f);
	camera->set_fov(50.0f);
	add_child(camera);
	camera->make_current();
	set_shot(0);
}

void Viewer::set_shot(int index) {
	shot = index;
	struct View {
		Vector3 pos, target;
	};
	const View views[] = {
		{ Vector3(-9.0f, 63.0f, -13.0f), Vector3(0.0f, 60.0f, 0.0f) },
		{ Vector3(33.0f, 64.5f, -15.0f), Vector3(33.0f, 60.0f, 0.0f) },
		{ Vector3(66.0f, 57.0f, 14.0f), Vector3(66.0f, 60.0f, 0.0f) },
		{ Vector3(30.0f, 9.0f, 70.0f), Vector3(45.0f, 2.0f, 100.0f) },
		{ Vector3(150.0f, 22.0f, 45.0f), Vector3(160.0f, 4.0f, 100.0f) },
		{ Vector3(10.0f, 10.0f, 140.0f), Vector3(10.0f, 2.0f, 170.0f) },
		{ Vector3(-38.0f, 5.0f, 84.0f), Vector3(-38.0f, 2.0f, 100.0f) },
		{ Vector3(0.0f, 600.0f, 0.0f), Vector3(200.0f, 0.0f, 300.0f) },
		{ Vector3(100.0f, 400.0f, 200.0f), Vector3(100.0f, 0.0f, 199.0f) },
	};
	const int count = (int)(sizeof(views) / sizeof(views[0]));
	const View &v = views[index % count];
	camera->look_at_from_position(v.pos, v.target, Vector3(0, 1, 0));
}

void Viewer::_process(double delta) {
	clock += (float)delta;
	if (Input::get_singleton()->is_action_just_pressed("accept")) {
		set_shot(shot + 1);
	}
}

} // namespace ww2
