#include "game/main.h"

#include "game/game_state.h"
#include "screens/screens.h"
#include "screens/viewer.h"

using namespace godot;

namespace ww2 {

// Implemented in missions/mission_factory.cpp.
Node *create_mission(Main *main);

namespace {
template <class T>
Node *make(Main *main) {
	T *s = memnew(T);
	s->set_main(main);
	return s;
}
} // namespace

Node *create_screen(int id, Main *main) {
	switch (id) {
		case SCREEN_MENU:
			return make<MenuScreen>(main);
		case SCREEN_NEW_PILOT:
			return make<NewPilotScreen>(main);
		case SCREEN_DIARY:
			return make<DiaryScreen>(main);
		case SCREEN_BRIEFING:
			return make<BriefingScreen>(main);
		case SCREEN_DEBRIEF:
			return make<DebriefScreen>(main);
		case SCREEN_MEMORIAL:
			return make<MemorialScreen>(main);
		case SCREEN_ROSTER:
			return make<RosterScreen>(main);
		case SCREEN_ENDING:
			return make<EndingScreen>(main);
		case SCREEN_MISSION:
			return create_mission(main);
		case SCREEN_VIEWER:
			return memnew(Viewer);
		default:
			return nullptr;
	}
}

} // namespace ww2
