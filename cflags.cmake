set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wreturn-type -Werror -Wmissing-field-initializers")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_REENTRANT")

# -fdata-sections -ffunction-sections
# -Wall
# -rdynamic

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -Wmissing-declarations")
