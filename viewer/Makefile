PLATFORM= native
BASE_BUILD_DIR:= ../build/$(PLATFORM)
BUILD_DIR:= $(BASE_BUILD_DIR)/viewer
ENGINE_BUILD_DIR:= $(BASE_BUILD_DIR)/engine
GL_WRAPPER_BUILD_DIR:= $(BASE_BUILD_DIR)/gl_wrapper

LIB_NAME:= libviewer.a

include ../cflags.mk

INCLUDES+= -I../include
INCLUDES+= -I$(BASE_BUILD_DIR)

##########################################################################

COMMON_SRCS+= viewer_main.cpp
COMMON_SRCS+= terrain_viewer.cpp
COMMON_SRCS+= heightmap_viewer.cpp

ALL_SRCS:= $(COMMON_SRCS)

##########################################################################

OBJS:= $(COMMON_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.o)
DEPS:= $(ALL_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.d)


.SECONDEXPANSION:
.PHONY: all clean run libengine maps

all: $(BUILD_DIR)/$(LIB_NAME)

libengine:
	make -C ../engine

libgl_wrapper:
	make -C ../gl_wrapper

maps:
	make -C ../tools

-include $(DEPS)
include ../rules.mk


$(BUILD_DIR)/$(LIB_NAME): libengine libgl_wrapper $(OBJS)
	ar -r $@ $(OBJS)
