LOCALSOURCES= \
../shell/Commands.cpp \
../shell/Console.cpp \
../shell/GameConsole.cpp \
../shell/ShellCommand.cpp

LOCALOBJECTS = $(addprefix ./shell/, $(notdir $(LOCALSOURCES:.cpp=.o)))
LOCALDEPS = $(addprefix ./shell/, $(notdir $(LOCALOBJECTS:.o=.d)))

SOURCES+=$(LOCALSOURCES)
OBJECTS+=$(LOCALOBJECTS)
DEPS+=$(LOCALDEPS)

shell/%.o: ./shell/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CXXFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
