#include "game/campaign.h"

#include "core/util.h"

#include <algorithm>

namespace {
int clamp_index(int v, int count) {
	return std::max(0, std::min(v, count - 1));
}
} // namespace

namespace ww2 {

const std::vector<MissionDef> &campaign() {
	static const std::vector<MissionDef> missions = {
		// ---------------------------------------------------------------- 1
		{ MISSION_DOGFIGHT, DF_SWEEP, "June 2nd, 1944", "Fighter Sweep", "Pas-de-Calais, France",
				"Arrived at Ashford three days ago. The 406th flies the P-47 - seven tons of airplane they call "
				"the Jug. The old hands say she will bring you home with half a wing gone. I hope I never need "
				"to find out.\n\n"
				"Major Hollis put me on the board this morning. A sweep across the Channel, my first. "
				"Mac says keep your head on a swivel and never fly straight for more than ten seconds.\n\n"
				"I wrote to Mother. Did not tell her about today.",
				"Two flights will sweep the coast between Calais and Boulogne at 8,000 feet. Intelligence "
				"reports a Staffel of Bf 109s operating from Abbeville. They WILL come up to meet you.\n\n"
				"Stay with your wingman. Get on their tails, fire in short bursts and watch your ammunition.",
				"Destroy the enemy fighters", TOD_MORNING, 0.4f, 1, 0.82f, 0.32f, false },
		// ---------------------------------------------------------------- 2
		{ MISSION_BOMBING, BM_RAILYARD, "June 4th, 1944", "The Rail Yard", "Amiens, France",
				"Rain all day yesterday. We played cards in the dispersal hut and listened to the armorers "
				"hanging five-hundred pounders under the wings. Something big is coming - every road in Kent "
				"is jammed with trucks and the harbors are full.\n\n"
				"Today we go after the railways. If the trains cannot run, their tanks cannot reach the coast.",
				"Target is the marshalling yard at Amiens. You carry a limited load of 500 lb bombs. "
				"Line up along the tracks and release when the target passes under your sight - the bombs "
				"take a few seconds to fall, so lead your target.\n\n"
				"Expect flak around the yard. Do not fly straight for long.",
				"Destroy rolling stock and warehouses", TOD_NOON, 0.5f, 1, 0.76f, 0.62f, false },
		// ---------------------------------------------------------------- 3
		{ MISSION_STRAFING, ST_CONVOY, "June 6th, 1944", "D-Day", "Road to Caen, Normandy",
				"They woke us at 0300. The ground crews had been up all night painting black and white stripes "
				"on every airplane. Major Hollis said only, 'Gentlemen, this is it.'\n\n"
				"Crossing the Channel I saw more ships than I thought existed in the world. "
				"The boys on those beaches need every German truck stopped before it gets there.",
				"German reinforcements are moving up the Caen road toward the beaches. Go in on the deck "
				"and shoot up anything that moves: trucks, half-tracks, armor.\n\n"
				"Fly LOW to hit what you aim at, but mind the trees. Light flak is travelling with the column.",
				"Destroy the convoy", TOD_MORNING, 0.6f, 2, 0.50f, 0.66f, false },
		// ---------------------------------------------------------------- 4
		{ MISSION_DOGFIGHT, DF_ESCORT, "June 7th, 1944", "Little Friends", "Caen, Normandy",
				"Nobody slept. The beaches held, but the Germans are pushing armor toward the coast and "
				"the Marauders go after the bridges at Caen today. We ride with them.\n\n"
				"The bomber boys call us 'little friends'. Mac says the trick is to stay close - a fighter "
				"that goes chasing kills leaves the bombers naked.",
				"Escort a box of B-26 Marauders to the Orne bridges at Caen. Take station above and behind "
				"them and break up every attack. Enemy fighters will go for the bombers, not for you.\n\n"
				"The mission succeeds if the box reaches the target with most of its aircraft. "
				"Do not be drawn away - come straight back to the formation after every engagement.",
				"Bring the bombers through", TOD_NOON, 0.45f, 2, 0.46f, 0.64f, false },
		// ---------------------------------------------------------------- 5
		{ MISSION_STRAFING, ST_BEACH, "June 9th, 1944", "The Battery", "Utah Beach, Normandy",
				"Landed on the beach strip for the first time today - engineers laid steel matting straight "
				"over the sand. From the cockpit you can see the wrecked landing craft still in the surf.\n\n"
				"A German battery up the coast is shelling the anchorage. The Navy cannot find it. We can.",
				"A coastal battery north of the beachhead is firing on the ships. Run along the shoreline "
				"at wave-top height and hit the guns in their emplacements, the machine-gun nests and "
				"the ammunition trucks behind them.\n\n"
				"The concrete bunkers will not fall to your guns - do not fly into them.",
				"Silence the battery", TOD_NOON, 0.3f, 2, 0.30f, 0.60f, true },
		// ---------------------------------------------------------------- 6
		{ MISSION_DOGFIGHT, DF_SWEEP, "June 10th, 1944", "Bandits over the Beachhead", "Sainte-Mere-Eglise, Normandy",
				"We are flying from a strip bulldozed out of a Norman apple orchard. Dust in everything, "
				"and the front so close we can hear the guns at night.\n\n"
				"The Luftwaffe has finally shown up in strength. Mac got his fourth yesterday. "
				"He pretends it is nothing but he painted the cross on his cowling himself.",
				"Radar has a formation inbound for the beachhead - 109s with Focke-Wulf 190s among them. "
				"The 190 rolls faster than you can. Do not try to turn with him; use your speed and your "
				"eight guns.\n\nKeep them off the beaches.",
				"Destroy the enemy fighters", TOD_DUSK, 0.35f, 3, 0.36f, 0.58f, true },
		// ---------------------------------------------------------------- 7
		{ MISSION_STRAFING, ST_TRAIN, "June 12th, 1944", "Train Busting", "Lisieux, Normandy",
				"Rail cutting is the fashion now. Kowalski came back yesterday with a piece of locomotive "
				"boiler through his wing - he had flown through the explosion.\n\n"
				"There is an art to it, they say: kill the engine first, then the rest cannot run away.",
				"A supply train is running west from Lisieux. Catch it on the open line, hit the locomotive "
				"to stop it, then work over the wagons. One of the cars is a flak wagon - deal with it early.\n\n"
				"The train is moving: aim ahead of it.",
				"Stop the train and destroy its wagons", TOD_MORNING, 0.5f, 2, 0.52f, 0.72f, true },
		// ---------------------------------------------------------------- 8
		{ MISSION_BOMBING, BM_BRIDGE, "June 14th, 1944", "The Bridge", "River Orne, Normandy",
				"A week of mud and flak. Lost Petersen on Monday - engine hit over Villers, he never got out. "
				"His cot was empty for one night. The replacement looks about sixteen.\n\n"
				"Today it is a bridge. Bridges are small and they shoot back.",
				"The road bridge over the Orne is the only crossing still standing in this sector. "
				"Drop it. Your run is along the river, so you will cross the bridge at an angle - "
				"pick your release carefully.\n\nFuel dumps and flak positions on the banks are secondary targets.",
				"Destroy the bridge", TOD_OVERCAST, 0.9f, 3, 0.48f, 0.68f, true },
		// ---------------------------------------------------------------- 9
		{ MISSION_DOGFIGHT, DF_INTERCEPT, "June 16th, 1944", "Bombers Inbound", "The Anchorage, Normandy",
				"The Germans are bombing the ships at dusk, when our patrols go home and theirs come out. "
				"Last night a Ju 88 put a bomb into an ammunition ship; the flash lit the whole coast.\n\n"
				"Tonight we stay up late.",
				"Junkers 88s are coming in low over the sea to bomb the anchorage. Intercept them before "
				"they cross the coast. They fly in a tight formation and every one has a rear gunner - "
				"attack from above and to the side, never sit straight behind one.\n\n"
				"The mission fails if the bombers get through.",
				"Shoot down the bombers before they reach the ships", TOD_DUSK, 0.5f, 3, 0.33f, 0.52f, true },
		// ---------------------------------------------------------------- 10
		{ MISSION_STRAFING, ST_AIRFIELD, "June 18th, 1944", "Airfield Attack", "Evreux, France",
				"Hollis called me in. There is a bar on my collar that was not there last week and a letter "
				"from home in my pocket. Dad has framed the newspaper clipping.\n\n"
				"Tonight the whole group goes after their airfields. If we catch them on the ground "
				"they cannot catch us in the air.",
				"Evreux airfield. Fighters are parked along the dispersal and fuel is stored by the hangars. "
				"One pass, on the deck, at full throttle.\n\n"
				"Airfield flak is the heaviest there is. Stay low, keep moving and do not go around twice.",
				"Destroy parked aircraft and fuel", TOD_DUSK, 0.3f, 4, 0.62f, 0.74f, true },
		// ---------------------------------------------------------------- 11
		{ MISSION_BOMBING, BM_LAUNCH_SITE, "June 20th, 1944", "Noball", "Saint-Omer, France",
				"Something new. Since the 13th, flying bombs have been falling on London - no pilot, "
				"a jet engine that sounds like a motorcycle, and a ton of explosive when it stops. "
				"The launching ramps are in the Pas-de-Calais, hidden in the woods.\n\n"
				"The targets are code-named Noball. Everyone who has been says the flak is unbelievable.",
				"A V-1 launching site: a ramp, storage buildings and the fuel and warhead stores. The ramp "
				"is the primary target. It is narrow - put your bombs along its length.\n\n"
				"These sites are ringed with light and heavy flak. Weave on the way in.",
				"Destroy the launching ramp", TOD_NOON, 0.4f, 3, 0.80f, 0.36f, false },
		// ---------------------------------------------------------------- 12
		{ MISSION_DOGFIGHT, DF_DIVER, "June 23rd, 1944", "Diver Patrol", "The Kent coast, England",
				"Back in England for a week and the sky is full of the things. They come over at four "
				"hundred miles an hour, straight as a rail, and the guns cannot touch them.\n\n"
				"So they send us. The Jug can catch one in a dive. The trick, they say, is not to be "
				"too close when it goes off.",
				"Flying bombs are crossing the coast toward London. Patrol above their track, dive to "
				"catch them and shoot them down over open country. They are fast: use height for speed "
				"and make your shots count.\n\n"
				"A warhead explodes when hit. Break away the instant you see it go.",
				"Shoot down the flying bombs", TOD_MORNING, 0.55f, 3, 0.55f, 0.20f, false },
		// ---------------------------------------------------------------- 13
		{ MISSION_BOMBING, BM_HARBOUR, "June 26th, 1944", "The Harbour", "Le Havre, France",
				"Cherbourg is falling and the Germans are shipping what they can out through Le Havre. "
				"Barges, tugs, anything that floats.\n\n"
				"We go after the port. Water is a strange thing to bomb - a miss makes a very fine splash "
				"and nothing else.",
				"Barges are moored along the quays at Le Havre. Bomb the barges and the warehouses "
				"on the quayside. Ships are hard to hit - go for the moored rows, and take the "
				"storage sheds when you cannot get a clean run.\n\n"
				"Harbour flak is concentrated. Do not linger.",
				"Destroy the barges and quayside stores", TOD_MORNING, 0.6f, 3, 0.58f, 0.44f, false },
		// ---------------------------------------------------------------- 14
		{ MISSION_DOGFIGHT, DF_AMBUSH, "June 30th, 1944", "Jumped", "Falaise, France",
				"They tell you to look up-sun. They tell you every ten seconds. Kowalski did not, "
				"and now he is walking home from somewhere near Falaise, if he is walking at all.\n\n"
				"Patrol today. Just a patrol.",
				"Routine patrol south of Caen. Intelligence has nothing to report.\n\n"
				"(Nothing to report never lasts. If you are bounced, break hard into the attack, "
				"do not dive away - the 190 will out-dive you.)",
				"Survive the ambush and destroy the attackers", TOD_NOON, 0.4f, 4, 0.44f, 0.80f, true },
		// ---------------------------------------------------------------- 15
		{ MISSION_STRAFING, ST_VILLAGE, "July 4th, 1944", "The Headquarters", "Villers-Bocage, Normandy",
				"Independence Day. The cooks found a flag and the armorers fired a salute with a "
				"captured machine gun. Then business as usual.\n\n"
				"Intelligence has a German divisional headquarters in a chateau near Villers-Bocage, "
				"staff cars in the yard. We are to pay a visit.",
				"A headquarters in a chateau at the edge of a village. Shoot up the staff cars, the radio "
				"trucks and the chateau itself. The village is close around the road: houses and the "
				"church tower are obstacles, so pick your height.\n\n"
				"Guards will return fire from the gardens.",
				"Destroy the headquarters and its vehicles", TOD_MORNING, 0.4f, 3, 0.42f, 0.72f, true },
		// ---------------------------------------------------------------- 16
		{ MISSION_BOMBING, BM_CONVOY, "July 8th, 1944", "Column on the Move", "Saint-Lo, France",
				"A whole armored column caught in the open on the road to Saint-Lo. The observation "
				"plane says they are moving fast, trying to reach the woods before we arrive.\n\n"
				"Moving targets. Mac says drop where they are going to be, not where they are.",
				"An armored column is moving along the road. Bomb the vehicles: the column is in motion, "
				"so lead it - place the impact marker ahead of the vehicles, not on them.\n\n"
				"The half-tracks carry light flak.",
				"Destroy the armored column", TOD_NOON, 0.3f, 3, 0.36f, 0.76f, true },
		// ---------------------------------------------------------------- 17
		{ MISSION_DOGFIGHT, DF_ACE, "July 12th, 1944", "The Red-Nosed 109", "Caen, Normandy",
				"There is a red-nosed 109 over Caen that has shot down four of the group in two weeks. "
				"He comes alone, out of the sun, hits once and is gone before anyone turns.\n\n"
				"Hollis wants him. He looked around the room and his eyes stopped on me.",
				"An experienced enemy pilot is operating alone over the Caen sector. Find him and "
				"shoot him down. Expect an opponent who never flies straight, who reverses his turns "
				"and uses the vertical.\n\n"
				"You go alone. It is that kind of fight.",
				"Shoot down the ace", TOD_MORNING, 0.35f, 5, 0.46f, 0.66f, true },
		// ---------------------------------------------------------------- 18
		{ MISSION_STRAFING, ST_BARGES, "July 18th, 1944", "River Traffic", "The Seine at Vernon, France",
				"The bridges over the Seine are all down now, so they cross at night on barges and hide "
				"them under the banks by day. Today we go looking under the banks.\n\n"
				"Two hundred and thirty hours in the log. Mother writes that Dad reads the newspaper "
				"out loud at breakfast, every column, looking for the 406th.",
				"Barges and ferries are hiding along the Seine. Fly down the river and shoot up "
				"everything on the water and the crossing points on the banks. Light flak is set up "
				"on both banks.\n\n"
				"The river bends: keep your height where the trees come down to the water.",
				"Destroy the river traffic", TOD_DUSK, 0.45f, 4, 0.66f, 0.62f, true },
	};
	return missions;
}

MissionDef random_mission(MissionType type, int variant, int difficulty) {
	Rng &r = grng();
	MissionDef m = campaign()[0];
	m.type = type;
	m.variant = variant;
	m.date = "Instant action";
	m.title = variant_name(type, variant);
	m.location = "Somewhere over Normandy";
	m.diary = "";
	m.objective = "Complete the sortie";
	m.difficulty = difficulty;
	const TimeOfDay times[] = { TOD_MORNING, TOD_NOON, TOD_DUSK, TOD_OVERCAST };
	m.time = times[r.irange(0, 3)];
	m.cloud_cover = r.range(0.25f, 0.7f);
	m.map_x = r.range(0.3f, 0.8f);
	m.map_y = r.range(0.4f, 0.8f);
	m.from_normandy = true;
	// Reuse the campaign briefing of the same variant so the player gets the tactical notes.
	for (const MissionDef &c : campaign()) {
		if (c.type == type && c.variant == variant) {
			m.briefing = c.briefing;
			m.objective = c.objective;
			m.location = c.location;
			break;
		}
	}
	return m;
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

const char *variant_name(MissionType type, int variant) {
	static const char *df[] = { "Fighter Sweep", "Bomber Escort", "Intercept", "Diver Patrol", "Ambush", "Duel" };
	static const char *bm[] = { "Rail Yard", "Bridge", "Moving Column", "Harbour", "Launch Site", "Airfield" };
	static const char *st[] = { "Convoy", "Airfield", "Train Busting", "Coastal Battery", "River Barges", "Headquarters" };
	switch (type) {
		case MISSION_DOGFIGHT:
			return df[clamp_index(variant, DF_VARIANT_COUNT)];
		case MISSION_BOMBING:
			return bm[clamp_index(variant, BM_VARIANT_COUNT)];
		case MISSION_STRAFING:
			return st[clamp_index(variant, ST_VARIANT_COUNT)];
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
	if (score >= 24000) {
		return 4;
	}
	if (score >= 14000) {
		return 3;
	}
	if (score >= 7000) {
		return 2;
	}
	if (score >= 2500) {
		return 1;
	}
	return 0;
}

} // namespace ww2
