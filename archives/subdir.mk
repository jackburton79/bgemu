LOCALSOURCES=\
../archives/Archive.cpp \
../archives/BIFArchive.cpp \
../archives/DirectoryArchive.cpp \
../archives/FileArchive.cpp

LOCALOBJECTS=$(addprefix ./archives/, $(notdir $(LOCALSOURCES:.cpp=.o)))
LOCALDEPS=$(addprefix ./archives/, $(notdir $(LOCALOBJECTS:.o=.d)))

SOURCES+= $(LOCALSOURCES)
OBJECTS += $(LOCALOBJECTS)
DEPS += $(LOCALDEPS)

archives/%.o: ../archives/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CXXFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
