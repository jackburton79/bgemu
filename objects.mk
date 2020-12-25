USER_OBJS :=

LIBS := -lz `sdl2-config --libs`

CXXFLAGS := -Wall -Werror -g -O0 `sdl2-config --cflags` -Iresources -I./ -Ianimations -Istreams -Igraphics -Igui -Ishell -Iarchives -Isupport