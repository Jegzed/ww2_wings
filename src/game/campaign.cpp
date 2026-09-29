#include "game/campaign.h"

namespace ww2 {

const std::vector<MissionDef> &campaign() {
	static const std::vector<MissionDef> missions = {
		{ MISSION_DOGFIGHT, "June 2nd, 1944", "Fighter Sweep", "Pas-de-Calais, France",
				"Arrived at Ashford three days ago. The 406th flies the P-47 - seven tons of airplane they call "
				"the Jug. The old hands say she will bring you home with half a wing gone. I hope I never need "
				"to find out.\n\n"
				"Major Hollis put me on the board this morning. A sweep across the Channel, my first. "
				"Mac says keep your head on a swivel and never fly straight for more than ten seconds.\n\n"
				"I wrote to Mother. Did not tell her about today.",
				"Two flights will sweep the coast between Calais and Boulogne at 8,000 feet. Intelligence "
				"reports a Staffel of Bf 109s operating from Abbeville. They WILL come up to meet you.\n\n"
				"Stay with your wingman. Get on their tails, fire in short bursts and watch your ammunition.",
				"Destroy the enemy fighters", TOD_MORNING, 1, 0 },
		{ MISSION_BOMBING, "June 4th, 1944", "The Rail Yard", "Amiens, France",
				"Rain all day yesterday. We played cards in the dispersal hut and listened to the armorers "
				"hanging five-hundred pounders under the wings. Something big is coming - every road in Kent "
				"is jammed with trucks and the harbors are full.\n\n"
				"Today we go after the railways. If the trains cannot run, their tanks cannot reach the coast.",
				"Target is the marshalling yard at Amiens. You carry a limited load of 500 lb bombs. "
				"Line up along the tracks and release when the target passes under your sight - the bombs "
				"take a few seconds to fall, so lead your target.\n\n"
				"Expect flak around the yard. Do not fly straight for long.",
				"Destroy rolling stock and warehouses", TOD_NOON, 1, 0 },
		{ MISSION_STRAFING, "June 6th, 1944", "D-Day", "Road to Caen, Normandy",
				"They woke us at 0300. The ground crews had been up all night painting black and white stripes "
				"on every airplane. Major Hollis said only, 'Gentlemen, this is it.'\n\n"
				"Crossing the Channel I saw more ships than I thought existed in the world. "
				"The boys on those beaches need every German truck stopped before it gets there.",
				"German reinforcements are moving up the Caen road toward the beaches. Go in on the deck "
				"and shoot up anything that moves: trucks, half-tracks, armor.\n\n"
				"Fly LOW to hit what you aim at, but mind the trees. Light flak is travelling with the column.",
				"Destroy the convoy", TOD_MORNING, 2, 0 },
		{ MISSION_DOGFIGHT, "June 10th, 1944", "Bandits over the Beachhead", "Sainte-Mere-Eglise, Normandy",
				"We are flying from a strip bulldozed out of a Norman apple orchard. Dust in everything, "
				"and the front so close we can hear the guns at night.\n\n"
				"The Luftwaffe has finally shown up in strength. Mac got his fourth yesterday. "
				"He pretends it is nothing but he painted the cross on his cowling himself.",
				"Radar has a formation inbound for the beachhead - 109s with Focke-Wulf 190s among them. "
				"The 190 rolls faster than you can. Do not try to turn with him; use your speed and your "
				"eight guns.\n\nKeep them off the beaches.",
				"Destroy the enemy fighters", TOD_DUSK, 3, 1 },
		{ MISSION_BOMBING, "June 14th, 1944", "The Bridge", "River Orne, Normandy",
				"A week of mud and flak. Lost Petersen on Monday - engine hit over Villers, he never got out. "
				"His cot was empty for one night. The replacement looks about sixteen.\n\n"
				"Today it is a bridge. Bridges are small and they shoot back.",
				"The road bridge over the Orne is the only crossing still standing in this sector. "
				"Drop it. Your run is along the river, so you will cross the bridge at an angle - "
				"pick your release carefully.\n\nFuel dumps and flak positions on the banks are secondary targets.",
				"Destroy the bridge", TOD_OVERCAST, 3, 1 },
		{ MISSION_STRAFING, "June 18th, 1944", "Airfield Attack", "Evreux, France",
				"Hollis called me in. There is a bar on my collar that was not there last week and a letter "
				"from home in my pocket. Dad has framed the newspaper clipping.\n\n"
				"Tonight the whole group goes after their airfields. If we catch them on the ground "
				"they cannot catch us in the air.",
				"Evreux airfield. Fighters are parked along the dispersal and fuel is stored by the hangars. "
				"One pass, on the deck, at full throttle.\n\n"
				"Airfield flak is the heaviest there is. Stay low, keep moving and do not go around twice.",
				"Destroy parked aircraft and fuel", TOD_DUSK, 4, 1 },
	};
	return missions;
}

const char *mission_type_name(MissionType type) {
	switch (type) {
		case MISSION_DOGFIGHT:
			return "AIR COMBAT";
		case MISSION_BOMBING:
			return "BOMBING";
		case MISSION_STRAFING:
			return "GROUND ATTACK";
	}
	return "";
}

const char *rank_name(int rank) {
	static const char *names[] = { "2nd Lieutenant", "1st Lieutenant", "Captain", "Major", "Lt. Colonel" };
	if (rank < 0) {
		rank = 0;
	}
	if (rank > 4) {
		rank = 4;
	}
	return names[rank];
}

int rank_for_score(int score) {
	if (score >= 16000) {
		return 4;
	}
	if (score >= 9000) {
		return 3;
	}
	if (score >= 4500) {
		return 2;
	}
	if (score >= 1500) {
		return 1;
	}
	return 0;
}

} // namespace ww2
