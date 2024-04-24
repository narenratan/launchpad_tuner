cmake_minimum_required(VERSION 3.9.6...3.15.0)

if(WIN32)
   set(CMAKE_TOOLCHAIN_FILE ../libs/vcpkg/scripts/buildsystems/vcpkg.cmake)
   set(VCPKG_TARGET_TRIPLET x64-windows-static)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(LaunchpadTuner LANGUAGES C CXX)

set(ELEMENTS_ROOT libs/elements)

# Make sure ELEMENTS_ROOT is an absolute path to add to the CMake module path
get_filename_component(ELEMENTS_ROOT "${ELEMENTS_ROOT}" ABSOLUTE)
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${ELEMENTS_ROOT}/cmake")

# If we are building outside the project, you need to set ELEMENTS_ROOT:
if (NOT ELEMENTS_BUILD_EXAMPLES)
   include(ElementsConfigCommon)
   set(ELEMENTS_BUILD_EXAMPLES OFF)
   add_subdirectory(${ELEMENTS_ROOT} elements)
endif()

set(ELEMENTS_APP_PROJECT "LaunchpadTuner")
set(ELEMENTS_APP_TITLE "Launchpad tuner")
set(ELEMENTS_APP_COPYRIGHT "Copyright (c) 2016-2023 Joel de Guzman")
set(ELEMENTS_APP_ID "com.cycfi.hello-universe")
set(ELEMENTS_APP_VERSION "1.0")

if(APPLE)
    set(
        ELEMENTS_APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gui_mac.m
        libs/MTS-ESP/Master/libMTSMaster.cpp
    )
else()
    set(
        ELEMENTS_APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp
        libs/MTS-ESP/Master/libMTSMaster.cpp
    )
endif()

# For your custom application icon on macOS or Windows see cmake/AppIcon.cmake module
include(AppIcon)
include(ElementsConfigApp)

target_include_directories(
    ${ELEMENTS_APP_PROJECT}
    PRIVATE
    libs/clap/include
    libs/MTS-ESP/Master
)

if(APPLE)
    target_compile_options(
        ${ELEMENTS_APP_PROJECT}
        PRIVATE
        "-fno-objc-arc"
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".clap" PREFIX "")
