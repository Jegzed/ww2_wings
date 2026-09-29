#include "core/audio.h"

#include <godot_cpp/variant/packed_byte_array.hpp>

#include <map>
#include <string>
#include <vector>

using namespace godot;

namespace ww2 {
namespace audio {

namespace {

const int RATE = 22050;

struct State {
	std::map<std::string, Ref<AudioStreamWAV>> streams;
	Node *host = nullptr;
	std::vector<AudioStreamPlayer *> pool;
	size_t next = 0;
	AudioStreamPlayer *music = nullptr;
	std::string music_name;
	std::string music_pending;
	float music_volume = -8.0f;
	float music_fade = 1.0f; // 0..1 multiplier
	bool music_fading_out = false;
};
State *state = nullptr;

State &st() {
	if (!state) {
		state = new State();
	}
	return *state;
}

typedef std::vector<float> Buf;

Buf make_buf(float seconds) {
	return Buf((size_t)(seconds * RATE), 0.0f);
}

void lowpass(Buf &b, float cutoff_hz) {
	float rc = 1.0f / (TAU_F * cutoff_hz);
	float dt = 1.0f / RATE;
	float a = dt / (rc + dt);
	float y = 0.0f;
	for (float &s : b) {
		y += a * (s - y);
		s = y;
	}
}

void highpass(Buf &b, float cutoff_hz) {
	Buf low = b;
	lowpass(low, cutoff_hz);
	for (size_t i = 0; i < b.size(); i++) {
		b[i] -= low[i];
	}
}

void normalize(Buf &b, float peak) {
	float m = 0.0001f;
	for (float s : b) {
		m = MAX(m, std::fabs(s));
	}
	float k = peak / m;
	for (float &s : b) {
		s *= k;
	}
}

// Blend the tail into the head so a noisy buffer loops without a click.
void make_loopable(Buf &b, float seconds) {
	size_t n = (size_t)(seconds * RATE);
	if (n * 2 >= b.size()) {
		return;
	}
	size_t len = b.size() - n;
	for (size_t i = 0; i < n; i++) {
		float t = (float)i / (float)n;
		b[i] = b[i] * t + b[len + i] * (1.0f - t);
	}
	b.resize(len);
}

Buf noise(float seconds, Rng &rng) {
	Buf b = make_buf(seconds);
	for (float &s : b) {
		s = rng.range(-1.0f, 1.0f);
	}
	return b;
}

Ref<AudioStreamWAV> to_stream(const Buf &b, bool loop) {
	PackedByteArray data;
	data.resize((int64_t)b.size() * 2);
	uint8_t *w = data.ptrw();
	for (size_t i = 0; i < b.size(); i++) {
		int v = (int)(clampf(b[i], -1.0f, 1.0f) * 32767.0f);
		w[i * 2] = (uint8_t)(v & 0xff);
		w[i * 2 + 1] = (uint8_t)((v >> 8) & 0xff);
	}
	Ref<AudioStreamWAV> s;
	s.instantiate();
	s->set_format(AudioStreamWAV::FORMAT_16_BITS);
	s->set_mix_rate(RATE);
	s->set_stereo(false);
	s->set_data(data);
	if (loop) {
		s->set_loop_mode(AudioStreamWAV::LOOP_FORWARD);
		s->set_loop_begin(0);
		s->set_loop_end((int)b.size());
	}
	return s;
}

Buf synth_engine(float f0, float rumble, uint64_t seed) {
	Rng rng(seed);
	const float len = 2.0f;
	Buf b = make_buf(len);
	float phases[16];
	for (float &p : phases) {
		p = rng.range(0, TAU_F);
	}
	for (size_t i = 0; i < b.size(); i++) {
		float t = (float)i / RATE;
		float v = 0.0f;
		for (int k = 1; k <= 14; k++) {
			float amp = 1.0f / std::pow((float)k, 0.85f);
			if (k % 2 == 0) {
				amp *= 0.7f;
			}
			v += std::sin(TAU_F * f0 * k * t + phases[k]) * amp;
		}
		// Firing-order lope and prop beat.
		float lope = 0.72f + 0.28f * std::sin(TAU_F * f0 * 0.5f * t);
		float beat = 0.85f + 0.15f * std::sin(TAU_F * 4.0f * t);
		b[i] = v * lope * beat;
	}
	Buf n = noise(len + 0.25f, rng);
	lowpass(n, 700.0f);
	lowpass(n, 700.0f);
	make_loopable(n, 0.25f);
	for (size_t i = 0; i < b.size() && i < n.size(); i++) {
		b[i] = b[i] * 0.2f + n[i] * rumble;
	}
	lowpass(b, 1800.0f);
	normalize(b, 0.8f);
	return b;
}

Buf synth_wind(uint64_t seed) {
	Rng rng(seed);
	Buf n = noise(3.4f, rng);
	lowpass(n, 900.0f);
	highpass(n, 120.0f);
	for (size_t i = 0; i < n.size(); i++) {
		float t = (float)i / RATE;
		n[i] *= 0.7f + 0.3f * std::sin(TAU_F * 0.33f * t);
	}
	make_loopable(n, 0.4f);
	normalize(n, 0.7f);
	return n;
}

Buf synth_gun(float rate_hz, float body_hz, float seconds, uint64_t seed) {
	Rng rng(seed);
	Buf b = make_buf(seconds);
	int shots = (int)std::round(rate_hz * seconds);
	float period = seconds / shots;
	Buf n = noise(seconds, rng);
	for (size_t i = 0; i < b.size(); i++) {
		float t = (float)i / RATE;
		float local = std::fmod(t, period);
		float env = std::exp(-local * 38.0f);
		float thump = std::sin(TAU_F * body_hz * local * (1.0f - local * 1.5f)) * std::exp(-local * 22.0f);
		b[i] = n[i] * env * 0.9f + thump * 0.9f;
	}
	lowpass(b, 3800.0f);
	normalize(b, 0.85f);
	return b;
}

Buf synth_explosion(float seconds, float brightness, uint64_t seed) {
	Rng rng(seed);
	Buf n = noise(seconds, rng);
	Buf b = make_buf(seconds);
	// Time-varying low-pass: bright crack that decays to a rumble.
	float y = 0.0f;
	float y2 = 0.0f;
	for (size_t i = 0; i < b.size(); i++) {
		float t = (float)i / RATE;
		float cutoff = 80.0f + brightness * 5000.0f * std::exp(-t * 7.0f);
		float a = 1.0f - std::exp(-TAU_F * cutoff / RATE);
		y += a * (n[i] - y);
		y2 += a * (y - y2);
		float env = std::exp(-t * (3.2f / seconds * 1.6f)) * smooth01(t * 400.0f);
		float sub = std::sin(TAU_F * (62.0f - 30.0f * saturate(t / seconds)) * t) * std::exp(-t * 3.0f);
		b[i] = (y2 * 2.4f + sub * 0.8f) * env;
	}
	// Soft clip for weight.
	for (float &s : b) {
		s = std::tanh(s * 2.2f);
	}
	size_t fade = (size_t)(0.15f * RATE);
	for (size_t i = 0; i < fade && i < b.size(); i++) {
		b[b.size() - 1 - i] *= (float)i / fade;
	}
	normalize(b, 0.95f);
	return b;
}

Buf synth_hit(uint64_t seed, bool ricochet) {
	Rng rng(seed);
	float len = ricochet ? 0.45f : 0.16f;
	Buf b = make_buf(len);
	float freqs[3] = { rng.range(900, 1400), rng.range(1700, 2300), rng.range(2600, 3400) };
	for (size_t i = 0; i < b.size(); i++) {
		float t = (float)i / RATE;
		float v = rng.range(-1, 1) * std::exp(-t * 90.0f) * 0.9f;
		for (int k = 0; k < 3; k++) {
			float f = freqs[k];
			if (ricochet) {
				f *= 1.0f + 0.6f * std::exp(-t * 6.0f);
			}
			v += std::sin(TAU_F * f * t) * std::exp(-t * (ricochet ? 9.0f : 30.0f)) * 0.35f;
		}
		b[i] = v;
	}
	normalize(b, 0.7f);
	return b;
}

Buf synth_whistle() {
	float len = 2.4f;
	Buf b = make_buf(len);
	float phase = 0.0f;
	for (size_t i = 0; i < b.size(); i++) {
		float t = (float)i / RATE;
		float k = t / len;
		float f = lerpf(2100.0f, 650.0f, k * k) * (1.0f + 0.01f * std::sin(TAU_F * 7.0f * t));
		phase += TAU_F * f / RATE;
		float env = smooth01(k * 6.0f) * smooth01((1.0f - k) * 12.0f);
		b[i] = (std::sin(phase) + 0.3f * std::sin(phase * 2.0f)) * env * (0.3f + 0.7f * k);
	}
	normalize(b, 0.5f);
	return b;
}

Buf synth_click(float pitch, float len, uint64_t seed) {
	Rng rng(seed);
	Buf b = make_buf(len);
	for (size_t i = 0; i < b.size(); i++) {
		float t = (float)i / RATE;
		b[i] = (rng.range(-1, 1) * 0.5f + std::sin(TAU_F * pitch * t)) * std::exp(-t * 70.0f);
	}
	lowpass(b, 5000.0f);
	normalize(b, 0.6f);
	return b;
}

Buf synth_page(uint64_t seed) {
	Rng rng(seed);
	Buf n = noise(0.42f, rng);
	highpass(n, 1500.0f);
	lowpass(n, 6000.0f);
	for (size_t i = 0; i < n.size(); i++) {
		float k = (float)i / n.size();
		float env = std::sin(k * PI_F);
		env *= 0.6f + 0.4f * std::sin(k * 40.0f);
		n[i] *= env * env;
	}
	normalize(n, 0.5f);
	return n;
}

float note_hz(int semitones_from_a4) {
	return 440.0f * std::pow(2.0f, semitones_from_a4 / 12.0f);
}

// Warm brass-like tone with a soft attack.
void add_note(Buf &b, float start, float dur, float hz, float amp, float brightness, bool wrap) {
	size_t n0 = (size_t)(start * RATE);
	size_t n = (size_t)(dur * RATE);
	for (size_t i = 0; i < n; i++) {
		size_t idx = n0 + i;
		if (idx >= b.size()) {
			if (!wrap) {
				break;
			}
			idx %= b.size();
		}
		float t = (float)i / RATE;
		float k = (float)i / n;
		float env = smooth01(k * dur / 0.12f) * smooth01((1.0f - k) * dur / 0.35f);
		float v = 0.0f;
		for (int h = 1; h <= 7; h++) {
			float a = 1.0f / std::pow((float)h, 2.0f - brightness);
			v += std::sin(TAU_F * hz * h * t * (1.0f + 0.0007f * h)) * a;
		}
		float vib = 1.0f + 0.004f * std::sin(TAU_F * 5.0f * t) * smooth01(t * 2.0f);
		b[idx] += v * vib * env * amp;
	}
}

Buf synth_fanfare() {
	Buf b = make_buf(3.2f);
	// G C E G - E - G (bright major call)
	const int notes[] = { -14, -9, -5, -2, -5, -2 };
	const float starts[] = { 0.0f, 0.22f, 0.44f, 0.66f, 1.2f, 1.45f };
	const float durs[] = { 0.24f, 0.24f, 0.24f, 0.55f, 0.27f, 1.6f };
	for (int i = 0; i < 6; i++) {
		add_note(b, starts[i], durs[i], note_hz(notes[i]), 0.5f, 0.9f, false);
		add_note(b, starts[i], durs[i], note_hz(notes[i] - 12), 0.3f, 0.5f, false);
	}
	add_note(b, 1.45f, 1.6f, note_hz(-9), 0.3f, 0.6f, false);
	add_note(b, 1.45f, 1.6f, note_hz(-5), 0.3f, 0.6f, false);
	normalize(b, 0.7f);
	return b;
}

Buf synth_dirge() {
	Buf b = make_buf(6.0f);
	// Slow minor descent, like a last post fragment.
	const int notes[] = { -5, -9, -12, -9, -17 };
	const float starts[] = { 0.0f, 1.0f, 1.9f, 2.8f, 3.7f };
	const float durs[] = { 1.0f, 0.9f, 0.9f, 0.9f, 2.2f };
	for (int i = 0; i < 5; i++) {
		add_note(b, starts[i], durs[i], note_hz(notes[i]), 0.5f, 0.5f, false);
	}
	add_note(b, 3.7f, 2.2f, note_hz(-29), 0.35f, 0.3f, false);
	add_note(b, 3.7f, 2.2f, note_hz(-14), 0.25f, 0.3f, false);
	normalize(b, 0.6f);
	return b;
}

Buf synth_music(uint64_t seed, bool sombre) {
	Rng rng(seed);
	const float bar = 4.0f;
	const int bars = 8;
	Buf b = make_buf(bar * bars);
	// Chords as semitone offsets from A4. Dm Bb F C | Dm Bb Gm A
	const int chords[8][3] = {
		{ -19, -16, -12 }, { -23, -19, -16 }, { -16, -12, -9 }, { -21, -17, -14 },
		{ -19, -16, -12 }, { -23, -19, -16 }, { -26, -23, -19 }, { -24, -20, -17 },
	};
	const int bass[8] = { -31, -35, -28, -33, -31, -35, -38, -36 };
	for (int i = 0; i < bars; i++) {
		float start = i * bar;
		for (int k = 0; k < 3; k++) {
			for (int d = -1; d <= 1; d++) {
				float hz = note_hz(chords[i][k]) * (1.0f + d * 0.0025f);
				add_note(b, start - 0.3f + (start < 0.3f ? bar * bars : 0.0f), bar + 0.9f, hz, 0.13f, 0.35f, true);
			}
		}
		add_note(b, start, bar, note_hz(bass[i]), 0.34f, 0.2f, true);
	}
	// Melody: a simple horn line over the pad.
	const int mel[] = { -7, -5, -4, -7, -9, -7, -12, -12, -7, -5, -4, 0, -2, -4, -5, -5 };
	const float mel_len[] = { 1.5f, 0.5f, 2.0f, 1.5f, 0.5f, 2.0f, 3.0f, 1.0f, 1.5f, 0.5f, 2.0f, 1.5f, 0.5f, 2.0f, 3.0f, 1.0f };
	float t = 0.0f;
	for (int i = 0; i < 16; i++) {
		bool rest = (i == 7 || i == 15);
		if (!rest) {
			add_note(b, t, mel_len[i] * 1.05f, note_hz(mel[i] - (sombre ? 12 : 0)), 0.33f, 0.75f, true);
		}
		t += mel_len[i];
	}
	// Distant snare roll on the bar lines.
	if (!sombre) {
		for (int i = 0; i < bars * 4; i++) {
			float s = i * 1.0f;
			int taps = (i % 4 == 3) ? 4 : 1;
			for (int k = 0; k < taps; k++) {
				size_t n0 = (size_t)((s + k * 0.25f) * RATE);
				for (size_t j = 0; j < (size_t)(0.12f * RATE) && n0 + j < b.size(); j++) {
					float tt = (float)j / RATE;
					b[n0 + j] += rng.range(-1, 1) * std::exp(-tt * 45.0f) * 0.10f;
				}
			}
		}
	}
	lowpass(b, 3200.0f);
	normalize(b, 0.75f);
	return b;
}

Buf synth_alarm() {
	Buf b = make_buf(0.5f);
	for (size_t i = 0; i < b.size(); i++) {
		float t = (float)i / RATE;
		float env = t < 0.22f ? 1.0f : 0.0f;
		b[i] = (std::sin(TAU_F * 880.0f * t) > 0 ? 0.4f : -0.4f) * env;
	}
	lowpass(b, 2500.0f);
	return b;
}

Ref<AudioStreamWAV> build(const std::string &n) {
	if (n == "engine") {
		return to_stream(synth_engine(55.0f, 0.55f, 11), true);
	} else if (n == "engine_enemy") {
		return to_stream(synth_engine(74.0f, 0.4f, 12), true);
	} else if (n == "wind") {
		return to_stream(synth_wind(13), true);
	} else if (n == "gun") {
		return to_stream(synth_gun(14.0f, 110.0f, 0.5f, 14), true);
	} else if (n == "cannon") {
		return to_stream(synth_gun(8.0f, 70.0f, 0.5f, 15), true);
	} else if (n == "flak_gun") {
		// A single report, for one-shot use (the "cannon" stream loops).
		return to_stream(synth_gun(8.0f, 70.0f, 0.125f, 27), false);
	} else if (n == "explosion") {
		return to_stream(synth_explosion(2.4f, 0.8f, 16), false);
	} else if (n == "explosion_big") {
		return to_stream(synth_explosion(3.6f, 0.5f, 17), false);
	} else if (n == "flak") {
		return to_stream(synth_explosion(0.9f, 1.0f, 18), false);
	} else if (n == "hit") {
		return to_stream(synth_hit(19, false), false);
	} else if (n == "ricochet") {
		return to_stream(synth_hit(20, true), false);
	} else if (n == "whistle") {
		return to_stream(synth_whistle(), false);
	} else if (n == "bomb_release") {
		return to_stream(synth_click(180.0f, 0.18f, 21), false);
	} else if (n == "click") {
		return to_stream(synth_click(900.0f, 0.06f, 22), false);
	} else if (n == "type") {
		return to_stream(synth_click(2200.0f, 0.04f, 23), false);
	} else if (n == "page") {
		return to_stream(synth_page(24), false);
	} else if (n == "fanfare") {
		return to_stream(synth_fanfare(), false);
	} else if (n == "dirge") {
		return to_stream(synth_dirge(), false);
	} else if (n == "music_menu") {
		return to_stream(synth_music(25, false), true);
	} else if (n == "music_diary") {
		return to_stream(synth_music(26, true), true);
	} else if (n == "alarm") {
		return to_stream(synth_alarm(), true);
	}
	return to_stream(synth_click(600.0f, 0.05f, 1), false);
}

} // namespace

Ref<AudioStreamWAV> stream(const String &name) {
	std::string key = name.utf8().get_data();
	auto it = st().streams.find(key);
	if (it != st().streams.end()) {
		return it->second;
	}
	Ref<AudioStreamWAV> s = build(key);
	s->set_name(name);
	st().streams[key] = s;
	return s;
}

void set_host(Node *host) {
	State &s = st();
	s.pool.clear();
	s.music = nullptr;
	s.music_name.clear();
	s.music_pending.clear();
	s.host = host;
	if (!host) {
		return;
	}
	for (int i = 0; i < 20; i++) {
		AudioStreamPlayer *p = memnew(AudioStreamPlayer);
		host->add_child(p);
		s.pool.push_back(p);
	}
	s.music = memnew(AudioStreamPlayer);
	host->add_child(s.music);
}

void play(const String &name, float volume_db, float pitch) {
	State &s = st();
	if (s.pool.empty()) {
		return;
	}
	AudioStreamPlayer *chosen = nullptr;
	for (size_t i = 0; i < s.pool.size(); i++) {
		AudioStreamPlayer *p = s.pool[(s.next + i) % s.pool.size()];
		if (!p->is_playing()) {
			chosen = p;
			s.next = (s.next + i + 1) % s.pool.size();
			break;
		}
	}
	if (!chosen) {
		chosen = s.pool[s.next];
		s.next = (s.next + 1) % s.pool.size();
	}
	Ref<AudioStreamWAV> wav = stream(name);
	if (wav->get_loop_mode() != AudioStreamWAV::LOOP_DISABLED) {
		// A looping stream on a fire-and-forget player would never stop.
		ERR_PRINT(String("audio::play called with looping sound: ") + name);
		return;
	}
	chosen->set_stream(wav);
	chosen->set_volume_db(volume_db);
	chosen->set_pitch_scale(MAX(pitch, 0.05f));
	chosen->play();
}

AudioStreamPlayer *make_loop(Node *parent, const String &name, float volume_db) {
	AudioStreamPlayer *p = memnew(AudioStreamPlayer);
	p->set_stream(stream(name));
	p->set_volume_db(volume_db);
	parent->add_child(p);
	return p;
}

void play_music(const String &name, float volume_db) {
	State &s = st();
	if (!s.music) {
		return;
	}
	std::string key = name.utf8().get_data();
	s.music_volume = volume_db;
	if (s.music_name == key && s.music->is_playing() && !s.music_fading_out) {
		return;
	}
	if (s.music->is_playing()) {
		s.music_pending = key;
		s.music_fading_out = true;
	} else {
		s.music_name = key;
		s.music_pending.clear();
		s.music_fading_out = false;
		s.music_fade = 1.0f;
		s.music->set_stream(stream(name));
		s.music->set_volume_db(volume_db);
		s.music->play();
	}
}

void stop_music() {
	State &s = st();
	if (s.music && s.music->is_playing()) {
		s.music_pending.clear();
		s.music_fading_out = true;
	}
}

void update(float dt) {
	State &s = st();
	if (!s.music) {
		return;
	}
	if (s.music_fading_out) {
		s.music_fade -= dt * 1.2f;
		if (s.music_fade <= 0.0f) {
			s.music->stop();
			s.music_fading_out = false;
			s.music_fade = 1.0f;
			s.music_name.clear();
			if (!s.music_pending.empty()) {
				std::string next = s.music_pending;
				s.music_pending.clear();
				play_music(String(next.c_str()), s.music_volume);
			}
			return;
		}
		s.music->set_volume_db(s.music_volume + (1.0f - s.music_fade) * -40.0f);
	}
}

void shutdown() {
	State &s = st();
	for (AudioStreamPlayer *p : s.pool) {
		p->stop();
		p->set_stream(Ref<AudioStreamWAV>());
	}
	if (s.music) {
		s.music->stop();
		s.music->set_stream(Ref<AudioStreamWAV>());
	}
}

void clear_cache() {
	delete state;
	state = nullptr;
}

} // namespace audio
} // namespace ww2
