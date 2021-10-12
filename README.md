bgemu
=====
[![build](https://github.com/jackburton79/bgemu/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/jackburton79/bgemu/actions/workflows/c-cpp.yml)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/ff3bb74ed5174a989893fb6bc833ff71)](https://app.codacy.com/gh/jackburton79/bgemu?utm_source=github.com&utm_medium=referral&utm_content=jackburton79/bgemu&utm_campaign=Badge_Grade_Settings)
[![CodeFactor](https://www.codefactor.io/repository/github/jackburton79/bgemu/badge)](https://www.codefactor.io/repository/github/jackburton79/bgemu)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/jackburton79/bgemu.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/jackburton79/bgemu/context:cpp)

An interpreter to run Interplay's Infinity Engine games, like Baldur's Gate 1 and 2.

![Screenshot](https://raw.github.com/jackburton79/bgemu/master/screenshots/area.png)

Only partially functional. Can load areas, actors, run internal scripts, etc.

To run, you need game files from an infinity engine based game (Baldur's Gate, Baldur's Gate 2, etc) 

 ./BGEmu --path=<path-to-game>
  
commandline options:
  --no-newgame      Don't start a new game
  --test            Star in test mode
  -gNNNxNNN         Select windows size (example: -g1024x768)
  -f                Start in fullscreen mode
  --no-scripts      Don't run scripts
  
In the tests folder there is also a test application for the path finding algorithm (which needs more work)
