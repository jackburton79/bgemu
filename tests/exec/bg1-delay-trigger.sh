#!/bin/sh
# Delay(N) is true on one script pass out of N (a pass = 16 game ticks) for an
# object whose scripts run on every pass: sampled every tick for 256 ticks
# (16 passes) Delay(4) is true for exactly 4 passes = 64 samples, while
# Delay(1) is always true. It used to be always true.
#
# Usage: tests/exec/bg1-delay-trigger.sh <BG1 path>

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
SAVE_DIR="$(mktemp -d)"
i=0
while [ "$i" -lt 256 ]; do
	printf 'Step-Ticks 1\nEvaluate-Trigger -,Delay(4)\n' >> "$SCRIPT"
	i=$((i + 1))
done
printf 'Step-Ticks 1\nEvaluate-Trigger -,Delay(1)\n' >> "$SCRIPT"

output=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 120 \
	"$BINARY" -p "$BG1_PATH" -a AR0602 -S "$SAVE_DIR" -x "$SCRIPT" 2>&1 | grep -a "^Evaluate-Trigger:")
rm -rf "$SCRIPT" "$SAVE_DIR"

delay4=$(printf '%s\n' "$output" | head -256 | grep -c "true")
delay1=$(printf '%s\n' "$output" | tail -1)
if [ "$delay4" -eq 64 ] && [ "$delay1" = "Evaluate-Trigger: true" ]; then
	echo "PASS  $0"
	exit 0
fi
echo "FAIL  $0 (Delay(4) true $delay4 times out of 256, expected 64; Delay(1): $delay1)"
exit 1
