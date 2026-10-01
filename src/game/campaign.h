#pragma once

#include "core/world_env.h"

#include <vector>

namespace ww2 {

enum MissionType {
	MISSION_DOGFIGHT,
	MISSION_BOMBING,
	MISSION_STRAFING,
};

// Air combat variants.
enum DogfightVariant {
	DF_SWEEP, // fighters only
	DF_ESCORT, // protect a box of B-26 Marauders to their target
	DF_INTERCEPT, // stop Ju 88 bombers before they reach the beachhead
	DF_DIVER, // chase V-1 flying bombs
	DF_AMBUSH, // bandits start on your tail
	DF_ACE, // one expert opponent
	DF_VARIANT_COUNT,
};

// Bombing variants.
enum BombingVariant {
	BM_RAILYARD,
	BM_BRIDGE,
	BM_CONVOY, // moving column, lead the marker
	BM_HARBOUR, // barges and quays
	BM_LAUNCH_SITE, // V-1 ramp under heavy flak
	BM_AIRFIELD,
	BM_VARIANT_COUNT,
};

// Ground attack variants.
enum StrafingVariant {
	ST_CONVOY,
	ST_AIRFIELD,
	ST_TRAIN, // moving train, stop the locomotive
	ST_BEACH, // coastal battery among bunkers
	ST_BARGES, // river traffic
	ST_VILLAGE, // headquarters in a village
	ST_VARIANT_COUNT,
};

struct MissionDef {
	MissionType type;
	int variant; // one of the enums above
	const char *date;
	const char *title;
	const char *location;
	const char *diary; // journal entry shown before the mission
	const char *briefing;
	const char *objective;
	TimeOfDay time;
	float cloud_cover; // 0..1
	int difficulty; // 1..5
	float map_x, map_y; // target on the briefing map (0..1 of the map)
	bool from_normandy; // base is the forward strip rather than Kent
};

const std::vector<MissionDef> &campaign();

// A one-off mission of the given type with a random variant and weather.
MissionDef random_mission(MissionType type, int variant, int difficulty);

const char *mission_type_name(MissionType type);
const char *variant_name(MissionType type, int variant);
const char *rank_name(int rank);
int rank_for_score(int score);

} // namespace ww2
