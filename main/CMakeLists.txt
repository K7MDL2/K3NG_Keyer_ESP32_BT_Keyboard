cmake_minimum_required(VERSION 3.16.0)

# Now have a custom, versioned, binary name

file(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/.
                  ${CMAKE_SOURCE_DIR}/components/.
                  ${CMAKE_SOURCE_DIR}/main/lib/.)
set(include_dirs ${CMAKE_SOURCE_DIR}/main
                  ${CMAKE_SOURCE_DIR}/components
                  ${CMAKE_SOURCE_DIR}/main/lib/.)

set(CMAKE_CXX_STANDARD "20")

set(COMPONENTS arduino)

set(IDF_TARGET "esp32")

set(SDKCONFIG ${CMAKE_SOURCE_DIR}/build/sdkconfig)

message(STATUS "Main:  CONFIG_DISPLAY_TYPE: ${CONFIG_DISPLAY_TYPE}")

set(core2_requirements)
if(CONFIG_DISPLAY_TYPE STREQUAL "M5STACK_CORE2_LCD" OR CONFIG_DISPLAY_TYPE STREQUAL "4")
    list(APPEND core2_requirements m5stack__m5unified)
endif()

idf_component_register(SRCS "main.cpp" ${app_sources}
    INCLUDE_DIRS "."  ${include_dirs}
    REQUIRES nvs_flash bt_keyboard bt TFT_eSPI gt911-arduino LiquidCrystal_I2C arduino-esp32
    TinyGPSPlus-ESP32 TimeLib IST8310 MCP23017 Goertzel espcoredump
    esp_driver_spi esp_driver_gpio esp_driver_i2c
    ${core2_requirements}
  )

if(core2_requirements)
    idf_component_get_property(m5_dir m5stack__m5unified COMPONENT_DIR)
    idf_component_get_property(m5_lib m5stack__m5unified COMPONENT_LIB)
    set(speaker_source "${m5_dir}/src/utility/Speaker_Class.inl")
    set(m5_source "${m5_dir}/src/M5Unified.cpp")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${speaker_source}" "${m5_source}")
    file(READ "${speaker_source}" speaker_text)
    set(stack_expression "size_t stack_size = 1536 + (_cfg.dma_buf_len * sizeof(uint32_t));")
    string(FIND "${speaker_text}" "${stack_expression}" stack_position)
    if(stack_position EQUAL -1)
        message(FATAL_ERROR "M5Unified speaker stack overlay needs review for this library version")
    endif()
    string(REPLACE "${stack_expression}"
        "size_t stack_size = 4096 + (_cfg.dma_buf_len * sizeof(uint32_t));"
        speaker_text "${speaker_text}")
    set(overlay_dir "${CMAKE_CURRENT_BINARY_DIR}/core2_audio_overlay")
    file(MAKE_DIRECTORY "${overlay_dir}")
    file(CONFIGURE OUTPUT "${overlay_dir}/Speaker_Class.inl" CONTENT "${speaker_text}" @ONLY)
    file(READ "${m5_source}" m5_text)
    set(speaker_include "#include \"utility/Speaker_Class.inl\"")
    string(FIND "${m5_text}" "${speaker_include}" include_position)
    if(include_position EQUAL -1)
        message(FATAL_ERROR "M5Unified speaker include overlay needs review for this library version")
    endif()
    string(REPLACE "${speaker_include}" "#include \"${overlay_dir}/Speaker_Class.inl\"" m5_text "${m5_text}")
    file(CONFIGURE OUTPUT "${overlay_dir}/M5Unified.cpp" CONTENT "${m5_text}" @ONLY)
    get_target_property(m5_sources ${m5_lib} SOURCES)
    list(REMOVE_ITEM m5_sources "${m5_source}" "src/M5Unified.cpp")
    set_property(TARGET ${m5_lib} PROPERTY SOURCES "${m5_sources}")
    target_sources(${m5_lib} PRIVATE "${overlay_dir}/M5Unified.cpp")
    target_include_directories(${m5_lib} PRIVATE "${m5_dir}/src/utility")
endif()

#  create precompiled files
set(out_path "${CMAKE_SOURCE_DIR}/precompiled_images/${CONFIG_DISPLAY_TYPE}")
file(MAKE_DIRECTORY ${out_path})
idf_build_get_property(build_dir BUILD_DIR)
idf_build_get_property(elf_name EXECUTABLE_NAME GENERATOR_EXPRESSION)
set(app_bin "${build_dir}/${elf_name}.bin")

add_custom_command(OUTPUT "${out_path}/app.bin"
# POST_BUILD
  DEPENDS "${build_dir}/.bin_timestamp"
#  DEPENDS "${app_bin}"
  COMMENT "Copying binary to precompiled_images/app.bin"
  COMMAND "${CMAKE_COMMAND}" -E copy "${app_bin}" "${out_path}"
  COMMAND "${CMAKE_COMMAND}" -E copy "${build_dir}/bootloader/bootloader.bin" "${out_path}" 
  COMMAND "${CMAKE_COMMAND}" -E copy "${build_dir}/partition_table/partition-table.bin" "${out_path}"   
)

add_custom_target(copy_app_binary ALL DEPENDS "${out_path}/app.bin")
add_dependencies(copy_app_binary gen_project_binary)
