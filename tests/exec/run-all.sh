#!/bin/sh
# Runs every regression test in this directory against the given game
# installs and reports pass/fail per file (grepping for "ASSERT FAIL").
#
# Usage: tests/exec/run-all.sh <BG1 path|-> <BG2 path|->
# Either path may be "-" to skip that game's applicable files.
#
# Expects the binary to already be built (DEBUG=1 make) at ./bin/BGEmu,
# run from the repository root.

set -u

BG1_PATH="${1:-}"
BG2_PATH="${2:-}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BINARY="./bin/BGEmu"

if [ ! -x "$BINARY" ]; then
	echo "error: $BINARY not found - build it first (DEBUG=1 make)" >&2
	exit 1
fi

failures=0
total=0

run_one() {
	game_path="$1"
	file="$2"
	total=$((total + 1))
	output=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 60 \
		"$BINARY" -p "$game_path" -D -x "$file" 2>&1)
	fails=$(printf '%s\n' "$output" | grep -c "ASSERT FAIL")
	crashes=$(printf '%s\n' "$output" | grep -Ec "SEGV|AddressSanitizer: (heap|stack|global)|failed!")
	if [ "$fails" -eq 0 ] && [ "$crashes" -eq 0 ]; then
		echo "PASS  $file"
	else
		echo "FAIL  $file ($fails assertion failure(s), $crashes crash/error line(s))"
		printf '%s\n' "$output" | grep -E "ASSERT FAIL|SEGV|AddressSanitizer|failed!" | sed 's/^/      /'
		failures=$((failures + 1))
	fi
}

if [ "$BG1_PATH" != "-" ] && [ -n "$BG1_PATH" ]; then
	for f in "$SCRIPT_DIR"/scripting-*.txt "$SCRIPT_DIR"/bg1-*.txt; do
		[ -f "$f" ] && run_one "$BG1_PATH" "$f"
	done
fi

if [ "$BG2_PATH" != "-" ] && [ -n "$BG2_PATH" ]; then
	for f in "$SCRIPT_DIR"/scripting-*.txt "$SCRIPT_DIR"/bg2-*.txt; do
		[ -f "$f" ] && run_one "$BG2_PATH" "$f"
	done
fi

echo ""
echo "$((total - failures))/$total passed"
[ "$failures" -eq 0 ]
