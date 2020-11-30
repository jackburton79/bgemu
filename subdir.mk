LOCALSOURCES = \
../Action.cpp \
../Actor.cpp \
../AreaRoom.cpp \
../BackMap.cpp \
../Container.cpp \
../Core.cpp \
../Dialog.cpp \
../Door.cpp \
../Effect.cpp \
../EncryptedStream.cpp \
../Graphics.cpp \
../Game.cpp \
../IETypes.cpp \
../Listener.cpp \
../MovieDecoder.cpp \
../MovieDecoderTest.cpp \
../Object.cpp \
../Parsing.cpp \
../Party.cpp \
../PathFind.cpp \
../Region.cpp \
../ResManager.cpp \
../RoomBase.cpp \
../Script.cpp \
../SearchMap.cpp \
../SoundEngine.cpp \
../SupportDefs.cpp \
../TextSupport.cpp \
../TileCell.cpp \
../Timer.cpp \
../Triggers.cpp \
../Variables.cpp \
../WorldMap.cpp

LOCALOBJECTS= $(addprefix ./, $(notdir $(LOCALSOURCES:.cpp=.o)))
LOCALDEPS= $(addprefix ./, $(notdir $(LOCALOBJECTS:.o=.d)))

SOURCES+= $(LOCALSOURCES)
OBJECTS += $(LOCALOBJECTS)
DEPS += $(LOCALDEPS)

%.o: ./%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CXXFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
