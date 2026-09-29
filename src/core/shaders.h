#pragma once

// GLSL sources for the procedural look of the game. Everything is generated:
// there are no texture assets, so paint schemes, terrain and sky are shaders.

namespace ww2 {
namespace shaders {

static const char *NOISE_LIB = R"GLSL(
float hash12(vec2 p) {
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}
vec2 hash22(vec2 p) {
	vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.xx + p3.yz) * p3.zy);
}
float hash13(vec3 p3) {
	p3 = fract(p3 * 0.1031);
	p3 += dot(p3, p3.zyx + 31.32);
	return fract((p3.x + p3.y) * p3.z);
}
float vnoise(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash12(i), hash12(i + vec2(1.0, 0.0)), u.x),
			mix(hash12(i + vec2(0.0, 1.0)), hash12(i + vec2(1.0, 1.0)), u.x), u.y);
}
float vnoise3(vec3 p) {
	vec3 i = floor(p);
	vec3 f = fract(p);
	vec3 u = f * f * (3.0 - 2.0 * f);
	float a = mix(mix(hash13(i), hash13(i + vec3(1, 0, 0)), u.x),
			mix(hash13(i + vec3(0, 1, 0)), hash13(i + vec3(1, 1, 0)), u.x), u.y);
	float b = mix(mix(hash13(i + vec3(0, 0, 1)), hash13(i + vec3(1, 0, 1)), u.x),
			mix(hash13(i + vec3(0, 1, 1)), hash13(i + vec3(1, 1, 1)), u.x), u.y);
	return mix(a, b, u.z);
}
float fbm(vec2 p) {
	float v = 0.0;
	float a = 0.5;
	mat2 m = mat2(vec2(1.6, 1.2), vec2(-1.2, 1.6));
	for (int i = 0; i < 5; i++) {
		v += a * vnoise(p);
		p = m * p;
		a *= 0.5;
	}
	return v;
}
)GLSL";

static const char *PLANE_PAINT = R"GLSL(
uniform vec3 top_color : source_color = vec3(0.6, 0.62, 0.65);
uniform vec3 bottom_color : source_color = vec3(0.6, 0.62, 0.65);
uniform vec3 nose_color : source_color = vec3(0.7, 0.1, 0.08);
uniform vec3 accent_color : source_color = vec3(0.7, 0.1, 0.08);
uniform vec3 camo_color : source_color = vec3(0.2, 0.25, 0.2);
uniform vec3 glare_color : source_color = vec3(0.2, 0.23, 0.13);
uniform float camo = 0.0;
uniform float glare_panel = 0.0;
uniform float nose_z = -3.9;
uniform float rudder_z = 100.0;
uniform int insignia = 0;
uniform float stripes = 0.0;
uniform float stripe_w = 0.32;
uniform float stripe_wing_x = 2.3;
uniform float stripe_fus_z = 2.4;
uniform vec3 wing_mark = vec3(4.4, -0.1, 0.62); // x, z, radius
uniform vec3 fus_mark = vec3(1.9, 0.1, 0.42); // z, y, radius
uniform float metal = 0.85;
uniform float rough = 0.38;
uniform float damage = 0.0;
uniform float cockpit_z0 = -0.8;

varying vec3 mpos;
varying vec3 mnrm;

void vertex() {
	mpos = VERTEX;
	mnrm = NORMAL;
}

float sd_star5(vec2 p, float r, float rf) {
	const vec2 k1 = vec2(0.809016994375, -0.587785252292);
	const vec2 k2 = vec2(-0.809016994375, -0.587785252292);
	p.x = abs(p.x);
	p -= 2.0 * max(dot(k1, p), 0.0) * k1;
	p -= 2.0 * max(dot(k2, p), 0.0) * k2;
	p.x = abs(p.x);
	p.y -= r;
	vec2 ba = rf * vec2(-k1.y, k1.x) - vec2(0.0, 1.0);
	float h = clamp(dot(p, ba) / dot(ba, ba), 0.0, r);
	return length(p - ba * h) * sign(p.y * ba.x - p.x * ba.y);
}

// p is normalised so the roundel has radius 1. Returns rgb + coverage.
vec4 mark_us(vec2 p) {
	vec3 blue = vec3(0.02, 0.05, 0.16);
	vec3 white = vec3(0.92);
	vec4 c = vec4(0.0);
	vec2 a = abs(p);
	if (a.x < 2.15 && a.y < 0.52) {
		c = vec4(blue, 1.0);
	}
	if (a.x < 2.03 && a.y < 0.40) {
		c = vec4(white, 1.0);
	}
	if (length(p) < 1.0) {
		c = vec4(blue, 1.0);
		if (sd_star5(p, 0.98, 0.382) < 0.0) {
			c = vec4(white, 1.0);
		}
	}
	return c;
}

vec4 mark_cross(vec2 p) {
	vec2 a = abs(p);
	vec4 c = vec4(0.0);
	if ((a.x < 0.44 && a.y < 1.0) || (a.y < 0.44 && a.x < 1.0)) {
		c = vec4(vec3(0.9), 1.0);
	}
	if ((a.x < 0.27 && a.y < 1.0) || (a.y < 0.27 && a.x < 1.0)) {
		c = vec4(vec3(0.03), 1.0);
	}
	return c;
}

vec4 mark(vec2 p) {
	if (insignia == 1) {
		return mark_us(p);
	} else if (insignia == 2) {
		return mark_cross(p);
	}
	return vec4(0.0);
}

void fragment() {
	float part = COLOR.a;
	bool is_fuselage = part > 0.9;
	bool is_wing = part > 0.7 && part <= 0.9;
	bool is_fin = part > 0.45 && part <= 0.55;
	vec3 n = normalize(mnrm);

	float upper = smoothstep(-0.35, 0.0, n.y);
	vec3 top = top_color;
	if (camo > 0.0) {
		float blotch = vnoise3(mpos * 0.75 + vec3(11.0));
		blotch += 0.5 * vnoise3(mpos * 2.1);
		top = mix(top, camo_color, smoothstep(0.62, 0.78, blotch) * camo);
		// Mottling fading down the fuselage sides.
		float mott = vnoise3(mpos * 5.0);
		top = mix(top, bottom_color, (1.0 - smoothstep(-0.1, 0.55, n.y)) * smoothstep(0.35, 0.7, mott) * 0.8 * camo);
	}
	vec3 col = mix(bottom_color, top, upper);
	float paint_metal = metal;
	float paint_rough = rough;

	// Anti-glare panel ahead of the windscreen.
	if (is_fuselage && glare_panel > 0.0 && n.y > 0.72 && mpos.z < cockpit_z0 && mpos.z > nose_z) {
		col = glare_color;
		paint_metal = 0.0;
		paint_rough = 0.85;
	}
	if (is_fuselage && mpos.z < nose_z) {
		col = nose_color;
		paint_metal = 0.1;
		paint_rough = 0.5;
	}
	if ((is_fin || is_fuselage) && mpos.z > rudder_z) {
		col = accent_color;
		paint_metal = 0.1;
		paint_rough = 0.55;
	}

	// Invasion stripes.
	if (stripes > 0.5) {
		float s = -1.0;
		if (is_wing) {
			s = (abs(mpos.x) - stripe_wing_x) / stripe_w;
		} else if (is_fuselage) {
			s = (mpos.z - stripe_fus_z) / stripe_w;
		}
		if (s >= 0.0 && s < 5.0) {
			col = mod(floor(s), 2.0) < 0.5 ? vec3(0.9) : vec3(0.03);
			paint_metal = 0.0;
			paint_rough = 0.7;
		}
	}

	// National markings.
	if (is_wing && abs(n.y) > 0.5) {
		vec4 m = mark(vec2(abs(mpos.x) - wing_mark.x, -(mpos.z - wing_mark.y)) / wing_mark.z);
		if (m.a > 0.5) {
			col = m.rgb;
			paint_metal = 0.0;
			paint_rough = 0.7;
		}
	}
	if (is_fuselage && abs(n.x) > 0.45) {
		vec4 m = mark(vec2(mpos.z - fus_mark.x, mpos.y - fus_mark.y) / fus_mark.z);
		if (m.a > 0.5) {
			col = m.rgb;
			paint_metal = 0.0;
			paint_rough = 0.7;
		}
	}

	// Panel lines and rivet-ish wear.
	vec2 pc = is_wing ? vec2(mpos.x * 1.1, mpos.z * 1.6) : vec2(mpos.z * 1.1, atan(mpos.y, mpos.x) * 1.6);
	vec2 g = abs(fract(pc) - 0.5);
	float line = 1.0 - smoothstep(0.0, 0.025, min(g.x, g.y));
	col *= 1.0 - line * 0.32;

	float wear = vnoise3(mpos * 3.0) * 0.6 + vnoise3(mpos * 11.0) * 0.4;
	col *= 0.86 + wear * 0.22;
	paint_rough = clamp(paint_rough + (wear - 0.5) * 0.25, 0.05, 1.0);

	// Exhaust and gun soot trailing back along the fuselage and wings.
	float soot = 0.0;
	if (is_fuselage) {
		soot = smoothstep(0.5, 0.0, abs(mpos.y + 0.15)) * smoothstep(nose_z, nose_z + 0.6, mpos.z) *
				smoothstep(nose_z + 4.5, nose_z + 0.8, mpos.z) * abs(n.x);
	}
	soot = clamp(soot * 0.55 + damage * smoothstep(0.35, 0.75, vnoise3(mpos * 1.7 + vec3(3.0))) * 1.2, 0.0, 1.0);
	col = mix(col, vec3(0.02), soot * 0.85);
	paint_rough = mix(paint_rough, 0.95, soot);
	paint_metal = mix(paint_metal, 0.0, soot);

	ALBEDO = col * COLOR.rgb;
	METALLIC = paint_metal;
	ROUGHNESS = paint_rough;
	SPECULAR = 0.5;
}
)GLSL";

static const char *PROP_DISC = R"GLSL(
shader_type spatial;
render_mode blend_mix, cull_disabled, unshaded, depth_draw_never, shadows_disabled;

uniform float blades = 4.0;
uniform float spin = 1.0;
uniform vec3 tip_color : source_color = vec3(0.9, 0.75, 0.1);

varying vec3 mpos;

void vertex() {
	mpos = VERTEX;
}

void fragment() {
	float r = length(mpos.xy);
	float a = atan(mpos.y, mpos.x);
	float streak = 0.5 + 0.5 * sin(a * blades + TIME * 37.0 * spin);
	float alpha = mix(0.10, 0.26, streak * streak);
	alpha *= smoothstep(1.0, 0.93, r) * smoothstep(0.08, 0.2, r);
	vec3 col = vec3(0.06);
	col = mix(col, tip_color, smoothstep(0.86, 0.9, r));
	ALBEDO = col;
	ALPHA = alpha;
}
)GLSL";

static const char *TERRAIN_A = R"GLSL(
uniform float field_scale = 170.0;
uniform float season = 0.0;
uniform vec4 strip_a = vec4(0.0, 0.0, 0.0, 1.0); // origin xz, dir xz
uniform vec4 strip_b = vec4(0.0, 0.0, 1.0, 0.0);
uniform float strip_a_width = 0.0;
uniform float strip_b_width = 0.0;
uniform int strip_a_kind = 0; // 1 road, 2 rail, 3 runway, 4 river, 5 canal
uniform int strip_b_kind = 0;
uniform vec4 strip_c = vec4(0.0, 0.0, 0.0, 1.0);
uniform float strip_c_width = 0.0;
uniform int strip_c_kind = 0;
uniform vec2 strip_a_range = vec2(-1e6, 1e6); // extent along the strip
uniform vec2 strip_b_range = vec2(-1e6, 1e6);
uniform vec2 strip_c_range = vec2(-1e6, 1e6);
uniform vec4 field_rect = vec4(0.0); // airfield: centre xz, half size xz
uniform float river_amount = 1.0;
uniform float wood_amount = 1.0;
uniform float town_amount = 0.0;

varying vec3 wpos;

void vertex() {
	wpos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
}

struct Surf {
	vec3 col;
	float rough;
	float water;
};

vec3 field_palette(float h) {
	if (h < 0.28) {
		return vec3(0.17, 0.30, 0.09); // pasture
	} else if (h < 0.48) {
		return vec3(0.24, 0.36, 0.11); // meadow
	} else if (h < 0.64) {
		return vec3(0.52, 0.44, 0.17); // wheat
	} else if (h < 0.78) {
		return vec3(0.27, 0.19, 0.11); // ploughed
	} else if (h < 0.9) {
		return vec3(0.12, 0.25, 0.08); // crops
	}
	return vec3(0.40, 0.42, 0.16); // hay
}

void apply_strip(inout Surf s, vec2 p, vec4 strip, float width, int kind, vec2 range) {
	if (kind == 0 || width <= 0.0) {
		return;
	}
	vec2 d = normalize(strip.zw);
	vec2 rel = p - strip.xy;
	float along = dot(rel, d);
	float side = dot(rel, vec2(-d.y, d.x));
	if (kind == 4) {
		side += (fbm(vec2(along * 0.004, 3.7)) - 0.5) * width * 3.0;
	}
	float a = abs(side);
	float aa = max(fwidth(side), 0.001);
	float inside = 1.0 - smoothstep(width - aa, width + aa, a);
	// Optional extent along the strip, with tapered ends.
	float ends = smoothstep(range.x - 1.0, range.x + 30.0, along) * (1.0 - smoothstep(range.y - 30.0, range.y + 1.0, along));
	if (kind == 2) {
		// Yards narrow down to the through line at each end.
		inside = 1.0 - smoothstep(width * ends - aa, width * ends + aa, a);
	} else {
		inside *= step(range.x, along) * step(along, range.y);
	}
	if (inside <= 0.0) {
		// verge
		float verge = (1.0 - smoothstep(width, width * 1.6 + 1.5, a)) * step(range.x, along) * step(along, range.y);
		s.col = mix(s.col, s.col * 0.8 + vec3(0.05, 0.04, 0.02), verge * 0.6);
		return;
	}
	vec3 c;
	float r = 0.9;
	float w = 0.0;
	if (kind == 1) {
		c = vec3(0.33, 0.29, 0.22) * (0.85 + 0.3 * vnoise(p * 0.7));
		// wheel ruts
		c *= 1.0 - 0.18 * (1.0 - smoothstep(0.0, 0.5, abs(a - width * 0.45)));
	} else if (kind == 2) {
		c = vec3(0.17, 0.15, 0.14) * (0.8 + 0.4 * vnoise(p * 1.5));
		// Wide strips are marshalling yards: a track every 7 m.
		float t = width > 5.0 ? abs(mod(side + 3.5, 7.0) - 3.5) : a;
		float sleeper = step(0.5, fract(along * 0.9));
		float track = 1.0 - smoothstep(0.9, 1.3, t);
		c = mix(c, vec3(0.12, 0.08, 0.05), sleeper * track * 0.8);
		float rail = 1.0 - smoothstep(0.08, 0.16, abs(t - 0.72));
		c = mix(c, vec3(0.45, 0.43, 0.42), rail);
		r = mix(0.9, 0.35, rail);
	} else if (kind == 3) {
		c = vec3(0.34, 0.34, 0.33) * (0.85 + 0.25 * vnoise(p * 0.35));
		float slab = min(abs(fract(along / 12.0) - 0.5), abs(fract(side / 12.0) - 0.5));
		c *= 1.0 - 0.25 * (1.0 - smoothstep(0.0, 0.012, slab));
		float dash = step(0.5, fract(along / 40.0)) * (1.0 - smoothstep(0.5, 0.8, a));
		c = mix(c, vec3(0.8), dash * 0.8);
		// tyre marks
		c *= 1.0 - 0.2 * (1.0 - smoothstep(1.0, 5.0, abs(a - 4.0))) * vnoise(vec2(along * 0.02, side));
	} else {
		c = vec3(0.05, 0.12, 0.13);
		r = 0.08;
		w = 1.0;
	}
	s.col = mix(s.col, c, inside);
	s.rough = mix(s.rough, r, inside);
	s.water = mix(s.water, w, inside);
}
)GLSL";

static const char *TERRAIN_B = R"GLSL(
void fragment() {
	vec2 p = wpos.xz;
	float dist = length(wpos - CAMERA_POSITION_WORLD);
	vec2 q = p / field_scale;

	// Bocage: irregular fields from a jittered Voronoi diagram.
	vec2 cell = floor(q);
	vec2 f = fract(q);
	float d1 = 8.0;
	float d2 = 8.0;
	vec2 id = vec2(0.0);
	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			vec2 o = vec2(float(x), float(y));
			vec2 r = o + 0.5 + (hash22(cell + o) - 0.5) * 0.85 - f;
			float d = max(abs(r.x), abs(r.y)) * 0.6 + length(r) * 0.4;
			if (d < d1) {
				d2 = d1;
				d1 = d;
				id = cell + o;
			} else if (d < d2) {
				d2 = d;
			}
		}
	}
	float edge = d2 - d1;
	float h = hash12(id + 7.31);
	vec3 col = field_palette(h);
	col *= 0.8 + 0.4 * hash12(id + 1.7);

	// Crop rows.
	float ang = hash12(id + 3.3) * 3.14159;
	vec2 rd = vec2(cos(ang), sin(ang));
	float rows = sin(dot(p, rd) * 1.4);
	float row_fade = 1.0 - smoothstep(60.0, 420.0, dist);
	float row_amt = (h > 0.48 && h < 0.9) ? 0.16 : 0.05;
	col *= 1.0 + rows * row_amt * row_fade;

	// Broad and fine variation.
	float big = fbm(p * 0.0021);
	col *= 0.78 + 0.5 * big;
	float fine = vnoise(p * 0.6) * 0.6 + vnoise(p * 2.3) * 0.4;
	col *= 0.93 + 0.14 * fine * (1.0 - smoothstep(60.0, 500.0, dist));
	// Mid-scale mottling keeps fields from looking flat at altitude.
	col *= 0.92 + 0.16 * vnoise(p * 0.035);
	col = mix(col, col * vec3(1.1, 0.95, 0.7), season);

	// Woods.
	float wood = smoothstep(0.58, 0.66, fbm(p * 0.0037 + vec2(40.0, 17.0))) * wood_amount;
	vec3 wood_col = vec3(0.045, 0.12, 0.04) * (0.7 + 0.7 * vnoise(p * 0.12));
	col = mix(col, wood_col, wood);

	// Hedgerows between fields.
	float hedge = (1.0 - smoothstep(0.015, 0.05, edge)) * (1.0 - wood);
	col = mix(col, vec3(0.05, 0.13, 0.04) * (0.7 + 0.6 * vnoise(p * 0.35)), hedge);

	Surf s;
	s.col = col;
	s.rough = 0.95;
	s.water = 0.0;

	// Meandering river from a noise iso-line.
	if (river_amount > 0.0) {
		float rv = abs(fbm(p * 0.00045 + vec2(9.0, 2.0)) - 0.5);
		float bank = 1.0 - smoothstep(0.006, 0.011, rv);
		float water = 1.0 - smoothstep(0.004, 0.0055, rv);
		s.col = mix(s.col, vec3(0.30, 0.27, 0.16), bank * river_amount);
		s.col = mix(s.col, vec3(0.04, 0.10, 0.11), water * river_amount);
		s.rough = mix(s.rough, 0.07, water * river_amount);
		s.water = water * river_amount;
	}

	// Airfield grass.
	if (field_rect.z > 0.0) {
		vec2 a = abs(p - field_rect.xy) - field_rect.zw;
		float inside = 1.0 - smoothstep(0.0, 30.0, max(a.x, a.y));
		vec3 grass = vec3(0.23, 0.33, 0.12) * (0.85 + 0.3 * vnoise(p * 0.05)) * (0.92 + 0.16 * fine);
		s.col = mix(s.col, grass, inside);
		s.rough = mix(s.rough, 0.95, inside);
		s.water *= 1.0 - inside;
	}

	// Water first so roads and rails can cross it.
	if (strip_a_kind >= 4) {
		apply_strip(s, p, strip_a, strip_a_width, strip_a_kind, strip_a_range);
	}
	if (strip_b_kind >= 4) {
		apply_strip(s, p, strip_b, strip_b_width, strip_b_kind, strip_b_range);
	}
	if (strip_c_kind >= 4) {
		apply_strip(s, p, strip_c, strip_c_width, strip_c_kind, strip_c_range);
	}
	if (strip_a_kind < 4) {
		apply_strip(s, p, strip_a, strip_a_width, strip_a_kind, strip_a_range);
	}
	if (strip_b_kind < 4) {
		apply_strip(s, p, strip_b, strip_b_width, strip_b_kind, strip_b_range);
	}
	if (strip_c_kind < 4) {
		apply_strip(s, p, strip_c, strip_c_width, strip_c_kind, strip_c_range);
	}

	ALBEDO = s.col;
	ROUGHNESS = s.rough;
	METALLIC = 0.0;
	SPECULAR = mix(0.25, 0.6, s.water);

	// Cheap relief so low sun angles pick out furrows and hedges.
	float near_k = 1.0 - smoothstep(40.0, 260.0, dist);
	vec3 nmap = vec3(0.0, 0.0, 1.0);
	if (near_k > 0.0) {
		float e = 0.35;
		float h0 = vnoise(p * 0.9) + 0.5 * vnoise(p * 2.7);
		float hx = vnoise((p + vec2(e, 0.0)) * 0.9) + 0.5 * vnoise((p + vec2(e, 0.0)) * 2.7);
		float hz = vnoise((p + vec2(0.0, e)) * 0.9) + 0.5 * vnoise((p + vec2(0.0, e)) * 2.7);
		nmap = normalize(vec3((h0 - hx) * 0.55 * near_k, (h0 - hz) * 0.55 * near_k, 1.0));
	}
	float ripple = vnoise(p * 0.6 + TIME * 0.4) - 0.5;
	float ripple2 = vnoise(p * 0.6 + vec2(3.1, 7.7) - TIME * 0.3) - 0.5;
	nmap = mix(nmap, normalize(vec3(ripple * 0.10, ripple2 * 0.10, 1.0)), s.water);
	NORMAL_MAP = nmap * 0.5 + 0.5;
	NORMAL_MAP_DEPTH = 1.0;
}
)GLSL";

static const char *SKY = R"GLSL(
shader_type sky;

uniform vec3 zenith_color : source_color = vec3(0.10, 0.28, 0.62);
uniform vec3 horizon_color : source_color = vec3(0.62, 0.76, 0.90);
uniform vec3 ground_color : source_color = vec3(0.25, 0.30, 0.24);
uniform vec3 sun_tint : source_color = vec3(1.0, 0.92, 0.8);
uniform vec3 cloud_lit : source_color = vec3(1.0, 0.98, 0.95);
uniform vec3 cloud_shade : source_color = vec3(0.55, 0.62, 0.74);
uniform float cloud_cover = 0.45;
uniform float cloud_scale = 0.35;
uniform vec2 cloud_offset = vec2(0.0);

float hash12(vec2 p) {
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}
float vnoise(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash12(i), hash12(i + vec2(1.0, 0.0)), u.x),
			mix(hash12(i + vec2(0.0, 1.0)), hash12(i + vec2(1.0, 1.0)), u.x), u.y);
}
float fbm(vec2 p) {
	float v = 0.0;
	float a = 0.5;
	mat2 m = mat2(vec2(1.6, 1.2), vec2(-1.2, 1.6));
	for (int i = 0; i < 6; i++) {
		v += a * vnoise(p);
		p = m * p;
		a *= 0.5;
	}
	return v;
}

void sky() {
	vec3 dir = normalize(EYEDIR);
	float h = dir.y;
	vec3 col = mix(horizon_color, zenith_color, pow(clamp(h, 0.0, 1.0), 0.42));

	float sd = LIGHT0_ENABLED ? max(dot(dir, LIGHT0_DIRECTION), 0.0) : 0.0;
	// Warm scattering around the sun, strongest at the horizon.
	col += sun_tint * pow(sd, 6.0) * 0.22 * (1.0 - clamp(h, 0.0, 1.0) * 0.6);
	col += sun_tint * pow(sd, 48.0) * 0.35;

	if (h > -0.02) {
		float hh = max(h, 0.0);
		vec2 uv = dir.xz / (hh + 0.12) * cloud_scale + cloud_offset;
		float n = fbm(uv);
		float n2 = fbm(uv + LIGHT0_DIRECTION.xz * 0.06);
		float lo = 1.0 - cloud_cover;
		float c = smoothstep(lo, lo + 0.22, n);
		float lit = clamp((n - n2) * 5.0 + 0.55, 0.0, 1.0);
		vec3 ccol = mix(cloud_shade, cloud_lit, lit);
		ccol += sun_tint * pow(sd, 10.0) * 0.5;
		// Thin wispy layer high up.
		float wisp = smoothstep(0.55, 0.9, fbm(uv * vec2(0.35, 1.4) + 13.0)) * 0.35;
		c = clamp(c + wisp * (1.0 - c), 0.0, 1.0);
		c *= smoothstep(0.02, 0.2, h);
		ccol = mix(horizon_color, ccol, smoothstep(0.0, 0.3, h) * 0.8 + 0.2);
		col = mix(col, ccol, c * 0.92);
	}
	// Haze band hugging the horizon so the ground fog meets the sky.
	col = mix(col, horizon_color, 1.0 - smoothstep(0.0, 0.07, abs(h)));
	if (h < 0.0) {
		col = mix(col, ground_color, smoothstep(0.0, -0.12, h));
	}

	if (LIGHT0_ENABLED) {
		float disc = smoothstep(0.99985, 0.99995, dot(dir, LIGHT0_DIRECTION));
		col += LIGHT0_COLOR * disc * 12.0;
	}
	COLOR = col;
}
)GLSL";

static const char *CLOUD = R"GLSL(
shader_type spatial;
render_mode cull_back, shadows_disabled, specular_disabled;

uniform vec3 lit_color : source_color = vec3(1.0, 0.98, 0.95);
uniform vec3 shade_color : source_color = vec3(0.62, 0.68, 0.80);
uniform float fade_near = 25.0;

varying vec3 wpos;
varying vec3 wnrm;

float hash13(vec3 p3) {
	p3 = fract(p3 * 0.1031);
	p3 += dot(p3, p3.zyx + 31.32);
	return fract((p3.x + p3.y) * p3.z);
}
float vnoise3(vec3 p) {
	vec3 i = floor(p);
	vec3 f = fract(p);
	vec3 u = f * f * (3.0 - 2.0 * f);
	float a = mix(mix(hash13(i), hash13(i + vec3(1, 0, 0)), u.x),
			mix(hash13(i + vec3(0, 1, 0)), hash13(i + vec3(1, 1, 0)), u.x), u.y);
	float b = mix(mix(hash13(i + vec3(0, 0, 1)), hash13(i + vec3(1, 0, 1)), u.x),
			mix(hash13(i + vec3(0, 1, 1)), hash13(i + vec3(1, 1, 1)), u.x), u.y);
	return mix(a, b, u.z);
}

void vertex() {
	vec3 w = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	float n = vnoise3(w * 0.035) + 0.5 * vnoise3(w * 0.09);
	VERTEX += NORMAL * (n - 0.75) * 0.35;
	wpos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	wnrm = normalize((MODEL_MATRIX * vec4(NORMAL, 0.0)).xyz);
}

void fragment() {
	float up = wnrm.y * 0.5 + 0.5;
	vec3 col = mix(shade_color, lit_color, smoothstep(0.15, 0.85, up));
	ALBEDO = col * 0.35;
	EMISSION = col * 0.55;
	ROUGHNESS = 1.0;
	// Dither away when the camera flies through.
	float d = length(wpos - CAMERA_POSITION_WORLD);
	float fade = smoothstep(fade_near * 0.3, fade_near, d);
	float rim = abs(dot(normalize(NORMAL), normalize(VIEW)));
	fade *= smoothstep(0.0, 0.35, rim) * 0.6 + 0.4;
	float dither = fract(52.9829189 * fract(dot(FRAGCOORD.xy, vec2(0.06711056, 0.00583715))));
	if (fade < dither) {
		discard;
	}
}
)GLSL";

static const char *PAPER = R"GLSL(
shader_type canvas_item;

uniform vec3 paper_color : source_color = vec3(0.90, 0.84, 0.70);
uniform vec3 stain_color : source_color = vec3(0.55, 0.40, 0.22);
uniform float lines = 1.0;
uniform float line_spacing = 38.0;
uniform float line_offset = 120.0;
uniform vec2 rect_size = vec2(800.0, 1000.0);

float hash12(vec2 p) {
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}
float vnoise(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash12(i), hash12(i + vec2(1.0, 0.0)), u.x),
			mix(hash12(i + vec2(0.0, 1.0)), hash12(i + vec2(1.0, 1.0)), u.x), u.y);
}
float fbm(vec2 p) {
	float v = 0.0;
	float a = 0.5;
	for (int i = 0; i < 5; i++) {
		v += a * vnoise(p);
		p = p * 2.03 + 17.0;
		a *= 0.5;
	}
	return v;
}

void fragment() {
	vec2 px = UV * rect_size;
	vec3 col = paper_color;
	float grain = fbm(px * 0.35);
	col *= 0.93 + 0.12 * grain;
	float fibre = vnoise(px * vec2(0.9, 0.08)) * 0.04;
	col -= fibre;
	float stain = smoothstep(0.55, 0.8, fbm(px * 0.006 + 5.0));
	col = mix(col, stain_color, stain * 0.22);
	// Aged, darker edges.
	vec2 e = min(UV, 1.0 - UV) * rect_size;
	float edge = 1.0 - smoothstep(0.0, 70.0, min(e.x, e.y) + (grain - 0.5) * 40.0);
	col = mix(col, stain_color * 0.8, edge * 0.45);
	if (lines > 0.5 && px.y > line_offset) {
		float l = abs(fract((px.y - line_offset) / line_spacing) - 0.5) * line_spacing;
		col = mix(col, vec3(0.35, 0.45, 0.6), (1.0 - smoothstep(0.4, 1.2, l)) * 0.35);
		float margin = abs(px.x - 78.0);
		col = mix(col, vec3(0.7, 0.3, 0.3), (1.0 - smoothstep(0.4, 1.4, margin)) * 0.4);
	}
	COLOR = vec4(col, 1.0);
}
)GLSL";

static const char *VIGNETTE = R"GLSL(
shader_type canvas_item;

uniform float strength = 0.55;
uniform vec3 tint : source_color = vec3(0.0);
uniform float hurt = 0.0;

void fragment() {
	vec2 d = UV - 0.5;
	float v = smoothstep(0.35, 0.95, length(d) * 1.25);
	vec3 col = mix(tint, vec3(0.6, 0.0, 0.0), hurt);
	COLOR = vec4(col, clamp(v * (strength + hurt * 0.6), 0.0, 1.0));
}
)GLSL";

} // namespace shaders
} // namespace ww2
