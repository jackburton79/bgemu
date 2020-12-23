LOCALSOURCES= \
../streams/EncryptedStream.cpp \
../streams/FileStream.cpp \
../streams/MemoryStream.cpp \
../streams/Stream.cpp \
../streams/StringStream.cpp

LOCALOBJECTS = $(addprefix ./streams/, $(notdir $(LOCALSOURCES:.cpp=.o)))
LOCALDEPS = $(addprefix ./streams/, $(notdir $(LOCALOBJECTS:.o=.d)))

SOURCES+=$(LOCALSOURCES)
OBJECTS+=$(LOCALOBJECTS)
DEPS+=$(LOCALDEPS)

streams/%.o: ./streams/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CXXFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
