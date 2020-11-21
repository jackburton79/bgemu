LOCALSOURCES=\
../animations/Animation.cpp \
../animations/AnimationFactory.cpp \
../animations/BGCharachterAnimationFactory.cpp \
../animations/BG2CharachterAnimationFactory.cpp \
../animations/BGMonsterAnimationFactory.cpp \
../animations/IWDAnimationFactory.cpp \
../animations/SimpleAnimationFactory.cpp \
../animations/SplitAnimationFactory.cpp


LOCALOBJECTS=$(addprefix ./animations/, $(notdir $(LOCALSOURCES:.cpp=.o)))
LOCALDEPS=$(addprefix ./animations/, $(notdir $(LOCALOBJECTS:.o=.d)))

SOURCES+= $(LOCALSOURCES)
OBJECTS += $(LOCALOBJECTS)
DEPS += $(LOCALDEPS)

animations/%.o: ../animations/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CXXFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
