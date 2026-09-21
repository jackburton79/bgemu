#!/bin/sh
# The game's global NPCs are written to (and read back from) the save: one
# that was moved (Imoen) comes back where it was left, not where BALDUR.GAM
# had it, and one in an area not yet visited (Kivan, AR3200) is still there
# when the area is first entered after the load.
#
# Usage: tests/exec/bg1-npc-save-load.sh <BG1 path>

set -u

BG1_PATH="${1:-}"
BINARY="./bin/BGEmu"
SAVE_PATH="/tmp/bgemu-test-npc-save.gam"

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
Run-Action IMOEN1,48,-,0,,,3300,3500
Step-Ticks 5
Save-Game $SAVE_PATH
Load-Game $SAVE_PATH
Assert-Position IMOEN1,3300,3500
Assert-Position XZAR,4581,2694
Run-Action -,110,-,4,AR3200,,3522,3712
Step-Ticks 10
Assert-Position KIVAN,3652,1472
SCRIPT_EOF

output=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 90 \
	"$BINARY" -p "$BG1_PATH" -D -a AR2700 -x "$SCRIPT" 2>&1)
rm -rf "$SCRIPT" "$SAVE_PATH" "$SAVE_PATH.arecache"

fails=$(printf '%s\n' "$output" | grep -c "ASSERT FAIL")
oks=$(printf '%s\n' "$output" | grep -c "ASSERT OK")
if [ "$fails" -eq 0 ] && [ "$oks" -eq 3 ]; then
	echo "PASS  $0"
	exit 0
fi
echo "FAIL  $0 ($fails assertion failure(s), $oks ok)"
printf '%s\n' "$output" | grep -a "ASSERT\|Save-Game\|Load-Game"
exit 1
