CC=g++
CFLAGS=-Wall `sdl-config --cflags` -Iresources -I./ -Istreams -Igui -Ishell
EXECUTABLE=BGEmu
LDFLAGS= `sdl-config --libs` -lz


SOURCES=Actor.cpp \
Animation.cpp \
AnimationFactory.cpp \
Archive.cpp \
bgemu.cpp \
BifArchive.cpp \
Bitmap.cpp \
Core.cpp \
DirectoryArchive.cpp \
Door.cpp \
Frame.cpp \
Graphics.cpp \
GraphicsEngine.cpp \
gui/BackWindow.cpp \
gui/Button.cpp \
gui/Control.cpp \
gui/GUI.cpp \
gui/Label.cpp \
gui/Scrollbar.cpp \
gui/Slider.cpp \
gui/TextArea.cpp \
gui/TextEdit.cpp \
gui/Window.cpp \
IETypes.cpp \
Listener.cpp \
MovieDecoder.cpp \
MovieDecoderTest.cpp \
Object.cpp \
Parsing.cpp \
Path.cpp \
PathFind.cpp \
PlainFileArchive.cpp \
Polygon.cpp \
Referenceable.cpp \
Region.cpp \
ResManager.cpp \
resources/2DAResource.cpp \
resources/AreaResource.cpp \
resources/BamResource.cpp \
resources/BCSResource.cpp \
resources/BmpResource.cpp \
resources/CHUIResource.cpp \
resources/CreResource.cpp \
resources/GeneratedIDS.cpp \
resources/IDSResource.cpp \
resources/ITMResource.cpp \
resources/KEYResource.cpp \
resources/MOSResource.cpp \
resources/MveResource.cpp \
resources/Resource.cpp \
resources/TisResource.cpp \
resources/TLKResource.cpp \
resources/WedResource.cpp \
resources/WMAPResource.cpp \
Room.cpp \
Script.cpp \
shell/Command.cpp \
shell/Console.cpp \
shell/DoorContext.cpp \
shell/InputConsole.cpp \
shell/OutputConsole.cpp \
shell/ResourceContext.cpp \
shell/ShellContext.cpp \
SoundEngine.cpp \
streams/FileStream.cpp \
streams/MemoryStream.cpp \
streams/Stream.cpp \
streams/StringStream.cpp \
TileCell.cpp \
Timer.cpp \
Utils.cpp \
WorldMap.cpp 

OBJECTS=$(SOURCES:.cpp=.o)

all: $(OBJECTS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@
