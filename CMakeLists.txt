#
# NOWPLAY CMake configuration.
#
project(NowPlay)

cmake_minimum_required (VERSION 2.8.6)

# Version Number
set (NOWPLAY_VERSION_MAJOR 2)
set (NOWPLAY_VERSION_MINOR 3)
set (NOWPLAY_VERSION_PATCH 0)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Manually setting Boost because the CMake script is pure shit
set(Boost_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/boost_1_72_0")
set(Boost_LIBRARIES "D:/Desarrollo/Bibliotecas/boost_1_72_0/bin.v2/libs/program_options/build/gcc-8.2.0/release/link-static/threading-multi/visibility-hidden/libboost_program_options-mgw82-mt-x64-1_72.a"
                    "D:/Desarrollo/Bibliotecas/boost_1_72_0/bin.v2/libs/filesystem/build/gcc-8.2.0/release/link-static/threading-multi/visibility-hidden/libboost_filesystem-mgw82-mt-x64-1_72.a")
                    
# Find the Qt 5 lib.
find_package(Qt5 COMPONENTS Widgets WinExtras)
add_definitions(${Qt5Widgets_DEFINITIONS})            

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated -mwindows -m64")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

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
  ${Boost_INCLUDE_DIR}
  ${Qt5Widgets_INCLUDE_DIRS}
  )

qt5_add_resources(RESOURCES
  rsc/resources.qrc
)

set(SOURCES
  ${RESOURCES}
  ${CORE_SOURCES}
  main.cpp
  NowPlay.cpp
  Utils.cpp
  AboutDialog.cpp
  SettingsDialog.cpp
  CopyThread.cpp
)

set(LIBRARIES
  ${Boost_LIBRARIES}
  Qt5::Widgets
  Qt5::WinExtras
)

add_executable(nowplay ${SOURCES})
target_link_libraries(nowplay ${LIBRARIES})
qt5_use_modules(nowplay Widgets)

add_custom_target(buildNumberDependency
                  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/buildnumber.cmake)
add_dependencies(nowplay buildNumberDependency)
