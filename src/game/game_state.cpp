#include "game/game_state.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/array.hpp>

using namespace godot;

namespace ww2 {

namespace {
GameState *instance = nullptr;
const char *SAVE_PATH = "user://career.json";
} // namespace

const char *skill_name(int skill) {
	static const char *names[] = { "Flying", "Shooting", "Mechanical", "Stamina" };
	return names[CLAMP(skill, 0, SKILL_COUNT - 1)];
}

const char *skill_blurb(int skill) {
	static const char *blurbs[] = {
		"How tightly you can turn and how well the ship answers your hand.",
		"Tighter bursts and more telling hits.",
		"Knowing your machine: guns that stay cool, an engine that takes punishment.",
		"Endurance and luck. Your odds of walking away when it all goes wrong.",
	};
	return blurbs[CLAMP(skill, 0, SKILL_COUNT - 1)];
}

GameState &GameState::get() {
	if (!instance) {
		instance = new GameState();
	}
	return *instance;
}

void GameState::destroy() {
	delete instance;
	instance = nullptr;
}

const MissionDef &GameState::current_mission() const {
	const std::vector<MissionDef> &c = campaign();
	int idx = instant_action ? instant_mission : mission_index;
	idx = CLAMP(idx, 0, (int)c.size() - 1);
	return c[idx];
}

bool GameState::campaign_complete() const {
	return mission_index >= (int)campaign().size();
}

void GameState::new_campaign() {
	pilot = Pilot();
	fallen.clear();
	mission_index = 0;
	has_pilot = false;
	last = MissionResult();
}

void GameState::set_pilot(const Pilot &p) {
	pilot = p;
	has_pilot = true;
}

void GameState::apply_result(MissionResult &r) {
	if (instant_action) {
		return;
	}
	Pilot &p = pilot;
	p.missions += 1;
	p.air_kills += r.air_kills;
	p.ground_kills += r.ground_kills;
	p.score += r.score;

	auto award = [&](const char *medal) {
		if (!p.has_medal(medal)) {
			p.medals.push_back(medal);
			r.new_medals.push_back(medal);
		}
	};

	if (r.wounded || r.killed) {
		p.wounds += 1;
		award("Purple Heart");
	}
	if (p.missions >= 3 || p.air_kills >= 1) {
		award("Air Medal");
	}
	if (p.air_kills >= 5) {
		award("Distinguished Flying Cross");
	}
	if (r.success && !r.shot_down && (r.air_kills >= 3 || (r.targets_total > 0 && r.targets_destroyed >= r.targets_total))) {
		award("Silver Star");
	}
	if (p.ground_kills >= 100) {
		award("Distinguished Unit Citation");
	}

	int new_rank = rank_for_score(p.score);
	if (new_rank > p.rank && !r.killed) {
		p.rank = new_rank;
		r.promoted = true;
	}

	// Experience: a successful sortie sharpens the skill it exercised.
	if (r.success && !r.killed) {
		int skill = SKILL_FLYING;
		if (r.type == MISSION_DOGFIGHT) {
			skill = (p.missions % 2 == 0) ? SKILL_FLYING : SKILL_SHOOTING;
		} else if (r.type == MISSION_BOMBING) {
			skill = (p.missions % 2 == 0) ? SKILL_MECHANICAL : SKILL_SHOOTING;
		} else {
			skill = (p.missions % 2 == 0) ? SKILL_STAMINA : SKILL_FLYING;
		}
		if (p.skills[skill] < 10) {
			p.skills[skill] += 1;
			r.skill_gained = skill;
		}
	}

	if (r.killed) {
		Fallen f;
		f.name = p.name;
		f.rank = rank_name(p.rank);
		f.date = current_mission().date;
		f.cause = r.cause;
		f.air_kills = p.air_kills;
		f.missions = p.missions;
		fallen.push_back(f);
		has_pilot = false;
	}
}

void GameState::advance() {
	if (instant_action) {
		return;
	}
	mission_index += 1;
}

bool GameState::has_save() const {
	return FileAccess::file_exists(SAVE_PATH);
}

void GameState::save() const {
	if (demo || instant_action) {
		return;
	}
	Dictionary d;
	d["version"] = 1;
	d["mission_index"] = mission_index;
	d["has_pilot"] = has_pilot;

	Dictionary p;
	p["name"] = pilot.name;
	p["hometown"] = pilot.hometown;
	Array skills;
	for (int i = 0; i < SKILL_COUNT; i++) {
		skills.push_back(pilot.skills[i]);
	}
	p["skills"] = skills;
	p["air_kills"] = pilot.air_kills;
	p["ground_kills"] = pilot.ground_kills;
	p["missions"] = pilot.missions;
	p["score"] = pilot.score;
	p["rank"] = pilot.rank;
	p["wounds"] = pilot.wounds;
	Array medals;
	for (int i = 0; i < pilot.medals.size(); i++) {
		medals.push_back(pilot.medals[i]);
	}
	p["medals"] = medals;
	d["pilot"] = p;

	Array dead;
	for (const Fallen &f : fallen) {
		Dictionary fd;
		fd["name"] = f.name;
		fd["rank"] = f.rank;
		fd["date"] = f.date;
		fd["cause"] = f.cause;
		fd["air_kills"] = f.air_kills;
		fd["missions"] = f.missions;
		dead.push_back(fd);
	}
	d["fallen"] = dead;

	Ref<FileAccess> f = FileAccess::open(SAVE_PATH, FileAccess::WRITE);
	if (f.is_valid()) {
		f->store_string(JSON::stringify(d, "\t"));
		f->close();
	}
}

bool GameState::load() {
	if (!has_save()) {
		return false;
	}
	Ref<FileAccess> f = FileAccess::open(SAVE_PATH, FileAccess::READ);
	if (f.is_null()) {
		return false;
	}
	Variant parsed = JSON::parse_string(f->get_as_text());
	f->close();
	if (parsed.get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary d = parsed;
	mission_index = (int)d.get("mission_index", 0);
	has_pilot = (bool)d.get("has_pilot", false);

	Dictionary p = d.get("pilot", Dictionary());
	pilot = Pilot();
	pilot.name = p.get("name", pilot.name);
	pilot.hometown = p.get("hometown", pilot.hometown);
	Array skills = p.get("skills", Array());
	for (int i = 0; i < SKILL_COUNT && i < skills.size(); i++) {
		pilot.skills[i] = CLAMP((int)skills[i], 1, 10);
	}
	pilot.air_kills = (int)p.get("air_kills", 0);
	pilot.ground_kills = (int)p.get("ground_kills", 0);
	pilot.missions = (int)p.get("missions", 0);
	pilot.score = (int)p.get("score", 0);
	pilot.rank = (int)p.get("rank", 0);
	pilot.wounds = (int)p.get("wounds", 0);
	Array medals = p.get("medals", Array());
	for (int i = 0; i < medals.size(); i++) {
		pilot.medals.push_back(String(medals[i]));
	}

	fallen.clear();
	Array dead = d.get("fallen", Array());
	for (int i = 0; i < dead.size(); i++) {
		Dictionary fd = dead[i];
		Fallen fl;
		fl.name = fd.get("name", "");
		fl.rank = fd.get("rank", "");
		fl.date = fd.get("date", "");
		fl.cause = fd.get("cause", "");
		fl.air_kills = (int)fd.get("air_kills", 0);
		fl.missions = (int)fd.get("missions", 0);
		fallen.push_back(fl);
	}
	last = MissionResult();
	return true;
}

void GameState::erase_save() {
	if (has_save()) {
		Ref<DirAccess> dir = DirAccess::open("user://");
		if (dir.is_valid()) {
			dir->remove("career.json");
		}
	}
}

float GameState::handling() const {
	return 0.88f + 0.035f * pilot.skills[SKILL_FLYING];
}

float GameState::accuracy() const {
	return 1.45f - 0.09f * pilot.skills[SKILL_SHOOTING];
}

float GameState::toughness() const {
	return 1.25f - 0.05f * pilot.skills[SKILL_MECHANICAL];
}

float GameState::survival_chance() const {
	return 0.35f + 0.055f * pilot.skills[SKILL_STAMINA];
}

} // namespace ww2
