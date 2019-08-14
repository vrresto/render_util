set(script_dir ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
set(generated_output_dir ${PROJECT_BINARY_DIR}/gl_binding/_generated)

set(PYTHONPATH ${MESA_PATH}/src/mapi/glapi/gen:${generated_output_dir})
set(GLAPI_PATH ${MESA_PATH}/src/mapi/glapi/gen)

set(scripts ${script_dir}/*)

set(GENERATED_SRCS
  p_proc
  p_proc_init
  inline_forwards
)

set(CXX_SRCS
  gl_binding_main.cpp
  gl_interface.cpp
)


if(NOT render_util_enabled_gl_procs_file)
  message(FATAL_ERROR "render_util_enabled_gl_procs_file unset")
endif()


set(enabled_procs_inc_input ${render_util_enabled_gl_procs_file})
set(enabled_procs_inc_output ${generated_output_dir}/enabled_procs.inc)
set(enabled_procs_inc_generator ${script_dir}/generate_enabled_procs)
set(enabled_procs_inc_deps
  ${enabled_procs_inc_input}
  ${enabled_procs_inc_generator}
)
add_custom_command(
    OUTPUT ${enabled_procs_inc_output}
    COMMAND mkdir -p ${generated_output_dir}
    COMMAND bash ${enabled_procs_inc_generator}
      < ${enabled_procs_inc_input}
      > ${enabled_procs_inc_output}
    DEPENDS ${enabled_procs_inc_deps}
)


set(enabled_procs_py_input ${CMAKE_CURRENT_SOURCE_DIR}/enabled_procs.py.in)
set(enabled_procs_py_output ${generated_output_dir}/enabled_procs.py)
set(enabled_procs_py_deps
  ${enabled_procs_inc_output}
  ${enabled_procs_py_input}
)
add_custom_command(
    OUTPUT ${enabled_procs_py_output}
    COMMAND mkdir -p ${generated_output_dir}
    COMMAND cpp -I ${PROJECT_BINARY_DIR}
      < ${enabled_procs_py_input}
      | grep -v "'#'"
      > ${enabled_procs_py_output}
    DEPENDS ${enabled_procs_py_deps}
)


add_custom_target(scripts_generated DEPENDS ${enabled_procs_py_output})

add_custom_target(gl_binding_input_generated)


foreach(name ${GENERATED_SRCS})
  set(output ${generated_output_dir}/gl_${name}.inc)
  add_custom_command(
      OUTPUT ${output}
      COMMAND mkdir -p ${generated_output_dir}
      COMMAND PYTHONPATH=${PYTHONPATH} GLAPI_PATH=${GLAPI_PATH}
        python -B ${script_dir}/print_${name}.py > ${output}
      DEPENDS scripts_generated
      DEPENDS ${enabled_procs_py_output}
      DEPENDS ${scripts}
  )
  add_custom_target(gl_binding_input_generated_${name} DEPENDS ${output})
  add_dependencies(gl_binding_input_generated gl_binding_input_generated_${name})
endforeach(name)


add_library(gl_binding ${CXX_SRCS})

add_dependencies(gl_binding gl_binding_input_generated)
