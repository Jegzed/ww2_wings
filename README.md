# WW2 Wings

A pilot's career over Normandy, June 1944 — a tribute to Cinemaware's *Wings* (Amiga, 1990),
moved from the Great War to the Second World War. Written in C++ for Godot 4.7 (GDExtension).

You fly a P-47 Thunderbolt with the 406th Fighter Squadron. Between sorties you read your
pilot's diary; in the air you play one of three mini-games, as in the original:

| Mini-game | View | Variants |
|---|---|---|
| **Air combat** | 3D chase camera | Fighter sweep, bomber escort (B-26), intercept (Ju 88s with gunners), V-1 "Diver" patrol, ambush, duel with an ace |
| **Bombing** | Top-down | Rail yard, bridge, moving column, harbour, V-1 launch site, airfield |
| **Ground attack** | Isometric | Convoy, airfield, train busting, coastal battery, river barges, village headquarters |

The campaign is 18 missions long (June 2nd to July 18th, 1944) and uses every variant; **Instant Action**
on the menu picks a random variant, time of day and weather.

Around them sits the career: create a pilot and spend points on four aptitudes
(Flying, Shooting, Mechanical, Stamina), earn promotions and medals, and — if your luck runs
out — watch a replacement take your bunk while your name goes on the squadron roster.

There are no art or sound assets in the repository. Aircraft, vehicles, buildings, terrain,
sky, particles, paper and every sound effect are generated in code.

## Running

```
run.bat
```

`run.bat` builds the extension if needed, registers it with the Godot project and starts the game.

Requirements: Windows, Godot 4.7, Visual Studio 2022 (C++ workload), CMake 3.20+.

## Building

```
git submodule update --init
build.bat
```

This compiles godot-cpp (first build only, a few minutes) and writes `game/bin/ww2wings.dll`.
Close the game before rebuilding — Windows locks the DLL while it is loaded.

## Controls

| | Keyboard | Gamepad |
|---|---|---|
| Steer | `W A S D` / arrows | Left stick |
| Guns / bombs | `Space` | A, right trigger |
| Throttle (air combat) | `Shift` / `Ctrl` | Shoulder buttons |
| Pause | `Esc` | Start |
| Fullscreen | `F11` | |

In air combat `S` pulls the nose up and `A`/`D` roll. In ground attack `W` dives and `S` climbs.

## Project layout

```
src/core/       Procedural meshes, shaders, sky and terrain, particles, synthesised audio, UI kit
src/game/       Main scene controller, campaign data, pilot and save game
src/screens/    Menu, pilot creation, diary, briefing, debriefing, memorial, roster
src/missions/   The three mini-games and their shared ground-target system
game/           The Godot project (a one-node scene; everything else is built in C++)
godot-cpp/      Godot C++ bindings (git submodule)
```

## Automation

The game can drive itself, which is how it is tested:

```
./shot.sh mission "5,10,20" 2 strafe   # autopilot the 3rd mission, save screenshots to ./shots
godot --path game -- --shot=mission --instant=0,3 --times=10   # one-off mission: type 0-2, variant 0-5
./shot.sh viewer                        # line up every procedural model
godot --path game -- --tour --out=C:/some/dir   # play the whole campaign unattended
```

## Status

Vertical slice plus content expansion: an 18-mission campaign covering 18 mission variants, with the
full career loop. Difficulty has been tuned against the built-in autopilot only.
