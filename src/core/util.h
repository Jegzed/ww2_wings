#pragma once

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cmath>
#include <cstdint>

namespace ww2 {

using godot::Basis;
using godot::Color;
using godot::String;
using godot::Transform3D;
using godot::Vector2;
using godot::Vector3;

constexpr float PI_F = 3.14159265358979f;
constexpr float TAU_F = 6.28318530717958f;

inline float deg2rad(float d) { return d * PI_F / 180.0f; }
inline float rad2deg(float r) { return r * 180.0f / PI_F; }

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float saturate(float v) { return clampf(v, 0.0f, 1.0f); }
inline float smooth01(float t) {
	t = saturate(t);
	return t * t * (3.0f - 2.0f * t);
}
inline float move_toward(float v, float target, float max_delta) {
	if (std::fabs(target - v) <= max_delta) {
		return target;
	}
	return v + (target > v ? max_delta : -max_delta);
}
// Frame-rate independent exponential approach.
inline float damp(float v, float target, float rate, float dt) {
	return lerpf(v, target, 1.0f - std::exp(-rate * dt));
}
inline Vector3 damp(const Vector3 &v, const Vector3 &target, float rate, float dt) {
	return v.lerp(target, 1.0f - std::exp(-rate * dt));
}

// Small deterministic PRNG (xorshift64*), so worlds can be rebuilt from a seed.
struct Rng {
	uint64_t s;
	explicit Rng(uint64_t seed = 0x9E3779B97F4A7C15ull) :
			s(seed ? seed : 0x9E3779B97F4A7C15ull) {
		next();
		next();
	}
	uint32_t next() {
		s ^= s >> 12;
		s ^= s << 25;
		s ^= s >> 27;
		return (uint32_t)((s * 0x2545F4914F6CDD1Dull) >> 32);
	}
	float f() { return (next() >> 8) * (1.0f / 16777216.0f); }
	float range(float a, float b) { return a + (b - a) * f(); }
	int irange(int a, int b) { return a + (int)(next() % (uint32_t)(b - a + 1)); }
	bool chance(float p) { return f() < p; }
	Vector3 in_sphere() {
		for (int i = 0; i < 16; i++) {
			Vector3 v(range(-1, 1), range(-1, 1), range(-1, 1));
			if (v.length_squared() <= 1.0f) {
				return v;
			}
		}
		return Vector3();
	}
};

// Shared, time-seeded generator for gameplay randomness.
Rng &grng();

} // namespace ww2
