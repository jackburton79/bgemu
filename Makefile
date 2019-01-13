CC=g++
CFLAGS=-Wall -Werror `sdl2-config --cflags` -Iresources -I./ -Istreams -Igraphics -Igui -Ishell -Iarchives -Isupport
EXECUTABLE=BGEmu
LDFLAGS=-lz `sdl2-config --libs`

SOURCES=\
archives/Archive.cpp \
archives/BIFArchive.cpp \
archives/DirectoryArchive.cpp \
archives/FileArchive.cpp \
custom/ResourceWindow.cpp \
Action.cpp \
Actor.cpp \
Animation.cpp \
AnimationFactory.cpp \
BackMap.cpp \
bgemu.cpp \
BGCharachterAnimationFactory.cpp \
BG2CharachterAnimationFactory.cpp \
Container.cpp \
Core.cpp \
Door.cpp \
EncryptedStream.cpp \
Graphics.cpp \
graphics/Bitmap.cpp \
graphics/GraphicsEngine.cpp \
graphics/Polygon.cpp \
graphics/GraphicsDefs.cpp \
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
Room.cpp \
RoomBase.cpp \
RoundResults.cpp \
Script.cpp \
shell/Commands.cpp \
shell/Console.cpp \
shell/InputConsole.cpp \
shell/OutputConsole.cpp \
shell/ShellCommand.cpp \
SimpleAnimationFactory.cpp \
SplitAnimationFactory.cpp \
SoundEngine.cpp \
streams/FileStream.cpp \
streams/MemoryStream.cpp \
streams/Stream.cpp \
streams/StringStream.cpp \
support/Path.cpp \
support/Referenceable.cpp \
support/Utils.cpp \
support/ZLibDecompressor.cpp \
SupportDefs.cpp \
TextSupport.cpp \
TileCell.cpp \
Timer.cpp

OBJECTS=$(SOURCES:.cpp=.o)

all: $(OBJECTS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.o $(EXECUTABLE)
	$(RM) streams/*.o
	$(RM) resources/*.o
	$(RM) archives/*.o
	$(RM) graphics/*.o
	$(RM) gui/*.o
	$(RM) shell/*.o
	$(RM) support/*.o

