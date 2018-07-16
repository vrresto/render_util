PLATFORM= native
BASE_BUILD_DIR:= ../build/$(PLATFORM)
BUILD_DIR:= $(BASE_BUILD_DIR)/render_util
FASTNOISE_DIR:= ../FastNoise

CXX_native:=g++
CXX_wine:=wineg++

CXX:=$(CXX_$(PLATFORM))

include ../cflags.mk

INCLUDES+= -I../include
INCLUDES+= -I$(FASTNOISE_DIR)
INCLUDES+= -I$(BASE_BUILD_DIR)

##########################################################################

SRCS+= terrain.cpp
SRCS+= terrain_cdlod.cpp
SRCS+= camera.cpp
SRCS+= texunits.cpp
SRCS+= texture_util.cpp
SRCS+= texture_manager.cpp
SRCS+= shader.cpp
SRCS+= shader_util.cpp
SRCS+= map_textures.cpp
SRCS+= water.cpp


OBJS+= $(SRCS:%.cpp=$(BUILD_DIR)/%.cpp.o)
DEPS+= $(SRCS:%.cpp=$(BUILD_DIR)/%.cpp.d)

OBJS+= $(BUILD_DIR)/FastNoise.o
DEPS+= $(BUILD_DIR)/FastNoise.d

##########################################################################

.SECONDEXPANSION:
.PHONY: all clean gl_wrapper

all: $(BUILD_DIR)/librender_util.a

gl_wrapper:
	make -C ../gl_wrapper

-include $(DEPS)
include ../rules.mk

$(BUILD_DIR)/librender_util.a: gl_wrapper $(OBJS)
	ar -r $@ $(OBJS)

$(BUILD_DIR)/FastNoise.o: $(FASTNOISE_DIR)/FastNoise.cpp | $$(@D)/.dummy
	$(CXX) -o $@ -c -MMD -MP $(INCLUDES) $(CXXFLAGS) $<

##########################################################################

clean:
	rm -rf $(BUILD_DIR)
