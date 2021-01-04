#
# NOWPLAY CMake configuration.
#
project(NowPlay)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

cmake_minimum_required (VERSION 2.8.6)

# Version Number
set (NOWPLAY_VERSION_MAJOR 2)
set (NOWPLAY_VERSION_MINOR 5)
set (NOWPLAY_VERSION_PATCH 1)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(MINGW OR MSVC)
    set(QT_EXTRAS WinExtras)
    set(LIBRARIES Qt5::WinExtras)
    set(CXX_ADDITIONAL -mwindows)
endif(MINGW OR MSVC)

# Find the Qt 5 lib.
find_package(Qt5 COMPONENTS Widgets ${QT_EXTRAS})
add_definitions(${Qt5Widgets_DEFINITIONS})            

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated -m64 -std=c++17 ${CXX_ADDITIONAL}")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if (MINGW OR MSVC)
    configure_file("${PROJECT_SOURCE_DIR}/resources.rc.in" "${PROJECT_BINARY_DIR}/resources.rc")
    set(CORE_SOURCES ${CORE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
    enable_language(RC)
endif(MINGW OR MSVC)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}          # Generated .h files
  ${CMAKE_CURRENT_BINARY_DIR}  # For wrap/ui files
  ${Qt5Widgets_INCLUDE_DIRS}
  )

qt5_add_resources(RESOURCES
  rsc/resources.qrc
  rsc/darktheme/style.qrc
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
  ${LIBRARIES}
  Qt5::Widgets
)

add_executable(nowplay ${SOURCES})
target_link_libraries(nowplay ${LIBRARIES})
qt5_use_modules(nowplay Widgets)

add_custom_target(buildNumberDependency
                  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/buildnumber.cmake)
add_dependencies(nowplay buildNumberDependency)
