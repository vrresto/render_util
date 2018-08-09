set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

link_libraries(pthread)

add_executable(create_curvature_map create_curvature_map.cpp)
add_executable(create_atmosphere_map create_atmosphere_map.cpp)

foreach(map_name atmosphere_map curvature_map)
  set(output ${CMAKE_CURRENT_BINARY_DIR}/${map_name})
  set(generator create_${map_name})
  set(generated_output ${generated_output} ${output})
  add_custom_target(${map_name} ALL DEPENDS ${output})
  add_custom_command(
      OUTPUT ${output}
      COMMAND ${generator} ${output}
      DEPENDS ${generator}
  )
endforeach(map_name)

install(FILES ${generated_output}
  DESTINATION ${renderutil_data_dir}
)
