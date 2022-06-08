-include custom.mk

CC = g++
RM = rm -rf

OUTDIR = ./bin
DIR_OBJ = ./obj
INCS = $(wildcard *.h $(foreach fd, $(SUBDIR), $(fd)/*.h))
SRCS = $(wildcard *.cpp $(foreach fd, $(SUBDIR), $(fd)/*.cpp))
NODIR_SRC = $(notdir $(SRCS))
OBJS = $(addprefix $(DIR_OBJ)/, $(SRCS:cpp=o)) # obj/xxx.o obj/folder/xxx .o
INC_DIRS = -I./ $(addprefix -I, $(SUBDIR))

#PHONY := all
#all: $(TARGET)

PHONY := $(TARGET)
$(TARGET):	$(OBJS)
	mkdir -p $(OUTDIR)
	$(CC) -o $(OUTDIR)/$@ $(OBJS) $(LDFLAGS) $(LIBS)

$(DIR_OBJ)/%.o: %.cpp $(INCS)
	mkdir -p $(@D)
	$(CC) -o $@ $(CXXFLAGS) -c $< $(INC_DIRS)

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

.PHONY = $(PHONY)