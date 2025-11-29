# Customize the components list based on display type
# Read sdkconfig
#idf_build_get_property(build_dir BUILD_DIR)
# do not used SDKCONFIG here or you redefine the path, use SKCONFIG inistead
#set(SKCONFIG "${build_dir}/config/sdkconfig")  # Does not exist this early in build.
#set(SKCONFIG "${CMAKE_SOURCE_DIR}/../sdkconfig")
#message(STATUS "^^^TFT_eSPI SDKConfig path is ${SKCONFIG}")

#if(EXISTS "${SKCONFIG}")
#    file(STRINGS "${SKCONFIG}" config_lines)
#    foreach(line ${config_lines})  
#        if(line MATCHES "^CONFIG_(.+)=(.+)$")
#            set(CONFIG_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")             
#        endif()
#    endforeach()
#endif()

message(STATUS "^^^TFT_eSPI: CONFIG_DISPLAY_TYPE=${CONFIG_DISPLAY_TYPE}")

# Customize the components list based on display type
#if("${CONFIG_DISPLAY_TYPE_NAME}" STREQUAL "2" OR "${CONFIG_DISPLAY_TYPE_NAME}" STREQUAL "3")

#set(include_dirs ${CMAKE_SOURCE_DIR}/components)

#if("${CONFIG_DISPLAY_TYPE_NAME}" STREQUAL "7") 
    idf_component_register(SRCS "TFT_eSPI.cpp"
                    INCLUDE_DIRS "." ${CMAKE_SOURCE_DIR}/components/TFT_eSPI/Fonts/GFXFF
                    REQUIRES  arduino-esp32)

    #set(DISPLAY_TYPE_ "1" CACHE INTERNAL "DISPLAY_NAME" FORCE)
    #set(USE_TFT_eSPI "0" CACHE INTERNAL "DISPLAY_MODEL" FORCE)
    message(STATUS "^^^TFT_eSPI: Component Enabled for ${DISPLAY_TYPE}")

#elseif(${CONFIG_TFT_1_9_IDEASPARK_LCD})
    #set(DISPLAY_TYPE "2" CACHE INTERNAL "DISPLAY_NAME" FORCE)
    #set(USE_TFT_eSPI "1" CACHE INTERNAL "DISPLAY_MODEL" FORCE)
    #add_subdirectory("${CMAKE_SOURCE_DIR}/components/TFT_eSPI")
    #add_compile_definitions(USE_TFT_eSPI=\"$ENV{USE_TFT_eSPI}\")
    #set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/components/TFT_eSPI")
    #message(STATUS "^^^TFT_eSPI: Component Enabled for ${DISPLAY_TYPE}")

#elseif(${CONFIG_TFT_3_2_DIYMALLS_LCD})
    #set(DISPLAY_TYPE "3" CACHE INTERNAL "DISPLAY_NAME" FORCE)
    #set(USE_TFT_eSPI "1" CACHE INTERNAL "DISPLAY_MODEL" FORCE)
    #set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/components/TFT_eSPI")
    #message(STATUS "^^^TFT_eSPI: Component Enabled for ${DISPLAY_TYPE}")

#elseif(${CONFIG_M5STACK_CORE2_LCD})
    #set(DISPLAY_TYPE "4" CACHE INTERNAL "DISPLAY_NAME" FORCE)
    #set(USE_TFT_eSPI "0" CACHE INTERNAL "DISPLAY_MODEL" FORCE)
    #message(STATUS "^^^TFT_eSPI: Component Disabled for ${DISPLAY_TYPE}")
#else()
    #set(DISPLAY_TYPE "1" CACHE INTERNAL "DISPLAY_NAME" FORCE)
    #set(USE_TFT_eSPI "0" CACHE INTERNAL "DISPLAY_MODEL" FORCE)
    #message(STATUS "^^^TFT_eSPI: Component Disabled for ${DISPLAY_TYPE}")
#endif()

#if(DISPLAY_TYPE_NAME STREQUAL TFT_1_9_IDEASPARK_LCD OR DISPLAY_TYPE_NAME STREQUAL TFT_3_2_DIYMALLS_LCD)
#if(CONFIG_TFT_1_9_IDEASPARK_LCD OR CONFIG_TFT_3_2_DIYMALLS_LCD) 
    #idf_component_register(SRCS "TFT_eSPI.cpp"
    #           INCLUDE_DIRS "." "${CMAKE_SOURCE_DIR}/components/TFT_eSPI/Fonts/GFXFF"
    #           PRIV_REQUIRES arduino-esp32)

    #set(COMPONENT_SRCS "TFT_eSPI.cpp")
    #set(COMPONENT_ADD_INCLUDEDIRS ".")
    #set(PRIV_REQUIRES arduino-esp32)
    #register_component()

#    message(STATUS "^^^TFT_eSPI Component Added for ${DISPLAY_TYPE_NAME}")
#else()
    #message(STATUS "^^^TFT_eSPI Component NOT Added for ${DISPLAY_TYPE_NAME}")
#endif()


# Access the variable
#message("Display_type = ${DISPLAY_TYPE}")

# Use it in code via preprocessor
#target_compile_definitions(${COMPONENT_LIB} PRIVATE DISPLAY_TYPE="${DISPLAY_TYPE}")


#
#elseif("${DISPLAY_TYPE_NAME}" STREQUAL "TFT_3_2_DIYMALLS_LCD")
#    idf_component_register(SRCS "TFT_eSPI.cpp"
#                    INCLUDE_DIRS "." ${CMAKE_SOURCE_DIR}/components/TFT_eSPI/Fonts/GFXFF
#                    PRIV_REQUIRES arduino-esp32)
#    message(STATUS "^^Optional TFT_eSPI Component Added for ${DISPLAY_TYPE_NAME}")#
#
#else()
#    message(STATUS "^^No TFT_eSPI Component Added for ${DISPLAY_TYPE_NAME}")#
#
#endif()
#project(TFT_eSPI)