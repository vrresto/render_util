set(terrain_test_SRCS
  tiff.cpp
  map_loader.cpp
  terrain_test.cpp
)

add_executable(terrain_test ${terrain_test_SRCS})

target_link_libraries(terrain_test
  viewer
  tiff
)

set(simple_test_SRCS
  simple_test.cpp
)

add_executable(simple_test ${simple_test_SRCS})

target_link_libraries(simple_test
  viewer
)
