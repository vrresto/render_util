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
VIEW_HEIGHTMAP_SRCS+= view_heightmap.cpp

ALL_SRCS:= $(COMMON_SRCS)
ALL_SRCS+= $(TERRAIN_TEST_SRCS)
ALL_SRCS+= $(VIEW_HEIGHTMAP_SRCS)

##########################################################################

OBJS:= $(COMMON_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.o)
DEPS:= $(ALL_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.d)

TERRAIN_TEST_OBJS:= $(TERRAIN_TEST_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.o)
VIEW_HEIGHTMAP_OBJS:= $(VIEW_HEIGHTMAP_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.o)

.SECONDEXPANSION:
.PHONY: all clean run libengine maps

all: $(BUILD_DIR)/$(EXECUTABLE_NAME)
# $(BUILD_DIR)/view_heightmap

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

$(BUILD_DIR)/view_heightmap: libengine libgl_wrapper $(OBJS) $(VIEW_HEIGHTMAP_OBJS)
	$(CXX) -o $@  $(LDFLAGS) $(VIEW_HEIGHTMAP_OBJS) $(OBJS) -lengine -lgl_wrapper -lglfw -lGL  -lSOIL -ltiff


##########################################################################

run: $(BUILD_DIR)/$(EXECUTABLE_NAME) maps
	$<
