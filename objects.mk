USER_OBJS :=

LIBS := -lz `sdl2-config --libs`

CXXFLAGS := -Wall -Werror `sdl2-config --cflags` -Iresources -I./ -Istreams -Igraphics -Igui -Ishell -Iarchives -Isupport