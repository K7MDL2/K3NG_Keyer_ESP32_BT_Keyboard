# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16.0)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS "components") 

# Parse DISPLAY_TYPE from header to form part of the project name and resulting .bin image names
file(READ "${CMAKE_SOURCE_DIR}/main/keyer_features_and_options_esp32_dev.h" CONFIG_CONTENTS)
string(REGEX MATCH "[^//]#define[ \t]+DISPLAY_TYPE[ \t]+([^ \t\r\n]+)" _match "${CONFIG_CONTENTS}")
set(CONFIG_DISPLAY_TYPE "${CMAKE_MATCH_1}" CACHE STRING "CONFIG_DISPLAY_TYPE" FORCE)
message(STATUS "Project: Detected CONFIG_DISPLAY_TYPE: ${CONFIG_DISPLAY_TYPE}")

if("${CONFIG_DISPLAY_TYPE}" STREQUAL "2" OR "${CONFIG_DISPLAY_TYPE}" STREQUAL "3")
    set(CONFIG_TFT_eSPI_ESPIDF CACHE STRING "CONFIG_TFT_eSPI_ESPIDF" FORCE)
endif()

set(PROJECT_DISPLAY_TYPE ${CONFIG_DISPLAY_TYPE})

set(PROJECT_NAME "K3NG_Keyer")  # the main project name
# or can create unique files name
#set(PROJECT_PREF "K3NG_Keyer")  # the main project name
#set(PROJECT_NAME "${PROJECT_PREF}_${PROJECT_DISPLAY_TYPE}")   # prefix the display type name with main project name

message(STATUS "Project: Detected PROJECT_NAME: ${PROJECT_NAME}")  # verify name on build screen
#add_compile_definitions(CONFIG_DISPLAY_TYPE="${CONFIG_DISPLAY_TYPE}")  # use the name in the build

project(${PROJECT_NAME})

message(STATUS "Project: END: CONFIG_DISPLAY_TYPE: ${CONFIG_DISPLAY_TYPE}")

idf_build_set_property(COMPILE_OPTIONS "-DPROJECT_ESP32_COMPILER" APPEND)

# Create a SPIFFS image from the contents of the 'font' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash
#spiffs_create_partition_image(storage1 fonts FLASH_IN_PROJECT)
#spiffs_create_partition_image(storage2 icons FLASH_IN_PROJECT)
#spiffs_create_partition_image(storage2 images FLASH_IN_PROJECT)
#spiffs_create_partition_image(storage3 icons FLASH_IN_PROJECT)
