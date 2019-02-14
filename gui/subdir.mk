LOCALSOURCES= \
../gui/BackWindow.cpp \
../gui/Button.cpp \
../gui/Control.cpp \
../gui/GUI.cpp \
../gui/Label.cpp \
../gui/ListView.cpp \
../gui/Scrollbar.cpp \
../gui/Slider.cpp \
../gui/TextArea.cpp \
../gui/TextEdit.cpp \
../gui/Window.cpp

LOCALOBJECTS=$(addprefix ./gui/, $(notdir $(LOCALSOURCES:.cpp=.o)))
LOCALDEPS=$(addprefix ./gui/, $(notdir $(LOCALOBJECTS:.o=.d)))

SOURCES+=$(LOCALSOURCES)
OBJECTS+=$(LOCALOBJECTS)
DEPS+=$(LOCALDEPS)

gui/%.o/: ../gui/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CXXFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
