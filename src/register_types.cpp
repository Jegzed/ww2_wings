#include "register_types.h"

#include "core/audio.h"
#include "core/materials.h"
#include "core/models.h"
#include "core/ui.h"
#include "game/game_state.h"
#include "game/main.h"
#include "missions/bombing.h"
#include "missions/dogfight.h"
#include "missions/mission_base.h"
#include "missions/strafing.h"
#include "screens/backdrop.h"
#include "screens/screens.h"
#include "screens/viewer.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;
using namespace ww2;

void initialize_ww2wings_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(Main);
	GDREGISTER_CLASS(Canvas);
	GDREGISTER_CLASS(Viewer);
	GDREGISTER_CLASS(Backdrop);

	GDREGISTER_CLASS(ScreenBase);
	GDREGISTER_CLASS(MenuScreen);
	GDREGISTER_CLASS(NewPilotScreen);
	GDREGISTER_CLASS(DiaryScreen);
	GDREGISTER_CLASS(BriefingScreen);
	GDREGISTER_CLASS(DebriefScreen);
	GDREGISTER_CLASS(MemorialScreen);
	GDREGISTER_CLASS(RosterScreen);
	GDREGISTER_CLASS(EndingScreen);

	GDREGISTER_CLASS(PauseMenu);
	GDREGISTER_CLASS(MissionBase);
	GDREGISTER_CLASS(Dogfight);
	GDREGISTER_CLASS(Bombing);
	GDREGISTER_CLASS(Strafing);
}

void uninitialize_ww2wings_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	// Drop cached engine resources before the engine shuts down.
	audio::clear_cache();
	models::clear_cache();
	mats::clear_cache();
	ui::clear_cache();
	GameState::destroy();
}

extern "C" {
GDExtensionBool GDE_EXPORT ww2wings_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
		const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_ww2wings_module);
	init_obj.register_terminator(uninitialize_ww2wings_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
