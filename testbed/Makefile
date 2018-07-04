PLATFORM= native
BASE_BUILD_DIR:= ../build/$(PLATFORM)
BUILD_DIR:= $(BASE_BUILD_DIR)/testbed
VIEWER_BUILD_DIR:= $(BASE_BUILD_DIR)/viewer
ENGINE_BUILD_DIR:= $(BASE_BUILD_DIR)/engine
GL_WRAPPER_BUILD_DIR:= $(BASE_BUILD_DIR)/gl_wrapper
FASTNOISE_DIR:= ../FastNoise

EXECUTABLE_NAME:= terrain_test

include ../cflags.mk

LDFLAGS+= -L$(VIEWER_BUILD_DIR)
LDFLAGS+= -L$(ENGINE_BUILD_DIR)
LDFLAGS+= -L$(GL_WRAPPER_BUILD_DIR)

INCLUDES+= -I../include
INCLUDES+= -I$(FASTNOISE_DIR)
INCLUDES+= -I$(BASE_BUILD_DIR)

##########################################################################

COMMON_SRCS+= tiff.cpp
COMMON_SRCS+= map_loader.cpp
COMMON_SRCS+= texture.cpp

TERRAIN_TEST_SRCS+= terrain_test.cpp
SIMPLE_TEST_SRCS+= simple_test.cpp

ALL_SRCS:= $(COMMON_SRCS)
ALL_SRCS+= $(TERRAIN_TEST_SRCS)
ALL_SRCS+= $(SIMPLE_TEST_SRCS)

##########################################################################

OBJS:= $(COMMON_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.o)
DEPS:= $(ALL_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.d)

TERRAIN_TEST_OBJS:= $(TERRAIN_TEST_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.o)
SIMPLE_TEST_OBJS:= $(SIMPLE_TEST_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.o)

.SECONDEXPANSION:
.PHONY: all clean run libengine maps

all: $(BUILD_DIR)/$(EXECUTABLE_NAME) $(BUILD_DIR)/simple_test

libengine:
	make -C ../engine

libgl_wrapper:
	make -C ../gl_wrapper

maps:
	make -C ../tools

libviewer:
	make -C ../viewer

-include $(DEPS)
include ../rules.mk


$(BUILD_DIR)/$(EXECUTABLE_NAME): libviewer libengine libgl_wrapper $(OBJS) $(TERRAIN_TEST_OBJS)
	$(CXX) -o $@  $(LDFLAGS) $(TERRAIN_TEST_OBJS) $(OBJS) -lviewer -lengine -lgl_wrapper -lglfw -lGL  -lSOIL -ltiff

$(BUILD_DIR)/simple_test: libengine libgl_wrapper $(OBJS) $(SIMPLE_TEST_OBJS)
	$(CXX) -o $@  $(LDFLAGS) $(SIMPLE_TEST_OBJS) $(OBJS) -lviewer -lengine -lgl_wrapper -lglfw -lGL  -lSOIL -ltiff


##########################################################################

run: $(BUILD_DIR)/$(EXECUTABLE_NAME) maps
	$<
