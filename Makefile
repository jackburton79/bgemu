CC=g++
CFLAGS=-Wall -Werror `sdl2-config --cflags` -Iresources -I./ -Istreams -Igraphics -Igui -Ishell -Iarchives -Isupport
EXECUTABLE=BGEmu
LDFLAGS=-lz `sdl2-config --libs`

SOURCES=\
Action.cpp \
Actor.cpp \
Animation.cpp \
AnimationFactory.cpp \
AreaRoom.cpp \
BackMap.cpp \
bgemu.cpp \
BGCharachterAnimationFactory.cpp \
BG2CharachterAnimationFactory.cpp \
Container.cpp \
Core.cpp \
Door.cpp \
EncryptedStream.cpp \
Graphics.cpp \
Game.cpp \
IETypes.cpp \
Listener.cpp \
MovieDecoder.cpp \
MovieDecoderTest.cpp \
Object.cpp \
Parsing.cpp \
Party.cpp \
PathFind.cpp \
Region.cpp \
ResManager.cpp \
RoomBase.cpp \
RoundResults.cpp \
Script.cpp \
SimpleAnimationFactory.cpp \
SplitAnimationFactory.cpp \
SoundEngine.cpp \
SupportDefs.cpp \
TextSupport.cpp \
TileCell.cpp \
Timer.cpp \
WorldMap.cpp \
graphics/Bitmap.cpp \
graphics/GraphicsEngine.cpp \
graphics/Polygon.cpp \
graphics/GraphicsDefs.cpp \
shell/Commands.cpp \
shell/Console.cpp \
shell/InputConsole.cpp \
shell/OutputConsole.cpp \
shell/ShellCommand.cpp \
gui/BackWindow.cpp \
gui/Button.cpp \
gui/Control.cpp \
gui/GUI.cpp \
gui/Label.cpp \
gui/ListView.cpp \
gui/Scrollbar.cpp \
gui/Slider.cpp \
gui/TextArea.cpp \
gui/TextEdit.cpp \
gui/Window.cpp \
support/Path.cpp \
support/Referenceable.cpp \
support/Utils.cpp \
support/ZLibDecompressor.cpp \
streams/FileStream.cpp \
streams/MemoryStream.cpp \
streams/Stream.cpp \
streams/StringStream.cpp \
resources/2DAResource.cpp \
resources/AreaResource.cpp \
resources/BamResource.cpp \
resources/BCSResource.cpp \
resources/BmpResource.cpp \
resources/CHUIResource.cpp \
resources/CreResource.cpp \
resources/DLGResource.cpp \
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
archives/Archive.cpp \
archives/BIFArchive.cpp \
archives/DirectoryArchive.cpp \
archives/FileArchive.cpp \
custom/ResourceWindow.cpp

OBJECTS=$(SOURCES:.cpp=.o)

DEPS:=$(OBJECTS:.o=.d)

all: $(OBJECTS) $(EXECUTABLE)

-include $(DEPS)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c -MM -MF $(patsubst %.o,%.d,$@) $<
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(DEPS) $(EXECUTABLE) $(OBJECTS)
