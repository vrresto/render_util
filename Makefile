PLATFORM= native
BUILD_DIR:= build/$(PLATFORM)

.SECONDEXPANSION:
.PHONY: all clean print_vars testbed run

##########################################################################

all: testbed

testbed:
	make -C testbed

run: testbed
	$(BUILD_DIR)/testbed/terrain_test

clean:
	rm -rf $(BUILD_DIR)

print_vars:
	@echo deps: $(DEPS)
	@echo CXX: $(CXX)
