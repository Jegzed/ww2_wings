#include "core/world_env.h"

#include "core/models.h"

#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/plane_mesh.hpp>
#include <godot_cpp/classes/sky.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>

#include "core/shaders.h"

using namespace godot;

namespace ww2 {

namespace {
struct SkyLook {
	float sun_elevation;
	Color sun_color;
	float sun_energy;
	Color zenith, horizon, ground;
	Color cloud_lit, cloud_shade;
	Color fog;
	float ambient;
	float exposure;
};

SkyLook look_for(TimeOfDay t) {
	switch (t) {
		case TOD_MORNING:
			return { 24.0f, Color(1.0, 0.86, 0.68), 1.55f, Color(0.07, 0.24, 0.62), Color(0.74, 0.80, 0.86),
				Color(0.74, 0.80, 0.86), Color(1.0, 0.95, 0.86), Color(0.58, 0.62, 0.74), Color(0.74, 0.80, 0.86), 0.9f,
				1.0f };
		case TOD_DUSK:
			return { 9.0f, Color(1.0, 0.76, 0.56), 1.45f, Color(0.10, 0.18, 0.45), Color(0.95, 0.66, 0.46),
				Color(0.90, 0.66, 0.50), Color(1.0, 0.76, 0.56), Color(0.40, 0.36, 0.52), Color(0.90, 0.68, 0.52), 0.8f,
				1.05f };
		case TOD_OVERCAST:
			return { 38.0f, Color(0.90, 0.92, 0.95), 0.85f, Color(0.42, 0.48, 0.56), Color(0.72, 0.75, 0.78),
				Color(0.72, 0.75, 0.78), Color(0.86, 0.88, 0.90), Color(0.48, 0.52, 0.58), Color(0.72, 0.75, 0.78), 1.1f,
				1.0f };
		case TOD_NOON:
		default:
			return { 52.0f, Color(1.0, 0.96, 0.88), 1.45f, Color(0.05, 0.22, 0.62), Color(0.66, 0.79, 0.92),
				Color(0.66, 0.79, 0.92), Color(1.0, 0.99, 0.96), Color(0.60, 0.67, 0.80), Color(0.66, 0.79, 0.92), 1.0f,
				1.0f };
	}
}
} // namespace

WorldKit build_world(Node *parent, const WorldOptions &options) {
	WorldKit kit;
	SkyLook look = look_for(options.time);

	Ref<Shader> shader;
	shader.instantiate();
	shader->set_code(shaders::SKY);

	kit.sky_material.instantiate();
	kit.sky_material->set_shader(shader);
	kit.sky_material->set_shader_parameter("zenith_color", look.zenith);
	kit.sky_material->set_shader_parameter("horizon_color", look.horizon);
	kit.sky_material->set_shader_parameter("ground_color", look.ground);
	kit.sky_material->set_shader_parameter("sun_tint", look.sun_color);
	kit.sky_material->set_shader_parameter("cloud_lit", look.cloud_lit);
	kit.sky_material->set_shader_parameter("cloud_shade", look.cloud_shade);
	kit.sky_material->set_shader_parameter("cloud_cover", options.time == TOD_OVERCAST ? 0.85f : options.cloud_cover);
	kit.sky_material->set_shader_parameter("cloud_offset", Vector2(grng().range(0, 50), grng().range(0, 50)));

	Ref<Sky> sky;
	sky.instantiate();
	sky->set_material(kit.sky_material);
	sky->set_radiance_size(Sky::RADIANCE_SIZE_256);

	kit.environment.instantiate();
	Ref<Environment> env = kit.environment;
	env->set_background(Environment::BG_SKY);
	env->set_sky(sky);
	env->set_ambient_source(Environment::AMBIENT_SOURCE_SKY);
	env->set_ambient_light_energy(look.ambient);
	// Keep some neutral fill so strongly tinted skies do not dye everything.
	env->set_ambient_light_color(Color(0.80f, 0.86f, 1.0f));
	env->set_ambient_light_sky_contribution(0.55f);
	env->set_reflection_source(Environment::REFLECTION_SOURCE_SKY);

	env->set_tonemapper(Environment::TONE_MAPPER_ACES);
	env->set_tonemap_exposure(look.exposure);
	env->set_tonemap_white(6.0f);

	env->set_glow_enabled(true);
	env->set_glow_intensity(0.5f);
	env->set_glow_strength(1.0f);
	env->set_glow_bloom(0.04f);
	env->set_glow_hdr_bleed_threshold(1.5f);
	env->set_glow_blend_mode(Environment::GLOW_BLEND_MODE_SCREEN);

	env->set_ssao_enabled(options.ssao);
	env->set_ssao_radius(2.0f);
	env->set_ssao_intensity(1.6f);

	env->set_fog_enabled(true);
	env->set_fog_light_color(look.fog);
	env->set_fog_light_energy(1.0f);
	env->set_fog_sun_scatter(options.sun_scatter);
	env->set_fog_density(options.fog_density);
	env->set_fog_aerial_perspective(0.0f);
	env->set_fog_sky_affect(0.0f);

	env->set_adjustment_enabled(true);
	env->set_adjustment_contrast(1.06f);
	env->set_adjustment_saturation(1.08f);

	kit.fog_color = look.fog;

	kit.env_node = memnew(WorldEnvironment);
	kit.env_node->set_name("WorldEnvironment");
	kit.env_node->set_environment(env);
	parent->add_child(kit.env_node);

	kit.sun = memnew(DirectionalLight3D);
	kit.sun->set_name("Sun");
	kit.sun->set_color(look.sun_color);
	kit.sun->set_param(Light3D::PARAM_ENERGY, look.sun_energy);
	kit.sun->set_shadow(true);
	kit.sun->set_shadow_mode(DirectionalLight3D::SHADOW_PARALLEL_4_SPLITS);
	kit.sun->set_param(Light3D::PARAM_SHADOW_MAX_DISTANCE, options.shadow_distance);
	kit.sun->set_param(Light3D::PARAM_SHADOW_BIAS, 0.06f);
	kit.sun->set_param(Light3D::PARAM_SHADOW_NORMAL_BIAS, 1.5f);
	kit.sun->set_param(Light3D::PARAM_SHADOW_BLUR, 1.2f);
	float elevation = options.sun_elevation_deg > 0.0f ? options.sun_elevation_deg : look.sun_elevation;
	kit.sun->set_rotation_degrees(Vector3(-elevation, options.sun_azimuth_deg, 0.0f));
	parent->add_child(kit.sun);

	return kit;
}

MeshInstance3D *build_terrain(Node *parent, Ref<ShaderMaterial> &r_material, float size) {
	Ref<PlaneMesh> plane;
	plane.instantiate();
	plane->set_size(Vector2(size, size));
	plane->set_subdivide_width(8);
	plane->set_subdivide_depth(8);

	r_material = mats::terrain();
	plane->set_material(r_material);

	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_name("Terrain");
	mi->set_mesh(plane);
	mi->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	mi->set_extra_cull_margin(size);
	parent->add_child(mi);
	return mi;
}

MultiMeshInstance3D *build_clouds(Node *parent, Rng &rng, int clusters, float area, float alt_min, float alt_max,
		float puff_size) {
	Ref<SphereMesh> sphere;
	sphere.instantiate();
	sphere->set_radius(1.0f);
	sphere->set_height(2.0f);
	sphere->set_radial_segments(14);
	sphere->set_rings(8);
	sphere->set_material(mats::cloud());

	std::vector<Transform3D> xfs;
	for (int i = 0; i < clusters; i++) {
		Vector3 c(rng.range(-area, area), rng.range(alt_min, alt_max), rng.range(-area, area));
		float scale = rng.range(0.6f, 1.6f) * puff_size;
		int puffs = rng.irange(5, 11);
		float stretch = rng.range(1.2f, 2.6f);
		float yaw = rng.range(0, TAU_F);
		for (int p = 0; p < puffs; p++) {
			float t = (float)p / (float)puffs;
			Vector3 o(rng.range(-1.0f, 1.0f) * stretch * scale, rng.range(-0.1f, 0.35f) * scale,
					rng.range(-1.0f, 1.0f) * scale);
			o = Basis(Vector3(0, 1, 0), yaw).xform(o);
			float r = scale * rng.range(0.45f, 0.95f) * (1.0f - 0.3f * t);
			Basis b = Basis().scaled(Vector3(r * rng.range(1.0f, 1.4f), r * rng.range(0.55f, 0.8f), r * rng.range(1.0f, 1.4f)));
			// Flat bases: push puffs up so bottoms roughly align.
			Vector3 pos = c + o + Vector3(0, r * 0.35f, 0);
			xfs.push_back(Transform3D(b, pos));
		}
	}

	Ref<MultiMesh> mm;
	mm.instantiate();
	mm->set_transform_format(MultiMesh::TRANSFORM_3D);
	mm->set_mesh(sphere);
	mm->set_instance_count((int)xfs.size());
	for (size_t i = 0; i < xfs.size(); i++) {
		mm->set_instance_transform((int)i, xfs[i]);
	}

	MultiMeshInstance3D *mmi = memnew(MultiMeshInstance3D);
	mmi->set_name("Clouds");
	mmi->set_multimesh(mm);
	mmi->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	parent->add_child(mmi);
	return mmi;
}

MultiMeshInstance3D *scatter(Node *parent, const String &mesh_name, const std::vector<Transform3D> &transforms,
		bool shadows) {
	Ref<MultiMesh> mm;
	mm.instantiate();
	mm->set_transform_format(MultiMesh::TRANSFORM_3D);
	mm->set_mesh(models::mesh(mesh_name));
	mm->set_instance_count((int)transforms.size());
	for (size_t i = 0; i < transforms.size(); i++) {
		mm->set_instance_transform((int)i, transforms[i]);
	}
	MultiMeshInstance3D *mmi = memnew(MultiMeshInstance3D);
	mmi->set_multimesh(mm);
	mmi->set_cast_shadows_setting(shadows ? GeometryInstance3D::SHADOW_CASTING_SETTING_ON
										  : GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	parent->add_child(mmi);
	return mmi;
}

} // namespace ww2
