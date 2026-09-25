#!/bin/sh
# A character made in character creation ("PLAYER1") only exists as a CRE
# injected into the running game; a save of it loaded by another run of the
# game (which has no such CRE) used to fail with "PLAYER1 ... does not exist"
# and end the program, and the injected CRE was freed while the resource manager
# still held it (a use-after-free at exit). The other save/load tests save and load in one run,
# where PLAYER1 is still there.
#
# Usage: tests/exec/bg1-created-character-load.sh <BG1 path>

set -u

BG1_PATH="${1:-}"
BINARY="./bin/BGEmu"
SAVE_PATH="/tmp/bgemu-test-created-character.gam"

if [ ! -x "$BINARY" ]; then
	echo "error: $BINARY not found - build it first (DEBUG=1 make)" >&2
	exit 1
fi
if [ -z "$BG1_PATH" ] || [ "$BG1_PATH" = "-" ]; then
	echo "SKIP  $0 (no BG1 path given)"
	exit 0
fi

rm -rf "$SAVE_PATH" "$SAVE_PATH.arecache"
SAVE_SCRIPT="$(mktemp)"
LOAD_SCRIPT="$(mktemp)"
echo "Save-Game $SAVE_PATH" > "$SAVE_SCRIPT"
cat > "$LOAD_SCRIPT" <<SCRIPT_EOF
Load-Game $SAVE_PATH
Assert-ClassLevel PLAYER1,0,1
Assert-ClassLevel PLAYER1,1,1
SCRIPT_EOF

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 90 \
	"$BINARY" -p "$BG1_PATH" -c player.spec -x "$SAVE_SCRIPT" > /dev/null 2>&1
output=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 90 \
	"$BINARY" -p "$BG1_PATH" -x "$LOAD_SCRIPT" 2>&1)
rm -rf "$SAVE_SCRIPT" "$LOAD_SCRIPT" "$SAVE_PATH" "$SAVE_PATH.arecache"

fails=$(printf '%s\n' "$output" | grep -c "ASSERT FAIL")
oks=$(printf '%s\n' "$output" | grep -c "ASSERT OK")
crashes=$(printf '%s\n' "$output" | grep -c "AddressSanitizer")
if [ "$fails" -eq 0 ] && [ "$oks" -eq 2 ] && [ "$crashes" -eq 0 ]; then
	echo "PASS  $0"
	exit 0
fi
echo "FAIL  $0 ($fails assertion failure(s), $oks ok, $crashes sanitizer report(s))"
printf '%s\n' "$output" | grep -a "ASSERT\|Load-Game\|does not exist\|AddressSanitizer"
exit 1
