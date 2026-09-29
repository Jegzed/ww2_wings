#!/usr/bin/env bash
# Renders automated screenshots of one screen into ./shots (used for visual checks).
# usage: ./shot.sh <name> [times] [mission-index] [label]
#   name:  menu | newpilot | diary | briefing | mission | debrief | memorial | roster | ending | viewer
#   label: optional file prefix for the saved images (defaults to <name>)
set -e
cd "$(dirname "$0")"
GODOT="${GODOT:-$LOCALAPPDATA/Microsoft/WinGet/Packages/GodotEngine.GodotEngine_Microsoft.Winget.Source_8wekyb3d8bbwe/Godot_v4.7.2-stable_win64_console.exe}"
NAME="${1:-menu}"
TIMES="${2:-1.5,4,7}"
MISSION="${3:-0}"
LABEL="${4:-$NAME}"
mkdir -p shots
"$GODOT" --path game --resolution 1600x900 -- --shot="$NAME" --label="$LABEL" --times="$TIMES" --mission="$MISSION" --out="$(pwd -W 2>/dev/null || pwd)/shots"
