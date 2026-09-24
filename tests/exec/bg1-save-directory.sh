#!/bin/sh
# Saves and the session's area checkpoints live in the directory given
# with -S (default: <game path>/bgemu-save), not in the working directory:
# SAVEGAME(1) from a script writes savegame_slot1.gam there, and the
# checkpoint of the area left for it goes under current/arecache.
#
# Usage: tests/exec/bg1-save-directory.sh <BG1 path>

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

SAVE_DIR="$(mktemp -d)"
SCRIPT="$(mktemp)"
CWD_DIR="$(mktemp -d)"
cat > "$SCRIPT" <<SCRIPT_EOF
Run-Action -,190,-,1,,,0,0
Step-Ticks 2
SCRIPT_EOF

# BGEMU_SAVE_DIR must lose against -S, and the working directory must stay
# untouched: run from an empty one.
BINARY_ABS="$(cd "$(dirname "$BINARY")" && pwd)/$(basename "$BINARY")"
(cd "$CWD_DIR" && SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy BGEMU_SAVE_DIR=/nonexistent/ignored \
	timeout 90 "$BINARY_ABS" -p "$BG1_PATH" -a AR2700 -S "$SAVE_DIR" -x "$SCRIPT" >/dev/null 2>&1)

result=0
[ -f "$SAVE_DIR/savegame_slot1.gam" ] || { echo "missing $SAVE_DIR/savegame_slot1.gam"; result=1; }
[ -d "$SAVE_DIR/savegame_slot1.gam.arecache" ] || { echo "missing slot1 .arecache directory"; result=1; }
[ -z "$(ls -A "$CWD_DIR")" ] || { echo "working directory not empty: $(ls "$CWD_DIR")"; result=1; }
rm -rf "$SAVE_DIR" "$SCRIPT" "$CWD_DIR"

if [ "$result" -eq 0 ]; then
	echo "PASS  $0"
	exit 0
fi
echo "FAIL  $0"
exit 1
