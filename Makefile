RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include archives/subdir.mk
-include graphics/subdir.mk
-include gui/subdir.mk
-include resources/subdir.mk
-include shell/subdir.mk
-include streams/subdir.mk
-include support/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(DEPS)),)
-include $(DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif


# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: BGEmu PathFindTest RandTest

# Tool invocations
BGEmu: $(OBJECTS) $(USER_OBJS) bgemu.o
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o "BGEmu" $(OBJECTS) $(USER_OBJS) bgemu.o $(LDFLAGS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

PathFindTest: $(OBJECTS) $(USER_OBJS) tests/PathFindTest.o
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o "tests/PathFindTest" $(OBJECTS) $(USER_OBJS) tests/PathFindTest.o $(LDFLAGS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

RandTest: $(OBJECTS) $(USER_OBJS) tests/RandTest.o
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o "tests/RandTest" $(OBJECTS) $(USER_OBJS) tests/RandTest.o $(LDFLAGS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJECTS)$(DEPS)$(EXECUTABLES)$(DEPS)$(C_UPPER_DEPS) bgemu.o tests/PathFindTest
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

