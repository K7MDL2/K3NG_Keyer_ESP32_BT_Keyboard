cmake_minimum_required(VERSION 3.16.0)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${sources} INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src" REQUIRES arduino-esp32 driver esp_driver_spi esp_driver_gpio esp_driver_i2c)

#project(bt-keyboard)