#!/bin/sh
# The action bar's quick spell slots: a right click on an empty one offers
# the spell page, picking Cure Light Wounds (memorized by the character from
# player.spec) assigns it, a left click then casts from the slot (asking for
# a target), and the assignment survives Save-Game/Load-Game (GAM quick
# spell fields). A shell script for the same reason as bg1-action-bar-cast.sh.
#
# Usage: tests/exec/bg1-action-bar-quickslots.sh <BG1 path>

set -u

BG1_PATH="${1:-}"
BINARY="./bin/BGEmu"
SAVE_PATH="/tmp/bgemu-test-quickslots.gam"

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
Mouse-Click 257,457
Assert-TargetMode none
RightClick-Control -,3,4
Mouse-Click 131,457
Mouse-Click 257,457
Assert-TargetMode cast
Mouse-Click 300,200
Save-Game $SAVE_PATH
Load-Game $SAVE_PATH
Mouse-Click 257,457
Assert-TargetMode cast
SCRIPT_EOF

output=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 90 \
	"$BINARY" -p "$BG1_PATH" -c player.spec -D -x "$SCRIPT" 2>&1)
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
