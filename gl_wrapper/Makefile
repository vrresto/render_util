PLATFORM= native
BASE_BUILD_DIR:= ../build/$(PLATFORM)
BUILD_DIR:= $(BASE_BUILD_DIR)/gl_wrapper
MESA_PATH:= ../mesa
PYTHONPATH:= $(MESA_PATH)/src/mapi/glapi/gen

include ../cflags.mk

INCLUDES+= -I../include
INCLUDES+= -I$(BASE_BUILD_DIR)

###########################################

CXX_SRCS+= gl_wrapper_main.cpp
CXX_SRCS+= gl_interface.cpp

OBJS+= $(CXX_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.o)
DEPS+= $(CXX_SRCS:%.cpp=$(BUILD_DIR)/%.cpp.d)

GENERATED_SRC+= gl_p_proc.inc
GENERATED_SRC+= gl_p_proc_init.inc 
GENERATED_SRC+= gl_inline_forwards.inc

GENERATED:= $(GENERATED_SRC:%=$(BUILD_DIR)/_generated/%)


############################################

.SECONDEXPANSION:
.PHONY: all clean generated

all: $(BUILD_DIR)/libgl_wrapper.a

-include $(DEPS)

$(BUILD_DIR)/libgl_wrapper.a: generated $(OBJS)
	ar -r $@ $(OBJS)

generated: $(GENERATED)

clean:
	rm -rf $(BUILD_DIR)

###########################################

$(BUILD_DIR)/%/.dummy:
	mkdir -p $(@D)
	touch $@

$(BUILD_DIR)/.dummy:
	mkdir -p $(@D)
	touch $@
	
# build/main.cpp.wine.o: build/_generated/gl_p_proc
# build/gl_wrapper/forward.cpp.wine.o: build/_generated/gl_forward build/_generated/gl_forward_table
# build/gl_wrapper/gl_interface.cpp.wine.o: build/_generated/gl_p_proc_init


$(BUILD_DIR)/_generated/gl_%.inc: | $$(@D)/.dummy
	PYTHONPATH=$(PYTHONPATH) GLAPI_PATH=$(PYTHONPATH) python -B scripts/print_$*.py > $@.tmp
	mv $@.tmp $@

$(BUILD_DIR)/%.cpp.o: %.cpp | $$(@D)/.dummy
	$(CXX) -o $@ -c -MMD -MP $(INCLUDES) $(CXXFLAGS) $<
