PLATFORM= native
BUILD_DIR:= build/$(PLATFORM)

.SECONDEXPANSION:
.PHONY: all clean print_vars engine testbed run

##########################################################################

all: testbed maps

engine:
	make -C engine
	
testbed:
	make -C testbed
	
maps:
	make -C tools
	
run: testbed maps
	$(BUILD_DIR)/testbed/terrain_test

clean:
	rm -rf $(BUILD_DIR)

print_vars:
	@echo deps: $(DEPS)
	@echo CXX: $(CXX)
