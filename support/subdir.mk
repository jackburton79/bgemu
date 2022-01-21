LOCALSOURCES= \
../Log.cpp \
../support/Path.cpp \
../support/Referenceable.cpp \
../support/Utils.cpp \
../support/ZLibDecompressor.cpp

LOCALOBJECTS= $(addprefix ./support/, $(notdir $(LOCALSOURCES:.cpp=.o)))
LOCALDEPS= $(addprefix ./support/, $(notdir $(LOCALOBJECTS:.o=.d)))

SOURCES+=$(LOCALSOURCES)
OBJECTS+=$(LOCALOBJECTS)
DEPS+=$(LOCALDEPS)

support/%.o: ../support/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CXXFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
