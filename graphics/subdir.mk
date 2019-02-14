LOCALSOURCES= \
../graphics/Bitmap.cpp \
../graphics/GraphicsEngine.cpp \
../graphics/Polygon.cpp \
../graphics/GraphicsDefs.cpp

LOCALOBJECTS = $(addprefix ./graphics/, $(notdir $(LOCALSOURCES:.cpp=.o)))
LOCALDEPS = $(addprefix ./graphics/, $(notdir $(LOCALOBJECTS:.o=.d)))

SOURCES+=$(LOCALSOURCES)
OBJECTS+=$(LOCALOBJECTS)
DEPS+=$(LOCALDEPS)

graphics/%.o: ../graphics/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CXXFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
