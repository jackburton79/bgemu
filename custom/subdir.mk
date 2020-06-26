LOCALSOURCES= \
../custom/ResourceWindow.cpp \

LOCALOBJECTS = $(addprefix ./custom/, $(notdir $(LOCALSOURCES:.cpp=.o)))
LOCALDEPS = $(addprefix ./custom/, $(notdir $(LOCALOBJECTS:.o=.d)))

SOURCES+=$(LOCALSOURCES)
OBJECTS+=$(LOCALOBJECTS)
DEPS+=$(LOCALDEPS)

custom/%.o: ../custom/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CXXFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
