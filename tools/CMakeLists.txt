add_library(render_util_tools
  atmosphere_map.cpp
  curvature_map.cpp
)

if(NOT no_tools)

  add_executable(create_curvature_map create_curvature_map.cpp)
  add_executable(create_atmosphere_map create_atmosphere_map.cpp)

  target_link_libraries(create_curvature_map render_util_tools)
  target_link_libraries(create_atmosphere_map render_util_tools render_util)

  foreach(map_name atmosphere_map curvature_map)
    set(output ${CMAKE_CURRENT_BINARY_DIR}/${map_name})
    set(generator create_${map_name})

    set(generator_cmd ${generator})

    if(platform_mingw)
      if(NOT CMAKE_HOST_WIN32)
        set(generator_cmd wine ${generator_cmd})
      endif(NOT CMAKE_HOST_WIN32)
    endif(platform_mingw)

    set(generated_output ${generated_output} ${output})
    add_custom_target(${map_name} ALL DEPENDS ${output})
    add_custom_command(
        OUTPUT ${output}
        COMMAND ${generator_cmd} ${output}
        DEPENDS ${generator}
    )
  endforeach(map_name)

  install(FILES ${generated_output}
    DESTINATION ${renderutil_cache_dir}
  )

endif(NOT no_tools)
