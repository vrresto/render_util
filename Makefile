PLATFORM= native
BUILD_DIR:= build/$(PLATFORM)

.SECONDEXPANSION:
.PHONY: all clean testbed tools run

##########################################################################

all: testbed tools

tools:
	make -C tools

testbed:
	make -C testbed

run: all
	$(BUILD_DIR)/testbed/terrain_test

clean:
	rm -rf $(BUILD_DIR)

cleanall:
	rm -rf build
