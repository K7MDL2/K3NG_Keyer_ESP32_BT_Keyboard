cmake_minimum_required(VERSION 3.5)

#include($ENV{IDF_PATH}/tools/cmake/project.cmake)
file(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)
set(include_dirs ${CMAKE_SOURCE_DIR}/include
                  D:/Documents/Arduino/libraries)

set(CMAKE_CXX_STANDARD "20")

set(IDF_TARGET "esp32")

set(SDKCONFIG ${CMAKE_SOURCE_DIR}/build/sdkconfig)

idf_component_register(SRCS "main.cpp" "${app_sources}"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash bt_keyboard bt)

set(out_path "${CMAKE_SOURCE_DIR}/precompiled_images")
idf_build_get_property(build_dir BUILD_DIR)
idf_build_get_property(elf_name EXECUTABLE_NAME GENERATOR_EXPRESSION)
set(app_bin "${build_dir}/${elf_name}.bin")

add_custom_command(OUTPUT "${out_path}/app.bin"
  POST_BUILD
  DEPENDS "${build_dir}/.bin_timestamp"
#  DEPENDS "${app_bin}"
  COMMENT "Copying binary to precompiled_images/app.bin"
  COMMAND "${CMAKE_COMMAND}" -E copy "${app_bin}" "${out_path}"
  COMMAND "${CMAKE_COMMAND}" -E copy "${build_dir}/bootloader/bootloader.bin" "${out_path}" 
  COMMAND "${CMAKE_COMMAND}" -E copy "${build_dir}/partition_table/partition-table.bin" "${out_path}" 
)

add_custom_target(copy_app_binary ALL DEPENDS "${out_path}/app.bin")
add_dependencies(copy_app_binary gen_project_binary)
