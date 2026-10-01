#pragma once

#include "game/campaign.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <vector>

namespace ww2 {

enum Skill {
	SKILL_FLYING,
	SKILL_SHOOTING,
	SKILL_MECHANICAL,
	SKILL_STAMINA,
	SKILL_COUNT,
};

const char *skill_name(int skill);
const char *skill_blurb(int skill);

struct Pilot {
	String name = "James Carter";
	String hometown = "Dayton, Ohio";
	int skills[SKILL_COUNT] = { 4, 4, 4, 4 };
	int air_kills = 0;
	int ground_kills = 0;
	int missions = 0;
	int score = 0;
	int rank = 0;
	int wounds = 0;
	godot::PackedStringArray medals;

	bool has_medal(const String &m) const { return medals.has(m); }
};

struct Fallen {
	String name;
	String rank;
	String date;
	String cause;
	int air_kills = 0;
	int missions = 0;
};

struct MissionResult {
	bool valid = false;
	MissionType type = MISSION_DOGFIGHT;
	bool success = false;
	bool shot_down = false;
	bool killed = false;
	bool wounded = false;
	bool aborted = false;
	int air_kills = 0;
	int ground_kills = 0;
	int targets_destroyed = 0;
	int targets_total = 0;
	int shots_fired = 0;
	int shots_hit = 0;
	int score = 0;
	float duration = 0.0f;
	String cause;

	// Filled in when the result is applied to the pilot.
	godot::PackedStringArray new_medals;
	bool promoted = false;
	int skill_gained = -1;
};

class GameState {
public:
	static GameState &get();
	static void destroy();

	Pilot pilot;
	std::vector<Fallen> fallen;
	int mission_index = 0;
	bool has_pilot = false;
	MissionResult last;

	// Automation: fly the missions with an autopilot (used for screenshots).
	bool demo = false;
	// Play a single mission from the menu without touching the campaign.
	bool instant_action = false;
	MissionDef instant = MissionDef(); // the one-off mission being flown

	const MissionDef &current_mission() const;
	bool campaign_complete() const;

	void new_campaign();
	void set_pilot(const Pilot &p);
	// Applies a mission result: kills, score, medals, promotion, death.
	void apply_result(MissionResult &result);
	void advance();

	bool has_save() const;
	void save() const;
	bool load();
	void erase_save();

	// Skill-derived gameplay multipliers.
	float handling() const; // ~0.9 .. 1.2
	float accuracy() const; // spread multiplier, lower is better
	float toughness() const; // damage taken multiplier
	float survival_chance() const;
};

} // namespace ww2
