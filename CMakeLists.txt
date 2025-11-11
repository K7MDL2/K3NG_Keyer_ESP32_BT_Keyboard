# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16.0)

set(EXTRA_COMPONENT_DIRS "components") 

# Parse DISPLAY_TYPE from header to form part of the project name and resulting .bin image names
file(READ "${CMAKE_SOURCE_DIR}/main/keyer_features_and_options_esp32_dev.h" CONFIG_CONTENTS)
string(REGEX MATCH "[^//]*#define[ \t]+DISPLAY_TYPE[ \t]+([^ \t\r\n]+)" _match "${CONFIG_CONTENTS}")
set(DISPLAY_TYPE_NAME "${CMAKE_MATCH_1}")
message(STATUS "Detected DISPLAY_TYPE: ${DISPLAY_TYPE_NAME}")
set(PROJECT_DISPLAY_TYPE ${DISPLAY_TYPE_NAME})
set(PROJECT_PREF "K3NG_Keyer")  # the main project name
set(PROJECT_NAME "${PROJECT_PREF}_${PROJECT_DISPLAY_TYPE}")   # prefix the display type name with main project name
message(STATUS "Detected PROJECT_NAME: ${PROJECT_NAME}")  # verify name on build screen
add_compile_definitions(DISPLAY_TYPE="${DISPLAY_TYPE_NAME}")  # use the name in the build

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME})

# Create a SPIFFS image from the contents of the 'font' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash
#spiffs_create_partition_image(storage1 fonts FLASH_IN_PROJECT)
#spiffs_create_partition_image(storage2 icons FLASH_IN_PROJECT)
#spiffs_create_partition_image(storage2 images FLASH_IN_PROJECT)
#spiffs_create_partition_image(storage3 icons FLASH_IN_PROJECT)
