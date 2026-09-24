# Scripting regression tests

Console command scripts for `./bin/BGEmu -x <file>` (`--exec-file`), run
headless against a real game install. They exercise the scripting engine
(triggers, actions, variables, dialog) directly through the console - no
GUI/mouse needed - and are self-checking: each assertion prints
`ASSERT OK: ...` or `ASSERT FAIL: ...`, so a run is verified by grepping
its output for `ASSERT FAIL` instead of eyeballing it.

## Running

```bash
DEBUG=1 make
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    ./bin/BGEmu -p <path to BG1 or BG2 install> -D -x tests/exec/<file>.txt \
    2>&1 | grep "ASSERT FAIL"
```

No output from the `grep` means every assertion in that file passed.
A run writes area checkpoints (and any save) to `<install>/bgemu-save/`;
add `-S /tmp/somewhere` (or set `BGEMU_SAVE_DIR`) to keep the install clean -
`run-all.sh` does so with a temp directory.
`run-all.sh` in this directory does this for every file against both
games in one go:

```bash
tests/exec/run-all.sh /path/to/BG1 /path/to/BG2
```

Either path may be omitted (`-`) to skip that game's files.

## Conventions

- **Game-agnostic files** (`scripting-*.txt`) don't depend on any
  specific game's content - they test the scripting *mechanism* itself
  (trigger AND/OR logic, variable scoping, action dispatch), using `-`
  as the actor name wherever `Assert-Trigger`/`Run-Action`/etc. need
  one. `-` resolves to the current party leader (`Party::ActorAt(0)`)
  instead of a named room lookup, so these files run unmodified against
  either game's default starting party. Run these against both games.
- **Game-specific files** (`bg1-*.txt` / `bg2-*.txt`) regression-test a
  bug found against real content from one specific game (a real NPC, a
  real dialog file, a real area) and only make sense with that game's
  data - the file's own header comment says which `-p` path it needs
  and what starting state it assumes (default party, default starting
  area - i.e. `./bin/BGEmu -p <path> -D -x <file>` with no other flags).
- A file testing a *mechanism* (not real content) that still needs real
  area geometry - e.g. pathfinding to a `CreateCreature`-spawned actor -
  can't use either game's default starting area: both are populated
  (Candlekeep's guards react to a spawned hostile; BG2's own start runs
  straight into a cutscene), which makes a synthetic test non-
  deterministic. Put `# AREA: <resref>` as the file's first line to load
  a specific, otherwise-empty area instead (`run-all.sh` reads this and
  adds `-a <resref>`) - pick one confirmed to have zero actors of its
  own via `-a <resref> -x /dev/null` and checking "Loading other
  actors:" is empty.
- Every new fix to scripting/dialog code is a good candidate for a new
  assertion here (or a new file, for a substantial new mechanism) -
  keeps a fix from silently regressing later. `Assert-Trigger`/
  `Assert-Triggers` are the two purpose-built commands for this
  (`shell/Commands.cpp`); reach for a plain `Run-Action`/`Queue-Action`
  + assertion on its effect to test an *action*, since there's no
  generic "did this action do the right thing" introspection beyond
  checking a trigger it should have affected.
- A bug that only shows up across a real quit-and-relaunch (most
  save/load correctness - see ResManager.cpp's own comment on
  `TryEmptyResourceCache()` being disabled, which otherwise papers over
  exactly this within a single process) can't be a `.txt` exec-file:
  `run-all.sh` only ever starts BGEmu once per file. Write it as a `.sh`
  script instead (see `bg1-save-load-cross-restart.sh`) that invokes the
  binary twice itself and does its own `ASSERT FAIL`/crash grepping in
  the same PASS/FAIL format - `run-all.sh` picks up `bg1-*.sh`/`bg2-*.sh`
  alongside the `.txt` files.
