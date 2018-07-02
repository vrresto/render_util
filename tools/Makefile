PLATFORM= native
BASE_BUILD_DIR:= ../build/$(PLATFORM)
BUILD_DIR:= $(BASE_BUILD_DIR)/tools

CFLAGS+= -pthread

include ../cflags.mk

# INCLUDES+= -I.
# INCLUDES+= -Iinclude
INCLUDES+= -I../include
INCLUDES+= -I$(BASE_BUILD_DIR)

##########################################################################

CXX_SRCS+= create_curvature_map.cpp
CXX_SRCS+= create_atmosphere_map.cpp
CXX_SRCS+= plot.cpp

##########################################################################

OBJS:= $(CXX_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.o)
DEPS:= $(CXX_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.d)

.SECONDEXPANSION:
.PHONY: all clean plot

all: $(BASE_BUILD_DIR)/curvature_map $(BASE_BUILD_DIR)/atmosphere_map

-include $(DEPS)
include ../rules.mk

$(BUILD_DIR)/create_%_map: $(BUILD_DIR)/create_%_map.cpp.o
	$(CXX) -o $@  $(LDFLAGS) $<

$(BASE_BUILD_DIR)/atmosphere_map: $(BUILD_DIR)/create_atmosphere_map
	$< $@

$(BASE_BUILD_DIR)/curvature_map: $(BUILD_DIR)/create_curvature_map
	$< $@


$(BUILD_DIR)/plot: $(BASE_BUILD_DIR)/curvature_map $(BUILD_DIR)/plot.cpp.o
	$(CXX) -o $@  $(LDFLAGS) $(BUILD_DIR)/plot.cpp.o

plot: $(BASE_BUILD_DIR)/curvature_map $(BUILD_DIR)/plot
# 	true
# 	$(BUILD_DIR)/plot $(BASE_BUILD_DIR)/curvature_map
	$(BUILD_DIR)/plot $(BASE_BUILD_DIR)/curvature_map | gnuplot -p -e 'plot "-" lt rgb "black"'
