# Optimisation
if(NOT enable_debug)
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=haswell")
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mssse3")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpmath=sse")
#   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math") # makes std::sort<float>() crash
#   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftree-vectorize")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGLM_FORCE_DEFAULT_ALIGNED_GENTYPES")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGLM_FORCE_INTRINSICS")
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGLM_FORCE_SSE3")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGLM_FORCE_SSSE3")
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGLM_FORCE_AVX2")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-variable")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-sign-compare")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-local-typedefs")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-reorder")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-but-set-variable")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-vla")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wfatal-errors")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wreturn-type -Werror -Wmissing-field-initializers")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_REENTRANT")

# disable win32 defines
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNODRAWTEXT")

# plog configuration
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPLOG_ENABLE_WCHAR_INPUT=0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPLOG_OMIT_LOG_DEFINES=1")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPLOG_CAPTURE_FILES=1")

# -fdata-sections -ffunction-sections
# -Wall
# -rdynamic

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -Wmissing-declarations")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic")
