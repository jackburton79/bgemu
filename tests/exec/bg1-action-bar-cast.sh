#!/bin/sh
# The action bar's Cast page: with the character from player.spec (a
# fighter/cleric with Cure Light Wounds memorized - hence a shell script,
# it needs the -c option a console exec-file can't give), Cast opens a page
# listing the memorized spell, picking it asks for a target (a spell aimed
# at a creature), a click on the character casts it - spending the memorized
# copy - and the bar goes back to the class row.
#
# Usage: tests/exec/bg1-action-bar-cast.sh <BG1 path>

set -u

BG1_PATH="${1:-}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
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
Mouse-Click 345,457
Mouse-Click 131,457
Assert-TargetMode cast
Mouse-Click 350,310
Assert-TargetMode none
Step-Ticks 400
Mouse-Click 345,457
Assert-TargetMode none
SCRIPT_EOF

output=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 90 \
	"$BINARY" -p "$BG1_PATH" -c player.spec -D -x "$SCRIPT" 2>&1)
rm -f "$SCRIPT"

fails=$(printf '%s\n' "$output" | grep -c "ASSERT FAIL")
oks=$(printf '%s\n' "$output" | grep -c "ASSERT OK")
cast=$(printf '%s\n' "$output" | grep -c "Spell CLERIC_CURE_LIGHT_WOUNDS finished")
notmem=$(printf '%s\n' "$output" | grep -c 'not currently memorized')
if [ "$fails" -eq 0 ] && [ "$oks" -eq 3 ] && [ "$cast" -eq 1 ] && [ "$notmem" -eq 0 ]; then
	echo "PASS  $0"
	exit 0
fi
echo "FAIL  $0 ($fails assertion failure(s), $oks ok, $cast cast, $notmem not-memorized)"
exit 1
