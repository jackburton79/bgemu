# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

bgemu is a from-scratch C++ reimplementation of Interplay/BioWare's Infinity
Engine (the engine behind Baldur's Gate 1/2, and structurally close to
Icewind Dale/Planescape: Torment), built directly against real game data
files (BG1/BG2 installs) rather than against a spec document. It links
against `libjgame` (a sibling library, pulled in as a git submodule) for
low-level SDL2 graphics/audio/streams/timers plumbing. Not yet playable
end-to-end, but large parts of the engine (area loading, pathfinding,
scripting, dialogs, GUI, animations, saving/loading) work against real
retail data.

## Build

```bash
git submodule update --init --recursive   # first time only, populates libjgame
make deps                                   # builds libjgame/lib/libjgame.a
make                                        # release build -> bin/BGEmu
DEBUG=1 make                                # debug build, -g -O0 -fsanitize=address
```

Needs SDL2 (`sdl2-config` on PATH) and zlib. `make clean` removes `bin/`
and `obj/`. There is no separate lint step; `-Wall -Werror` is baked into
`CXXFLAGS` so warnings fail the build.

**Always use `DEBUG=1 make` (ASan) while developing** — a release build
hides the memory bugs this codebase has a real history of (use-after-free,
leaks) that ASan catches immediately.

## Running

```bash
./bin/BGEmu -p <path-to-BG1-or-BG2-install>
```

Useful flags for development (see `README.md` for the full list):
- `-D` — debug mode (verbose script/trigger logging)
- `-x <file>` — run a console script (`--exec-file`) after the starting
  area loads, then quit; the backbone of headless testing (see below)
- `-a AR0602` — load a specific area directly, skipping the opening
  cutscene/worldmap
- `-P ANOMEN10,Imoen,Minsc` — start with a specific party instead of the
  hardcoded default
- `-c <spec-file>` — run through real character creation from a spec file
  (see `player.spec` for the format) instead of a hardcoded party; needed
  to reach any scripted content gated on `Player1` actually being freshly
  created (e.g. BG2's intro `NEWGAME.bcs` flow)
- `-d <RESREF>.<EXT>` — dump a resource (including e.g. a BAM's cycle/
  frame counts, useful for reverse-engineering an animation format)
- `--no-newgame` (`-N`) — skip the "start a new game" flow entirely
- `-S <dir>` / `BGEMU_SAVE_DIR` — where saves and area checkpoints go
  (`SavedGame::Directory()`); default `<game path>/bgemu-save`, so BG1 and
  BG2 never share saves. `tests/exec/run-all.sh` points it at a temp dir.

For headless/CI-style runs, pair with a dummy SDL driver:
```bash
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./bin/BGEmu -p <path> ...
```

The in-game console (accessible interactively, or driven via `-x`) is the
primary tool for reproducing and verifying bugs without a real mouse —
`Run-Action`, `Click-Area`/`Click-Object`, `Print-Object`/`Print-Inventory`,
`Mouse-Click`/`Mouse-Drag`, `Invoke-Control`, `Check-Passable`,
`Evaluate-Trigger`/`Evaluate-Triggers`, `Toggle-Inventory`/`-Record`/etc.,
`Screenshot`, `Set-Camera`, `Step-Ticks`. Grep `shell/Commands.cpp` (one
`ShellCommand` subclass per command) before adding a new one — there is
usually already something close. `Run-Action` takes **comma-separated**
fields: `Run-Action <actor>,<actionId>,<target>,<int1>,<string1>,<string2>,<x>,<y>`.

## Tests

Two kinds:

**Unit tests** (`tests/*.cpp`, built via `make PathFindTest` / `make RandTest`,
run directly as `./bin/PathFindTest`) — narrow, data-independent checks of
specific algorithms.

**Console regression scripts** (`tests/exec/*.txt`) — game-data-driven,
run against a real BG1/BG2 install via `-x`, self-checking via
`Assert-Trigger`/`Assert-Triggers` (print `ASSERT OK`/`ASSERT FAIL`):
```bash
tests/exec/run-all.sh <BG1-path|-> <BG2-path|->
```
`scripting-*.txt` are game-agnostic (use `-` as the actor name, resolving
to the current party leader) and should pass against both games;
`bg1-*.txt`/`bg2-*.txt` pin a real bug against real content from one
specific game (a real NPC/dialog/area) and only make sense there. See
`tests/exec/README.md` for the full conventions. Add a regression case
here for every scripting/dialog fix — there's no other safety net for
this part of the engine.

After any change, run `DEBUG=1 make` clean and `tests/exec/run-all.sh`
green on both games before calling something done.

## Architecture

### Core game loop and area lifecycle

`Core` (`game/Core.*`) is the top-level singleton: current area/room,
pause state, cutscene mode, party gold, dialog handler, random number
service. `Game` (`game/Game.*`) owns party/GUI-screen-level logic (the
GUI screens — Inventory/Record/Spellbook/Journal/Save/Load — are
implemented here, filling in CHU-authored windows with live data).
`AreaRoom`/`RoomBase` (`game/AreaRoom.*`, `RoomBase.*`) is a loaded,
playable area: actors, doors, containers, the search/collision map,
running scripts.

Area/worldmap transitions are **deferred**: `Core::RequestAreaChange()`
only sets a pending-transition flag; the actual unload/load happens at
the end of `Core::UpdateLogic()`. This exists specifically so an
in-progress script isn't destroyed out from under itself by the area it's
running in going away mid-tick. The corollary that has bitten this
codebase more than once: any action still queued for later in the *same*
tick, on an object whose owning area is about to be swapped out, needs to
check `Core::HasPendingTransition()` before running — `Object::AddAction()`
and `Object::ExecuteActions()` both do. If you touch action-queueing or
area-loading code, re-read those two guards before assuming a fix is safe.

### Resources

`resources/*Resource.{h,cpp}` — one class per IE file format (CRE, ITM,
SPL, ARE, WED, BCS, DLG, CHU, 2DA, BAM, MOS, TIS, TLK, STO, KEY, GAM,
WMAP, PLT, VVC, MVE, WAV, ...), each parsing directly from the packed
binary layout. `game/IETypes.h`/`.cpp` hold the shared on-disk `struct`s
(`IE::` namespace) these parse into — **these structs are read via raw
`ReadAt()`/memcpy from file bytes into `__attribute__((packed))` structs,
so field *declaration order* must exactly match on-disk byte order**;
getting this wrong (e.g. a wrong color-channel order) silently produces
garbage instead of a compile error. `archives/*Archive.*` handle the two
ways IE ships resources (loose files in a directory, or packed into
`.bif`/`.key`-indexed archives). `game/ResManager.cpp`'s `ResourceManager`
is the central cache/lookup (`gResManager` global) and `IDTable` resolves
`.IDS`/`.2DA` symbolic lookups and TLK string refs. Resource cache
eviction is currently disabled (`TryEmptyResourceCache` is `#if 0`) —
known, not yet a problem in practice.

### GUI

CHU files define windows/controls (buttons, labels, scrollbars, text
areas...) with numeric IDs; `gui/*.{h,cpp}` are the runtime widget
classes, `resources/CHUIResource.cpp` parses a CHU into `Window`/`Control`
trees. Real BG2 data authors some labels as blank placeholders and fills
their text from code at runtime (see `Game::_UpdateInventoryLabels()` and
siblings) — when a control's purpose or a numeric ID isn't obvious from
`chu_v1.htm` under `docs/iesdp-gh-pages/`, cross-check against a **local
GemRB clone**'s Python GUIScripts (`gemrb/GUIScripts/bg1|bg2/GUIINV.py`
etc.) and core C++ — GemRB is the most reliable ground truth this project
uses, and this session confirmed several real bugs (wrong control ID,
wrong color-channel handling, wrong glyph blit) by diffing behavior
against it. A `Label`'s font-glyph rendering recolors by *swapping the
destination bitmap's palette*, not by tinting pixels. With an 8-bit
(indexed) destination, `SDL_BlitSurface` only copies raw indices when
source and destination share the same palette contents - otherwise it
color-matches and scrambles the glyph's brightness ramp - so
`Font::_RenderString()` (`game/TextSupport.cpp`) copies the destination's
palette onto each glyph before blitting; a 16-bit destination gets a
correct index→RGB blit for free and needs no such step. Fonts flagged
`NEED_PALETTE` in `fonts.2da` (e.g. TOOLFONT) must always be given an
explicit palette (`ToolfontPalette()`), never rendered with their raw BAM
colors.

### Screens (`screens/`)

The GUI screens live in `GameScreen` subclasses, not in `Game`: one CHU, its
windows shown/hidden together, `Open()/Close()/Toggle()` (opening closes every
other screen and refreshes the command-bar icon), `Refresh()` to fill the
controls, and the control events (`ControlInvoked/RightClicked/Hovered/
DoubleClicked`, `BackgroundClicked`) that `GUI` hands to the screen owning the
CHU through `ScreenManager` (`Game::Screens()`). `PanelScreen` is the
full-screen panel layout (content window 2, command bar copy 0, portrait
column 1): `InventoryScreen`, `RecordScreen`, `JournalScreen`,
`SpellbookScreen` (GUIMG and GUIPR, one instance each). `SaveLoadScreen`
(GUISAVE and GUILOAD) and `StoreScreen` (GUISTORE, opened by STARTSTORE through
`Screens().Find<StoreScreen>()->OpenStore()`, pauses the game) derive from
`GameScreen` directly. Screens that share a class are told apart by CHU:
`Screens().Toggle("GUISAVE")`. `Assert-ScreenOpen <CHU>,<true|false>` checks
one. `ScreenSupport` holds what several share (item icons/names, weight
labels, loot entries).

Two HUD pieces are not screens, since they live in the HUD's own resource
(GUIW) and not in an auxiliary CHU: `LootWindow` (window 8, `Game::Loot()`) and
`ActionBar` (window 3, `Game::Bar()`, which also owns the click target mode
`ActionBar::TargetMode` that `AreaRoom` consumes). Saving and loading is `SavedGame`
(`Game::Saves()`: save directory, slot paths, `Save()`/`Load()`), the global
NPCs are `NPCRoster` (`Game::NPCs()`). `Game` keeps what isn't a screen: the
game loop, the party (join/leave, making an NPC), dialogs, the journal's data,
who the screens show (`ShownActor()`) and the HUD portraits.

### Loot and store windows

Both are driven by an action, not by clicking a control. USECONTAINER
(queued when a party member clicks a container or corpse) opens the loot
window - GUIW window 8, which temporarily hides the message area and
command bar (`LootWindow::Open()`); any non-party creature still
auto-takes everything. STARTSTORE opens GUISTORE's Buy/Sell page
(`StoreScreen::OpenStore()`, backed by `game/Store.{h,cpp}`, which holds the
live stock and the GemRB-derived buy/sell rules and pricing). Both pick up
control ids from GemRB's `CommonWindow.py`/`GUISTORE.py` and use
`Scrollbar::SetRowCallback()` for their item-list scrollbars; both close
when their area unloads or another screen opens. Console tests use
`Assert-LootWindow`/`Assert-ContainerHasItem` and
`Assert-StoreWindow`/`Assert-StoreStock`/`Print-Store`.

### HUD action bar

GUIW window 3 (`GUI::WINDOW_CMDS`, 12 buttons) is filled by
`ActionBar::Refresh()` (screens/ActionBar.cpp, `Game::Bar()`): the shown character's class
row (GemRB's qslots table, embedded there) with icons from GUIBTACT/
GUIBTBUT - the numbers in `kActionArt` are *frame indices of cycle 0* of
those BAMs, as in GemRB's guibtact.2da, and BG1 numbers the quick-slot frames
per slot while BG2 doesn't (detected from the BAM). Weapon buttons select the
quickslot (`Actor::SelectWeapon`, the CRE's selected-weapon word); Talk/
Attack/Cast/Use arm a one-shot `ActionBar::TargetMode` that the next area click
consumes (`AreaRoom::_HandleClickAt`, `Actor::ClickedOn(target, intent)`);
Cast/Use show a paged list (`bar_entry`) of memorized spells / magical items;
quick spells (right click assigns) live in `Actor` and the GAM, quick items are
the CRE's QuickItem1-3 slots. Refresh it whenever the shown character or their
inventory changes. Tests that need a memorized spell are shell scripts using
`-c player.spec` (`bg1-action-bar-*.sh`).

### Scripting and dialogs

`scripting/Triggers.cpp`/`Actions.cpp` implement BCS trigger/action
opcodes (dispatch tables keyed by the real `TRIGGER.IDS`/`ACTION.IDS` IDs
— coverage is partial and grows on demand as real game scripts hit a
missing one, not implemented wholesale up front). `scripting/Script.cpp`
runs a `Script` (AND/OR condition-block evaluation lives in
`Script::EvaluateTriggerList()`, shared between BCS scripts and dialog
state triggers). `scripting/Parsing.cpp`/`Tokenizer.cpp` parse both
compiled `.bcs` bytecode and the human-readable trigger/action text form
(used by DLG response text and by console commands) — the two have
subtly different parameter-passing conventions (e.g. object-function
syntax like `LastTalkedToBy()`) that have been real bug sources.
`game/Dialog.cpp` (`DialogHandler`) drives `.dlg` conversations (a
conversation opens on the first state *in trigger-index order*, not table
order - `DLGResource::InitialStates()`); the DLG
resource parser is `resources/DLGResource.cpp`. `game/Variables.cpp`
implements the GLOBAL/LOCALS/MYAREA-scoped variable store used by
`SetGlobal`/`Global`/etc — variable names are case-insensitive and
space-stripped, matching the real engine, not the intuitive C++ default.

### Actors and animations

`game/Actor.h/.cpp` is a live creature/PC (wraps a `CREResource` plus
runtime state — position, animation, path, party membership...).
`game/PathFind.cpp`/`SearchMap.cpp` do A* pathfinding over an area's
collision map. `animations/AnimationFactory.{h,cpp}` maps an animation
ID to one of several builder functions, each implementing one of the
real engine's distinct BAM-naming/cycle-numbering conventions (mirrored
from GemRB's `AvatarStruct`/`AV_ANIMTYPE` system in
`CharAnimations.cpp`) — `avatars.2da` (or a hardcoded fallback where the
real game install doesn't ship that as a loadable resource, e.g. BG2) is
the authoritative table for which builder a given animation ID needs;
don't guess a new animation ID's convention from a visually-similar one,
verify the real BAM's cycle count against the real formula first. Actor
orientation is always the extended 0–15 range in every game (not 0–7,
even in BG1) — individual animation builders fold it down where their
own real convention needs fewer directions.

Creatures outside the party that the game itself tracks (a new game's
companions and story NPCs from `BALDUR.GAM`, and later whoever leaves the
party) are **global NPCs**, as in GemRB and the original engine: `NPCRoster` (`Game::NPCs()`)
owns them, each remembers its area and point (`Actor::AreaName()`,
`Position()`), and `AreaRoom::_LoadActors()` places the ones whose area it is.
They are not part of any area's cached actors or checkpoint
(`AreaRoom::_UnloadArea()` skips them) and are saved in the GAM's second
creature table, not in an area. `Actor::IsPersistent()` = party member or
global NPC.

`AreaRoom` tracks the map selection (who mouse clicks/queued commands act
on) as `fSelectedActors`, an ordered list rather than a single actor —
`SelectActor()`/`SetSelectedActors()`/`AddToSelection()`/`ToggleSelected()`
are the entry points (all restricted to party members), `SelectedActor()`
still returns the first — the "primary" — for anything that only makes
sense for one actor (dialog, the console's single-target commands).
Clicking a party member's own avatar selects them; shift adds/removes just
that one (map click or HUD portrait, `Game::ToggleSelectedPartyMember()`);
a drag-rect replaces (or, with shift, adds to) the selection. A group
command spreads each member to a nearby point (`AreaRoom::_SpreadPoint()`,
reusing the same `kSpawnOffsets` table entrance placement already needs)
rather than a real formation — no shape, no follow-the-leader while
walking, just its starting point. Only one party member can hold a
dialog at a time (`Game::InitiateDialog()` asserts as much), so Talk and a
click on a friendly living creature always stay primary-only, even with
several selected; Attack/Defend and a click on anyone/anything else
dispatch to the whole selection.

## Working conventions specific to this project

- **Reactive, not roadmap-driven**: the user plays the game, reports a
  concrete bug or gap, you investigate against real game data and fix it.
  Prefer reproducing a bug with the console (`-x` script, `Run-Action`,
  a real CRE/dialog/area) over reasoning about it in the abstract —
  several bugs in this codebase's history looked fixed after a narrow
  code read but weren't, and only a real headless repro caught it.
- **GemRB is ground truth** for anything CHU/BAM/CRE/script-format
  related that isn't obvious from `docs/iesdp-gh-pages/`. When a local
  clone isn't available, its GitHub source is
  `https://github.com/gemrb/gemrb`.
- Keep commit messages short: what changed and why, not a step-by-step
  narrative of the investigation.
- A deliberately-deferred approximation (something correct-enough for now
  but not matching the real engine in some documented way) should stay
  commented as such in the code, not be silently left looking finished.
