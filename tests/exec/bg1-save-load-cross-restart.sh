#!/bin/sh
# A single bgemu process never truly exercises on-disk save/load
# correctness: ResourceManager's resource cache is never evicted within
# a process's lifetime (TryEmptyResourceCache() is #if 0 - see CLAUDE.md/
# ResManager.cpp), so gResManager->GetARA("SomeArea") on a "revisit"
# keeps returning the same, already-mutated in-memory object regardless
# of what Game::Load() did to any on-disk checkpoint directory - a real
# regression here (an area's changes silently not making it into a
# save's own checkpoint archive) stays invisible in-process and only
# shows up after an actual quit-and-relaunch. Needs two separate BGEmu
# invocations, so - unlike every other file in this directory - this
# is a shell script, not a console exec-file for run-all.sh.
#
# Bug: Game::Save()/Load() used to give each save its own, freshly
# pointed-to area-checkpoint directory (AreaRoom::SetAreaCheckpointDir())
# instead of copying into/out of it - so only the area actually being
# stood in when Save-Game ran (explicitly re-checkpointed) ever made it
# into that save's directory. Any *other* area modified earlier in the
# same session (visited, changed, and left before this save) had already
# been checkpointed into the old, scratch "not yet saved" directory and
# was never copied over - reloading the save later saw that area's
# pristine, unmodified state instead. Fixed by keeping a single,
# session-long checkpoint directory (AreaRoom::AreaCheckpointDir(), see
# its own comment) that Game::Save() copies wholesale into the save's own
# archive directory, and Game::Load() copies back out of it - same idea
# as GemRB's own single cache directory, archived to/extracted from a
# save's .SAV file.
#
# Usage: tests/exec/bg1-save-load-cross-restart.sh <BG1 path>

set -u

BG1_PATH="${1:-}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BINARY="./bin/BGEmu"
SAVE_PATH="/tmp/bgemu-test-cross-restart.gam"

if [ ! -x "$BINARY" ]; then
	echo "error: $BINARY not found - build it first (DEBUG=1 make)" >&2
	exit 1
fi
if [ -z "$BG1_PATH" ] || [ "$BG1_PATH" = "-" ]; then
	echo "SKIP  $0 (no BG1 path given)"
	exit 0
fi

rm -rf "$SAVE_PATH" "$SAVE_PATH.arecache"

# Process 1: visit AR0100, open its door, leave for AR2700 (never
# revisiting AR0100 again this process), then save - all still within
# one process, so the in-memory AreaCache/ResourceManager state is what
# a naive fix could hide behind.
cat > /tmp/bgemu-test-cross-restart-1.txt <<'EOF'
Run-Action -,110,-,0,AR0100,-,1000,1000
Step-Ticks 5
Run-Action -,143,Door0101,0,-,-,0,0
Step-Ticks 2
Assert-DoorOpened Door0101,true

Run-Action -,110,-,0,AR2700,-,1300,700
Step-Ticks 5

Save-Game /tmp/bgemu-test-cross-restart.gam
EOF

# Process 2: a brand new process (empty ResourceManager cache) loads
# that save and revisits AR0100 - the only way to actually prove its
# checkpoint made it into the save's own archive directory.
cat > /tmp/bgemu-test-cross-restart-2.txt <<'EOF'
Load-Game /tmp/bgemu-test-cross-restart.gam
Step-Ticks 2
Run-Action -,110,-,0,AR0100,-,1000,1000
Step-Ticks 5
Assert-DoorOpened Door0101,true
EOF

out1=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 60 \
	"$BINARY" -p "$BG1_PATH" -D -x /tmp/bgemu-test-cross-restart-1.txt 2>&1)
out2=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 60 \
	"$BINARY" -p "$BG1_PATH" -D -x /tmp/bgemu-test-cross-restart-2.txt 2>&1)

rm -f /tmp/bgemu-test-cross-restart-1.txt /tmp/bgemu-test-cross-restart-2.txt
rm -rf "$SAVE_PATH" "$SAVE_PATH.arecache"

fails=$(printf '%s\n%s\n' "$out1" "$out2" | grep -c "ASSERT FAIL")
crashes=$(printf '%s\n%s\n' "$out1" "$out2" | grep -Ec "SEGV|AddressSanitizer: (heap|stack|global)|failed!")
if [ "$fails" -eq 0 ] && [ "$crashes" -eq 0 ]; then
	echo "PASS  $SCRIPT_DIR/bg1-save-load-cross-restart.sh"
	exit 0
else
	echo "FAIL  $SCRIPT_DIR/bg1-save-load-cross-restart.sh ($fails assertion failure(s), $crashes crash/error line(s))"
	printf '%s\n%s\n' "$out1" "$out2" | grep -E "ASSERT FAIL|SEGV|AddressSanitizer|failed!" | sed 's/^/      /'
	exit 1
fi
