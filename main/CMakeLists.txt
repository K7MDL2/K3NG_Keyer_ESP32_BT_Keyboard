cmake_minimum_required(VERSION 3.5)

#include($ENV{IDF_PATH}/tools/cmake/project.cmake)
file(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/.
                  ${CMAKE_SOURCE_DIR}/components/.
                  ${CMAKE_SOURCE_DIR}/components/LiquidCrystal_I2C/LiquidCrystal_I2C.cpp
                  ${CMAKE_SOURCE_DIR}/components/LiquidCrystal_I2C/LCD.cpp
                  ${CMAKE_SOURCE_DIR}/components/LiquidCrystal_I2C/I2CIO.cpp)
                  
set(include_dirs ${CMAKE_SOURCE_DIR}/main/.
                  ${CMAKE_SOURCE_DIR}/components/.
                  ${CMAKE_SOURCE_DIR}/components/LiquidCrystal_I2C/include/.)

set(CMAKE_CXX_STANDARD "20")

set(COMPONENTS arduino)

set(IDF_TARGET "esp32")

set(SDKCONFIG ${CMAKE_SOURCE_DIR}/build/sdkconfig)

#set(EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/components/LiquidCrystal_I2C)

idf_component_register(SRCS "main.cpp" ${app_sources}
#idf_component_register(SRCS "main.cpp" "LiquidCrystal_I2C.cpp" I2CIO.cpp LCD.cpp ${app_sources}
    INCLUDE_DIRS "."  ${include_dirs}
    #REQUIRES nvs_flash bt_keyboard bt st7789 spiffs TFT_lcd LiquidCrystal_PCF8574)
    REQUIRES nvs_flash bt_keyboard bt LiquidCrystal_I2C arduino-esp32)


#  create precompiled files
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
