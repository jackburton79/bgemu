TARGET = BGEmu
LIBS = -lz `sdl2-config --libs`
CXXFLAGS = -Wall -Werror -g -O0 `sdl2-config --cflags`
SUBDIR = \
animations \
archives \
gamescript \
graphics \
gui \
resources \
shell \
streams \
support
