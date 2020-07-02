add_library(render_util_util
  normal_file.cpp
)

target_include_directories(render_util_util PUBLIC
  ${plog_dir}/include
)
