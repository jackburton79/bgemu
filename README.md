bgemu
=====
[![build](https://github.com/jackburton79/bgemu/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/jackburton79/bgemu/actions/workflows/c-cpp.yml)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/ff3bb74ed5174a989893fb6bc833ff71)](https://app.codacy.com/gh/jackburton79/bgemu?utm_source=github.com&utm_medium=referral&utm_content=jackburton79/bgemu&utm_campaign=Badge_Grade)
[![CodeFactor](https://www.codefactor.io/repository/github/jackburton79/bgemu/badge)](https://www.codefactor.io/repository/github/jackburton79/bgemu)

An interpreter to run Interplay's Infinity Engine games, like Baldur's Gate 1 and 2.

![Screenshot](https://raw.github.com/jackburton79/bgemu/master/screenshots/area.png)

Not yet playable, but already working

To run, you need game files from an infinity engine based game (Baldur's Gate, Baldur's Gate 2, etc) 

 ./BGEmu --path=\<path-to-game\>

## Command-line Options

<pre>
  --list-resources (-l)       List all available resources
  --test (-t)                 Start in test mode
  --test-animation (-T) ARG   Test animation with specified resource
  --dump-resource (-d) ARG    Dump specified resource to file
  --path (-p) ARG             Path to game data files
  --no-scripts (-n)           Don't run scripts
  --no-newgame (-N)           Don't start a new game
  --debug (-D)                Enable debug mode
  --fullscreen (-f)           Start in fullscreen mode
  -g NNNxNNN                  Select window size (example: -g1024x768)
  --party (-P) ARG            Comma-separated list of CRE resrefs to start the party with
                              (e.g., "-P ANOMEN10,Imoen,Minsc")
  --exec-file (-x) ARG        Path to test script (one GameConsole command per line)
                              Script runs automatically after starting area loads, then quits
  --area (-a) ARG             Area resref to load directly on startup (e.g., "-a AR0602")
                              Skips opening cutscene and worldmap
  --character (-c) ARG        Path to character-creation spec file
                              Format: "field value" lines with gender/race/class/kit/alignment
                              plus either six ability scores or a "roll" line
</pre>
