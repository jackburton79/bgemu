#!/bin/sh
# The action bar's Use page: two Potions of Healing (POTN08) given to the
# character from player.spec show up on it, and picking one - a potion that
# affects its drinker needs no target - uses one up.
# A shell script for the same reason as bg1-action-bar-cast.sh (-c option).
#
# Usage: tests/exec/bg1-action-bar-use.sh <BG1 path>

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

SCRIPT="$(mktemp)"
cat > "$SCRIPT" <<'SCRIPT_EOF'
Run-Action -,82,-,2,POTN08,-,0,0
Step-Ticks 2
Assert-ItemCount -,POTN08,2
Mouse-Click 387,457
Mouse-Click 131,457
Assert-TargetMode none
Step-Ticks 300
Assert-ItemCount -,POTN08,1
SCRIPT_EOF

output=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 90 \
	"$BINARY" -p "$BG1_PATH" -c player.spec -D -x "$SCRIPT" 2>&1)
rm -f "$SCRIPT"

fails=$(printf '%s\n' "$output" | grep -c "ASSERT FAIL")
oks=$(printf '%s\n' "$output" | grep -c "ASSERT OK")
if [ "$fails" -eq 0 ] && [ "$oks" -eq 3 ]; then
	echo "PASS  $0"
	exit 0
fi
echo "FAIL  $0 ($fails assertion failure(s), $oks ok)"
exit 1
