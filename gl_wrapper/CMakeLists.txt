set(PYTHONPATH ${MESA_PATH}/src/mapi/glapi/gen)

set(GENERATED_SRCS
  p_proc
  p_proc_init
  inline_forwards
)

set(CXX_SRCS
  gl_wrapper_main.cpp
  gl_interface.cpp
)

foreach(name ${GENERATED_SRCS})
  set(output ${PROJECT_BINARY_DIR}/gl_wrapper/_generated/gl_${name}.inc)
  add_custom_command(
      OUTPUT ${output}
      COMMAND mkdir -p ${PROJECT_BINARY_DIR}/gl_wrapper/_generated/
      COMMAND PYTHONPATH=${PYTHONPATH} GLAPI_PATH=${PYTHONPATH}
        python -B ${CMAKE_CURRENT_SOURCE_DIR}/scripts/print_${name}.py > ${output}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*
  )
  set(generated_output ${generated_output} ${output})
endforeach(name)

add_library(gl_wrapper ${CXX_SRCS})

add_custom_target(generated DEPENDS ${generated_output})

add_dependencies(gl_wrapper generated)
