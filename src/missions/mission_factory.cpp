#include "game/game_state.h"
#include "game/main.h"
#include "missions/bombing.h"
#include "missions/dogfight.h"
#include "missions/strafing.h"

using namespace godot;

namespace ww2 {

Node *create_mission(Main *main) {
	MissionBase *m = nullptr;
	switch (GameState::get().current_mission().type) {
		case MISSION_BOMBING:
			m = memnew(Bombing);
			break;
		case MISSION_STRAFING:
			m = memnew(Strafing);
			break;
		case MISSION_DOGFIGHT:
		default:
			m = memnew(Dogfight);
			break;
	}
	m->set_main(main);
	return m;
}

} // namespace ww2
