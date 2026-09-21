#!/bin/sh
# MAKEGLOBAL hands one of the area's own creatures (Binkos, AR2700) to the
# Game: it keeps its state and is placed exactly once when the area is entered
# again - also after a save and load, when the area comes back from its
# checkpoint file instead of the session's cache, and must not be created a
# second time from the area's own actor table.
#
# Usage: tests/exec/bg1-npc-makeglobal.sh <BG1 path>

set -u

BG1_PATH="${1:-}"
BINARY="./bin/BGEmu"
SAVE_PATH="/tmp/bgemu-test-makeglobal.gam"

if [ ! -x "$BINARY" ]; then
	echo "error: $BINARY not found - build it first (DEBUG=1 make)" >&2
	exit 1
fi
if [ -z "$BG1_PATH" ] || [ "$BG1_PATH" = "-" ]; then
	echo "SKIP  $0 (no BG1 path given)"
	exit 0
fi

rm -rf "$SAVE_PATH" "$SAVE_PATH.arecache"
SCRIPT="$(mktemp)"
cat > "$SCRIPT" <<SCRIPT_EOF
Assert-ActorCount BINKOS,1
Run-Action BINKOS,336,-,0,-,-,0,0
Run-Action BINKOS,48,-,0,-,-,3000,3000
Step-Ticks 5
Run-Action -,110,-,4,AR0602,,3522,3712
Step-Ticks 10
Assert-ActorCount BINKOS,0
Run-Action -,110,-,4,AR2700,,3522,3712
Step-Ticks 10
Assert-ActorCount BINKOS,1
Assert-Position BINKOS,3000,3000
Save-Game $SAVE_PATH
Load-Game $SAVE_PATH
Assert-ActorCount BINKOS,1
Assert-Position BINKOS,3000,3000
SCRIPT_EOF

output=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 90 \
	"$BINARY" -p "$BG1_PATH" -D -a AR2700 -x "$SCRIPT" 2>&1)
rm -rf "$SCRIPT" "$SAVE_PATH" "$SAVE_PATH.arecache"

fails=$(printf '%s\n' "$output" | grep -c "ASSERT FAIL")
oks=$(printf '%s\n' "$output" | grep -c "ASSERT OK")
if [ "$fails" -eq 0 ] && [ "$oks" -eq 6 ]; then
	echo "PASS  $0"
	exit 0
fi
echo "FAIL  $0 ($fails assertion failure(s), $oks ok)"
printf '%s\n' "$output" | grep -a "ASSERT\|Save-Game\|Load-Game"
exit 1
