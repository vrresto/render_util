CXX_native:= g++
CXX_wine:= wineg++ -m32
CXX:= $(CXX_$(PLATFORM))

CFLAGS+= -O2
# CFLAGS+= -Os
# CFLAGS+= -fdata-sections -ffunction-sections
CFLAGS+= -fno-strict-aliasing
CFLAGS+= -g 
CFLAGS+= -pipe
CFLAGS+= -Wreturn-type -Werror -Wmissing-field-initializers
# CFLAGS+= -Wall
CFLAGS+= -DGL_GLEXT_PROTOTYPES
CFLAGS+= -D_GNU_SOURCE
CFLAGS+= -D_REENTRANT
CFLAGS+= -DGL_WRAPPER
CFLAGS+= -rdynamic

# CFLAGS+= -fvisibility=hidden

CXXFLAGS:= $(CFLAGS)
CXXFLAGS+= -std=c++17
CXXFLAGS+= -Wmissing-declarations

CFLAGS+= -Wmissing-prototypes

LDFLAGS:= $(CXXFLAGS)

# LDFLAGS+= --gc-sections --print-gc-sections
# LDFLAGS:= -Wl,-enable-stdcall-fixup -s -static-libgcc -static-libstdc++
