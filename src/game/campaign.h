#pragma once

#include "core/world_env.h"

#include <vector>

namespace ww2 {

enum MissionType {
	MISSION_DOGFIGHT,
	MISSION_BOMBING,
	MISSION_STRAFING,
};

struct MissionDef {
	MissionType type;
	const char *date;
	const char *title;
	const char *location;
	const char *diary; // journal entry shown before the mission
	const char *briefing;
	const char *objective;
	TimeOfDay time;
	int difficulty; // 1..5
	int variant; // mission-specific layout
};

const std::vector<MissionDef> &campaign();

const char *mission_type_name(MissionType type);
const char *rank_name(int rank);
int rank_for_score(int score);

} // namespace ww2
