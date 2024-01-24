CC = g++
RM = rm -rf

BGEMU = BGEmu
GAMELIB = libgame.a
LIBS = -lz `sdl2-config --libs`
CXXFLAGS = -Wall -Werror -g -O0 `sdl2-config --cflags`
SUBDIR = \
animations \
archives \
game \
gui \
resources \
shell

OUTDIR = ./bin
DIR_OBJ = ./obj
INCS = $(wildcard *.h $(foreach fd, $(SUBDIR), $(fd)/*.h))
SRCS = $(wildcard /*.cpp $(foreach fd, $(SUBDIR), $(fd)/*.cpp))
NODIR_SRC = $(notdir $(SRCS))
OBJS = $(addprefix $(DIR_OBJ)/, $(SRCS:cpp=o)) # obj/xxx.o obj/folder/xxx .o
INC_DIRS = -I./ $(addprefix -I, $(SUBDIR))
INC_DIRS += -I libjgame/graphics
INC_DIRS += -I libjgame/streams
INC_DIRS += -I libjgame/support

#PHONY := all
all: $(BGEMU)

deps:
	make -C libjgame

tests:
	PathFindTest RandTest

PHONY := $(BGEMU) $(OBJS)
$(BGEMU):  bgemu.cpp $(OBJS)
	mkdir -p $(OUTDIR)
	$(CC) -o $(OUTDIR)/$@ bgemu.cpp $(OBJS) libjgame/lib/libjgame.a $(LIBS) $(INC_DIRS) $(CXXFLAGS) $(LDFLAGS)

$(DIR_OBJ)/%.o: %.cpp $(INCS)
	mkdir -p $(@D)
	$(CC) -o $@ $(CXXFLAGS) -c $< $(INC_DIRS)

PathFindTest: $(OBJS) tests/PathFindTest.cpp
	mkdir -p $(OUTDIR)
	$(CC) -o $(OUTDIR)/$@ tests/PathFindTest.cpp $(OBJS) libjgame/lib/libjgame.a $(LIBS) $(INC_DIRS) $(CXXFLAGS) $(LDFLAGS)

RandTest: $(GAMELIB) tests/RandTest.cpp
	mkdir -p $(OUTDIR)
	$(CC) -o $(OUTDIR)/$@ tests/RandTest.cpp $(OBJS) libjgame/lib/libjgame.a $(LIBS) $(INC_DIRS) $(CXXFLAGS) $(LDFLAGS)

PHONY += clean
clean:
	rm -rf $(OUTDIR)/* $(DIR_OBJ)/*

PHONY += echoes
echoes:
	@echo "INC files: $(INCS)"
	@echo "SRC files: $(SRCS)"
	@echo "OBJ files: $(OBJS)"
	@echo "LIB files: $(LIBS)"
	@echo "INC DIR: $(INC_DIRS)"
	@echo "LIB DIR: $(LIB_DIRS)"
	@echo "SUBDIR: $(SUBDIR)"

.PHONY = $(PHONY)
