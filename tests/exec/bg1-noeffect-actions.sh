#!/bin/sh
# WEATHER (67) and MULTIPLAYERSYNC (183) used to be declared but not
# implemented (each logged "no implementation for action"); they now complete
# without effect, so nothing is logged and an action queued after them
# (SETGLOBAL) still runs. LEAVEAREALUAPANIC (189) is implemented like
# LEAVEAREALUA - per IESDP it changes the area (the real scripts follow it
# with LEAVEAREALUA calls): it must log nothing and load the new area (2 area
# loads in all). A shell script because the check is on what the engine logs.
#
# Usage: tests/exec/bg1-noeffect-actions.sh <BG1 path>

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
Run-Action -,67,-,1,-,-,0,0
Run-Action -,183,-,0,-,-,0,0
Run-Action -,30,-,7,GLOBALNoEffect,-,0,0
Step-Ticks 5
Assert-Trigger -,true,Global("NoEffect","GLOBAL",7)
Run-Action -,189,-,0,AR2700,-,0,0
Step-Ticks 5
SCRIPT_EOF

output=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 90 \
	"$BINARY" -p "$BG1_PATH" -D -x "$SCRIPT" 2>&1)
rm -f "$SCRIPT"

fails=$(printf '%s\n' "$output" | grep -c "ASSERT FAIL")
oks=$(printf '%s\n' "$output" | grep -c "ASSERT OK")
missing=$(printf '%s\n' "$output" | grep -c "no implementation for action")
loads=$(printf '%s\n' "$output" | grep -c "Room::Load(")
if [ "$fails" -eq 0 ] && [ "$oks" -eq 1 ] && [ "$missing" -eq 0 ] && [ "$loads" -eq 2 ]; then
	echo "PASS  $0"
	exit 0
fi
echo "FAIL  $0 ($fails assertion failure(s), $oks ok, $missing unimplemented, $loads area loads)"
exit 1
