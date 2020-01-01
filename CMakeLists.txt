#
# NOWPLAYING CMake configuration.
#
project(NowPlay)

cmake_minimum_required (VERSION 2.8.6)

# Version Number
set (NOWPLAY_VERSION_MAJOR 1)
set (NOWPLAY_VERSION_MINOR 0)
set (NOWPLAY_VERSION_PATCH 0)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated -std=c++17 -mwindows -m64 -static -mconsole")

configure_file("${PROJECT_SOURCE_DIR}/resources.rc.in" "${PROJECT_BINARY_DIR}/resources.rc")
set(CORE_SOURCES ${CORE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
enable_language(RC)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}          # Generated .h files
  ${CMAKE_CURRENT_BINARY_DIR}  # For wrap/ui files
  )

set(SOURCES
  # project files
  ${CORE_SOURCES}
  nowplay.cpp
)

add_executable(nowplay ${SOURCES})

add_custom_target(buildNumberDependency
                  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/buildnumber.cmake")
add_dependencies(nowplay buildNumberDependency)
