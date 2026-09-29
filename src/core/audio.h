#pragma once

#include "core/util.h"

#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/classes/node.hpp>

namespace ww2 {
namespace audio {

using godot::AudioStreamPlayer;
using godot::AudioStreamWAV;
using godot::Node;
using godot::Ref;

// All sounds are synthesised on first use. Known names:
// engine, engine_enemy, wind, gun, cannon, explosion, flak, hit, ricochet, whistle,
// bomb_release, click, page, type, fanfare, dirge, music_menu, alarm
Ref<AudioStreamWAV> stream(const String &name);

// One-shot, fire-and-forget.
void play(const String &name, float volume_db = 0.0f, float pitch = 1.0f);

// Creates a looping player owned by `parent` (not started).
AudioStreamPlayer *make_loop(Node *parent, const String &name, float volume_db);

// Music is a single persistent track that cross-fades.
void play_music(const String &name, float volume_db = -8.0f);
void stop_music();

void set_host(Node *host); // node that owns the one-shot player pool
void update(float dt);
void shutdown(); // stops everything that is playing
void clear_cache();

} // namespace audio
} // namespace ww2
