#!/bin/sh
# LEAVEAREALUA(S:Area*,S:Parchment*,P:Point*,I:Face*): only a party member's
# trip changes the game's current area (as in GemRB); a non-party creature
# (Mercante here) moves alone to the destination and the party stays put.
# The second argument is the loading-screen MOS, not an entrance name.
# A shell script because it counts the area loads in the engine's log.
#
# Usage: tests/exec/bg1-leave-area.sh <BG1 path>

set -u

BG1_PATH="${1:-}"
BINARY="./bin/BGEmu"

if [ ! -x "$BINARY" ]; then
	echo "error: $BINARY not found - build it first (DEBUG=1 make)" >&2
	exit 1
fi
if [ -z "$BG1_PATH" ] || [ "$BG1_PATH" = "-" ]; then
	echo "SKIP  $0 (no BG1 path given)"
	exit 0
fi

run() {
	script="$(mktemp)"
	printf '%s\nStep-Ticks 10\n' "$1" > "$script"
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 90 \
		"$BINARY" -p "$BG1_PATH" -D -a AR0602 -x "$script" 2>&1 | grep -c "Room::Load("
	rm -f "$script"
}

npc_loads=$(run "Run-Action DOPMER,110,-,4,AR2700,TRGORION,3522,3712")
party_loads=$(run "Run-Action -,110,-,4,AR2700,TRGORION,3522,3712")

# One load at startup; the party's trip adds a second one.
if [ "$npc_loads" -eq 1 ] && [ "$party_loads" -eq 2 ]; then
	echo "PASS  $0"
	exit 0
fi
echo "FAIL  $0 (area loads: npc leaves $npc_loads, expected 1; party leaves $party_loads, expected 2)"
exit 1
