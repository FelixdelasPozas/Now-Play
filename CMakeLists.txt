#
# NOWPLAYING CMake configuration.
#
project(NowPlay)

cmake_minimum_required (VERSION 2.8.6)

# Version Number
set (NOWPLAY_VERSION_MAJOR 1)
set (NOWPLAY_VERSION_MINOR 3)
set (NOWPLAY_VERSION_PATCH 0)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Manually setting Boost because the CMake script is pure shit
set(Boost_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/boost_1_72_0")
set(Boost_LIBRARIES "D:/Desarrollo/Bibliotecas/boost_1_72_0/bin.v2/libs/program_options/build/gcc-8.2.0/release/link-static/threading-multi/visibility-hidden/libboost_program_options-mgw82-mt-x64-1_72.a"
                    "D:/Desarrollo/Bibliotecas/boost_1_72_0/bin.v2/libs/filesystem/build/gcc-8.2.0/release/link-static/threading-multi/visibility-hidden/libboost_filesystem-mgw82-mt-x64-1_72.a")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated -std=c++17 -mwindows -m64 -static -mconsole")

configure_file("${PROJECT_SOURCE_DIR}/resources.rc.in" "${PROJECT_BINARY_DIR}/resources.rc")
set(CORE_SOURCES ${CORE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
configure_file("${PROJECT_SOURCE_DIR}/icon.rc.in" "${PROJECT_BINARY_DIR}/icon.rc")
set(CORE_SOURCES ${CORE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/icon.rc)
set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
enable_language(RC)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}          # Generated .h files
  ${CMAKE_CURRENT_BINARY_DIR}  # For wrap/ui files
  ${Boost_INCLUDE_DIR}
  )

set(SOURCES
  # project files
  ${CORE_SOURCES}
  nowplay.cpp
)

add_executable(nowplay ${SOURCES})
target_link_libraries(nowplay ${Boost_LIBRARIES})

add_custom_target(buildNumberDependency
                  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/buildnumber.cmake")
add_dependencies(nowplay buildNumberDependency)
