LOCALSOURCES= \
../resources/2DAResource.cpp \
../resources/AreaResource.cpp \
../resources/BamResource.cpp \
../resources/BCSResource.cpp \
../resources/BmpResource.cpp \
../resources/CHUIResource.cpp \
../resources/CreResource.cpp \
../resources/DLGResource.cpp \
../resources/GeneratedIDS.cpp \
../resources/IDSResource.cpp \
../resources/ITMResource.cpp \
../resources/KEYResource.cpp \
../resources/MOSResource.cpp \
../resources/MveResource.cpp \
../resources/Resource.cpp \
../resources/TisResource.cpp \
../resources/TLKResource.cpp \
../resources/VVCResource.cpp \
../resources/WAVResource.cpp \
../resources/WedResource.cpp \
../resources/WMAPResource.cpp

LOCALOBJECTS = $(addprefix ./resources/, $(notdir $(LOCALSOURCES:.cpp=.o)))
LOCALDEPS = $(addprefix ./resources/, $(notdir $(LOCALOBJECTS:.o=.d)))

SOURCES+=$(LOCALSOURCES)
OBJECTS+=$(LOCALOBJECTS)
DEPS+=$(LOCALDEPS)

resources/%.o: ../resources/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CXXFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
