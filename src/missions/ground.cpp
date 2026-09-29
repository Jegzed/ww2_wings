#include "missions/ground.h"

#include "core/audio.h"

using namespace godot;

namespace ww2 {

namespace {
struct KindInfo {
	const char *name;
	float hx, hz, height, hp, bullet_armor;
	int score;
	bool explosive, flak, obstacle;
};

const KindInfo KINDS[] = {
	{ "truck", 1.4f, 3.2f, 3.0f, 30.0f, 1.0f, 100, false, false, false },
	{ "staff_car", 1.1f, 2.1f, 2.0f, 18.0f, 1.0f, 150, false, false, false },
	{ "halftrack", 1.4f, 3.2f, 2.6f, 60.0f, 0.6f, 150, false, false, false },
	{ "tank", 1.7f, 3.2f, 3.0f, 140.0f, 0.22f, 300, false, false, false },
	{ "flak", 3.2f, 3.2f, 3.2f, 45.0f, 1.0f, 200, false, true, false },
	{ "soldier", 0.8f, 0.8f, 1.9f, 4.0f, 1.0f, 20, false, false, false },
	{ "tent", 2.4f, 3.2f, 3.0f, 22.0f, 1.0f, 50, false, false, false },
	{ "crate_stack", 2.4f, 1.6f, 2.2f, 22.0f, 1.0f, 100, true, false, false },
	{ "house", 4.2f, 6.0f, 9.0f, 170.0f, 0.3f, 0, false, false, true },
	{ "barn", 5.0f, 8.5f, 9.0f, 170.0f, 0.3f, 0, false, false, true },
	{ "warehouse", 8.5f, 18.5f, 11.0f, 150.0f, 0.15f, 250, false, false, true },
	{ "factory", 15.5f, 23.0f, 12.0f, 280.0f, 0.1f, 500, false, false, true },
	{ "church", 5.0f, 13.0f, 34.0f, 500.0f, 0.1f, 0, false, false, true },
	{ "hangar", 17.5f, 20.5f, 10.0f, 190.0f, 0.2f, 300, false, false, true },
	{ "tower", 3.3f, 3.3f, 15.0f, 80.0f, 0.4f, 150, false, false, true },
	{ "fuel_tank", 5.8f, 5.8f, 8.0f, 45.0f, 0.8f, 300, true, false, true },
	{ "loco", 1.6f, 6.0f, 5.0f, 90.0f, 0.5f, 400, false, false, false },
	{ "boxcar", 1.6f, 4.8f, 4.2f, 45.0f, 0.8f, 100, false, false, false },
	{ "tanker", 1.6f, 4.8f, 4.2f, 30.0f, 1.0f, 200, true, false, false },
	{ "flatcar", 1.6f, 4.8f, 3.0f, 45.0f, 0.8f, 150, false, false, false },
	{ "bridge", 6.5f, 61.0f, 9.0f, 190.0f, 0.0f, 1200, false, false, false },
	{ "plane_bf109", 5.0f, 4.6f, 3.0f, 35.0f, 1.0f, 250, true, false, false },
	{ "plane_fw190", 5.2f, 4.6f, 3.0f, 35.0f, 1.0f, 250, true, false, false },
	{ "tree", 1.2f, 1.2f, 9.0f, 1e9f, 0.0f, 0, false, false, true },
	{ "poplar", 1.0f, 1.0f, 15.0f, 1e9f, 0.0f, 0, false, false, true },
};

const KindInfo *find_kind(const std::string &name) {
	for (const KindInfo &k : KINDS) {
		if (name == k.name) {
			return &k;
		}
	}
	return &KINDS[0];
}
} // namespace

int GroundWorld::add(const String &kind, const Vector3 &pos, float yaw_deg, bool counts) {
	GroundTarget t;
	t.kind = kind.utf8().get_data();
	const KindInfo *k = find_kind(t.kind);
	t.pos = pos;
	t.yaw = deg2rad(yaw_deg);
	t.hx = k->hx;
	t.hz = k->hz;
	t.height = k->height;
	t.hp = t.max_hp = k->hp;
	t.bullet_armor = k->bullet_armor;
	t.score = k->score;
	t.explosive = k->explosive;
	t.is_flak = k->flak;
	t.is_obstacle = k->obstacle;
	t.counts = counts && k->score > 0;
	t.flak_timer = grng().range(1.0f, 3.0f);

	if (t.kind == "plane_bf109" || t.kind == "plane_fw190") {
		bool fw = t.kind == "plane_fw190";
		t.paint = mats::plane_paint(fw ? mats::PAINT_FW190 : mats::PAINT_BF109_GREY);
		t.node = models::make_plane(fw ? models::PLANE_FW190 : models::PLANE_BF109, t.paint, false);
		root->add_child(t.node);
		t.node->set_position(pos + Vector3(0, 1.85f, 0));
		t.node->set_rotation(Vector3(deg2rad(11.0f), t.yaw, 0.0f));
	} else {
		t.mesh = models::instance(kind);
		t.node = t.mesh;
		root->add_child(t.node);
		t.node->set_position(pos);
		t.node->set_rotation(Vector3(0.0f, t.yaw, 0.0f));
	}
	targets.push_back(t);
	return (int)targets.size() - 1;
}

float GroundWorld::footprint_distance(const GroundTarget &t, const Vector3 &p) const {
	Vector3 rel = p - t.pos;
	float c = std::cos(-t.yaw);
	float s = std::sin(-t.yaw);
	// Rotate into the target's frame (yaw about +Y).
	float lx = rel.x * c + rel.z * s;
	float lz = -rel.x * s + rel.z * c;
	float dx = MAX(std::fabs(lx) - t.hx, 0.0f);
	float dz = MAX(std::fabs(lz) - t.hz, 0.0f);
	return std::sqrt(dx * dx + dz * dz);
}

int GroundWorld::hit_test(const Vector3 &p, float margin) const {
	for (int i = 0; i < (int)targets.size(); i++) {
		const GroundTarget &t = targets[i];
		if (!t.alive) {
			continue;
		}
		if (p.y > t.pos.y + t.height + margin) {
			continue;
		}
		float reach = MAX(t.hx, t.hz) + margin + 1.0f;
		if (std::fabs(p.x - t.pos.x) > reach || std::fabs(p.z - t.pos.z) > reach) {
			continue;
		}
		if (footprint_distance(t, p) <= margin) {
			return i;
		}
	}
	return -1;
}

bool GroundWorld::damage(int index, float amount, bool from_bomb) {
	GroundTarget &t = targets[index];
	if (!t.alive || t.max_hp > 1e8f) {
		return false;
	}
	if (!from_bomb) {
		amount *= t.bullet_armor;
	}
	t.hp -= amount;
	if (t.paint.is_valid()) {
		t.paint->set_shader_parameter("damage", saturate(1.0f - t.hp / t.max_hp));
	}
	if (t.hp <= 0.0f) {
		destroy(index);
		return true;
	}
	return false;
}

void GroundWorld::destroy(int index) {
	GroundTarget &t = targets[index];
	t.alive = false;
	t.hp = 0.0f;
	t.speed = 0.0f;

	float size = MAX(MAX(t.hx, t.hz) * 0.9f, 2.0f) * fx_scale;
	if (t.explosive) {
		size *= 1.6f;
	}
	size = MIN(size, 16.0f);
	Vector3 centre = t.pos + Vector3(0, MIN(t.height * 0.4f, 3.0f), 0);

	if (t.kind == "soldier") {
		t.node->set_rotation(Vector3(deg2rad(90.0f), t.yaw, 0.0f));
		t.node->set_position(t.pos + Vector3(0, 0.3f, 0));
	} else {
		fx::explosion(root, centre, size, true);
		fx::smoke_column(root, t.pos + Vector3(0, 1.0f, 0), MIN(size * 0.55f, 6.0f), true);
		audio::play(t.explosive || size > 8.0f ? "explosion_big" : "explosion", -4.0f, grng().range(0.85f, 1.15f));

		if (t.mesh) {
			t.mesh->set_material_override(models::burnt_material());
			Vector3 scale(1.0f, 0.55f, 1.0f);
			if (t.is_obstacle) {
				scale = Vector3(1.02f, 0.4f, 1.02f); // collapsed building
				t.height *= 0.4f;
			}
			t.node->set_scale(scale);
			t.node->set_rotation(Vector3(grng().range(-0.05f, 0.05f), t.yaw + grng().range(-0.1f, 0.1f),
					grng().range(-0.08f, 0.08f)));
			if (t.kind == "bridge") {
				t.node->set_scale(Vector3(1.0f, 1.0f, 1.0f));
				t.node->set_rotation(Vector3(deg2rad(3.0f), t.yaw, deg2rad(9.0f)));
				t.node->set_position(t.pos + Vector3(0, -5.0f, 0));
				for (int i = -1; i <= 1; i++) {
					Vector3 p = t.pos + Basis(Vector3(0, 1, 0), t.yaw).xform(Vector3(0, 0, i * 35.0f));
					fx::explosion(root, p, 12.0f, true);
					fx::smoke_column(root, p, 5.0f, true);
				}
			}
		} else if (t.paint.is_valid()) {
			t.paint->set_shader_parameter("damage", 1.0f);
			t.node->set_rotation(Vector3(deg2rad(4.0f), t.yaw, deg2rad(grng().chance(0.5f) ? 14.0f : -14.0f)));
			t.node->set_position(t.pos + Vector3(0, 1.1f, 0));
		}
	}

	if (on_destroyed) {
		on_destroyed(index, true);
	}
	if (t.explosive) {
		pending_blasts.push_back(index);
	}
}

int GroundWorld::blast(const Vector3 &at, float radius, float amount, int *r_touched) {
	int killed = 0;
	for (int i = 0; i < (int)targets.size(); i++) {
		GroundTarget &t = targets[i];
		if (!t.alive) {
			continue;
		}
		float reach = radius + MAX(t.hx, t.hz);
		if (std::fabs(at.x - t.pos.x) > reach || std::fabs(at.z - t.pos.z) > reach) {
			continue;
		}
		float d = footprint_distance(t, at);
		if (d >= radius) {
			continue;
		}
		float k = 1.0f - d / radius;
		if (t.counts && r_touched) {
			(*r_touched)++;
		}
		if (damage(i, amount * (0.1f + 0.9f * k), true)) {
			killed++;
		}
	}
	return killed;
}

void GroundWorld::update(float dt) {
	// Chain reactions are deferred a frame so they ripple visibly.
	if (!pending_blasts.empty()) {
		std::vector<int> now;
		now.swap(pending_blasts);
		for (int index : now) {
			const GroundTarget &t = targets[index];
			blast(t.pos, MAX(t.hx, t.hz) + 14.0f, 70.0f);
		}
	}
	for (GroundTarget &t : targets) {
		if (!t.alive || !t.node) {
			continue;
		}
		if (t.speed != 0.0f) {
			t.pos.z -= t.speed * dt;
			t.node->set_position(t.pos);
		}
		if (t.fleeing) {
			t.pos += t.flee * dt;
			t.node->set_position(t.pos);
		}
	}
}

int GroundWorld::total() const {
	int n = 0;
	for (const GroundTarget &t : targets) {
		if (t.counts) {
			n++;
		}
	}
	return n;
}

int GroundWorld::destroyed() const {
	int n = 0;
	for (const GroundTarget &t : targets) {
		if (t.counts && !t.alive) {
			n++;
		}
	}
	return n;
}

} // namespace ww2
