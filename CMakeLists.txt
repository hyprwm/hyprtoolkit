cmake_minimum_required(VERSION 3.19)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" VER_RAW)
string(STRIP ${VER_RAW} HYPRTOOLKIT_VERSION)

add_compile_definitions(HYPRTOOLKIT_VERSION="${HYPRTOOLKIT_VERSION}")

project(
  hyprtoolkit
  VERSION ${HYPRTOOLKIT_VERSION}
  DESCRIPTION "A modern C++ Wayland-native GUI toolkit")

include(CTest)
include(CheckIncludeFile)
include(GNUInstallDirs)

set(PREFIX ${CMAKE_INSTALL_PREFIX})
set(INCLUDE ${CMAKE_INSTALL_FULL_INCLUDEDIR})
set(LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS "GLES3")
find_package(hyprwayland-scanner 0.4.0 REQUIRED)
pkg_check_modules(
  deps
  REQUIRED
  IMPORTED_TARGET
  wayland-client
  wayland-protocols
  egl
  hyprutils>=0.9.0
  hyprlang>=0.6.0
  pixman-1
  libdrm
  gbm
  xkbcommon
  pango
  hyprgraphics>=0.2.0
  aquamarine>=0.9.5
)

configure_file(hyprtoolkit.pc.in hyprtoolkit.pc @ONLY)

set(CMAKE_CXX_STANDARD 23)
add_compile_options(
  -Wall
  -Wextra
  -Wno-unused-parameter
  -Wno-unused-value
  -Wno-missing-field-initializers
  -Wpedantic)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES DEBUG)
  message(STATUS "Configuring hyprtoolkit in Debug")
  add_compile_definitions(HYPRTOOLKIT_DEBUG)
else()
  add_compile_options(-O3)
  message(STATUS "Configuring hyprtoolkit in Release")
endif()

add_compile_definitions(HT_HIDDEN=public)

file(GLOB_RECURSE SRCFILES CONFIGURE_DEPENDS "src/*.cpp" "include/*.hpp")
file(GLOB_RECURSE PUBLIC_HEADERS CONFIGURE_DEPENDS "include/*.hpp")

execute_process(COMMAND ./scripts/generateShaderIncludes.sh
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_library(hyprtoolkit SHARED ${SRCFILES})
target_include_directories(
  hyprtoolkit
  PUBLIC "./include"
  PRIVATE "./src" "./src/include" "./protocols" "${CMAKE_BINARY_DIR}")
set_target_properties(hyprtoolkit PROPERTIES VERSION ${HYPRTOOLKIT_VERSION}
                                            SOVERSION 1)
target_link_libraries(hyprtoolkit OpenGL::EGL OpenGL::OpenGL PkgConfig::deps)

check_include_file("sys/timerfd.h" HAS_TIMERFD)
pkg_check_modules(epoll IMPORTED_TARGET epoll-shim)
if(NOT HAS_TIMERFD AND epoll_FOUND)
  target_link_libraries(hyprtoolkit PkgConfig::epoll)
endif()

# Protocols
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
message(STATUS "Found wayland-protocols at ${WAYLAND_PROTOCOLS_DIR}")
pkg_get_variable(WAYLAND_SCANNER_PKGDATA_DIR wayland-scanner pkgdatadir)
message(STATUS "Found wayland-scanner pkgdatadir at ${WAYLAND_SCANNER_PKGDATA_DIR}")

function(protocolNew protoPath protoName external)
  if(external)
    set(path ${CMAKE_SOURCE_DIR}/${protoPath})
  else()
    set(path ${WAYLAND_PROTOCOLS_DIR}/${protoPath})
  endif()
  add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/protocols/${protoName}.cpp
           ${CMAKE_SOURCE_DIR}/protocols/${protoName}.hpp
    COMMAND hyprwayland-scanner --client ${path}/${protoName}.xml
            ${CMAKE_SOURCE_DIR}/protocols/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  target_sources(hyprtoolkit PRIVATE protocols/${protoName}.cpp
                                    protocols/${protoName}.hpp)
endfunction()
function(protocolWayland)
  add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/protocols/wayland.cpp
           ${CMAKE_SOURCE_DIR}/protocols/wayland.hpp
    COMMAND hyprwayland-scanner --wayland-enums --client
            ${WAYLAND_SCANNER_PKGDATA_DIR}/wayland.xml ${CMAKE_SOURCE_DIR}/protocols/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  target_sources(hyprtoolkit PRIVATE protocols/wayland.cpp protocols/wayland.hpp)
endfunction()

protocolwayland()

protocolnew("stable/xdg-shell" "xdg-shell" false)
protocolnew("stable/linux-dmabuf" "linux-dmabuf-v1" false)
protocolnew("staging/fractional-scale" "fractional-scale-v1" false)
protocolnew("stable/viewporter" "viewporter" false)
protocolnew("staging/cursor-shape" "cursor-shape-v1" false)
protocolnew("stable/tablet" "tablet-v2" false)
protocolnew("unstable/text-input" "text-input-unstable-v3" false)

# tests
add_custom_target(tests)

add_executable(simpleWindow "tests/SimpleWindow.cpp")
target_link_libraries(simpleWindow PRIVATE PkgConfig::deps hyprtoolkit)
add_dependencies(tests simpleWindow)

add_executable(dialog "tests/Dialog.cpp")
target_link_libraries(dialog PRIVATE PkgConfig::deps hyprtoolkit)
add_dependencies(tests dialog)

add_executable(controls "tests/Controls.cpp")
target_link_libraries(controls PRIVATE PkgConfig::deps hyprtoolkit)
add_dependencies(tests controls)

# Installation
install(TARGETS hyprtoolkit)
install(DIRECTORY "include/hyprtoolkit" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_BINARY_DIR}/hyprtoolkit.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
