#!/bin/sh
# DAYNIGHT (Gorion's cutscenes use it) reloads the area's WED and maps. The
# search map of the first load used to be left behind, a leak reported when the
# game exits, so this checks the LeakSanitizer report of a run that reloads the
# area twice for any leak allocated by AreaRoom's map setup, on top of the
# console script's own assertions (the party still walks on the new maps).
#
# Usage: tests/exec/bg1-daynight-reload.sh <BG1 path>

set -u

BG1_PATH="${1:-}"
BINARY="./bin/BGEmu"
SCRIPT="$(dirname "$0")/bg1-daynight-reload.txt"

if [ ! -x "$BINARY" ]; then
	echo "error: $BINARY not found - build it first (DEBUG=1 make)" >&2
	exit 1
fi
if [ -z "$BG1_PATH" ] || [ "$BG1_PATH" = "-" ]; then
	echo "SKIP  $0 (no BG1 path given)"
	exit 0
fi

output=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
	LSAN_OPTIONS=suppressions=lsan.supp timeout 60 \
	"$BINARY" -p "$BG1_PATH" -D -a AR2700 -x "$SCRIPT" 2>&1)

fails=$(printf '%s\n' "$output" | grep -c "ASSERT FAIL")
leaks=$(printf '%s\n' "$output" | grep -c "AreaRoom::_Init")
if [ "$fails" -eq 0 ] && [ "$leaks" -eq 0 ]; then
	echo "PASS  $0"
	exit 0
fi
echo "FAIL  $0 ($fails assertion failure(s), $leaks leaked allocation(s) from AreaRoom::_Init*)"
printf '%s\n' "$output" | grep -E "ASSERT FAIL|AreaRoom::_Init" | sed 's/^/      /'
exit 1
