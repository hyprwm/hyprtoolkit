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
  hyprutils>=0.11.0
  hyprlang>=0.6.0
  pixman-1
  libdrm
  gbm
  xkbcommon
  pango
  cairo
  pangocairo
  iniparser
  hyprgraphics>=0.3.0
  aquamarine>=0.10.0)

configure_file(hyprtoolkit.pc.in hyprtoolkit.pc @ONLY)

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-value
                    -Wno-missing-field-initializers -Wpedantic)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES DEBUG)
  message(STATUS "Configuring hyprtoolkit in Debug")
  add_compile_definitions(HYPRTOOLKIT_DEBUG)
  set(BUILD_TESTING ON)
else()
  add_compile_options(-O3)
  message(STATUS "Configuring hyprtoolkit in Release")
  set(BUILD_TESTING OFF)
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
                                             SOVERSION 4)
target_link_libraries(hyprtoolkit PUBLIC OpenGL::EGL OpenGL::OpenGL
                                         PkgConfig::deps)

check_include_file("sys/timerfd.h" HAS_TIMERFD)
pkg_check_modules(epoll IMPORTED_TARGET epoll-shim)
if(NOT HAS_TIMERFD AND epoll_FOUND)
  target_link_libraries(hyprtoolkit PUBLIC PkgConfig::epoll)
endif()

check_include_file("sys/inotify.h" HAS_INOTIFY)
pkg_check_modules(inotify IMPORTED_TARGET libinotify)
if(NOT HAS_INOTIFY AND inotify_FOUND)
  target_link_libraries(hyprtoolkit PUBLIC PkgConfig::inotify)
endif()

# Protocols
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
message(STATUS "Found wayland-protocols at ${WAYLAND_PROTOCOLS_DIR}")
pkg_get_variable(WAYLAND_SCANNER_PKGDATA_DIR wayland-scanner pkgdatadir)
message(
  STATUS "Found wayland-scanner pkgdatadir at ${WAYLAND_SCANNER_PKGDATA_DIR}")

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
    COMMAND
      hyprwayland-scanner --wayland-enums --client
      ${WAYLAND_SCANNER_PKGDATA_DIR}/wayland.xml ${CMAKE_SOURCE_DIR}/protocols/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  target_sources(hyprtoolkit PRIVATE protocols/wayland.cpp
                                     protocols/wayland.hpp)
endfunction()

protocolwayland()

protocolnew("stable/xdg-shell" "xdg-shell" false)
protocolnew("stable/linux-dmabuf" "linux-dmabuf-v1" false)
protocolnew("staging/fractional-scale" "fractional-scale-v1" false)
protocolnew("stable/viewporter" "viewporter" false)
protocolnew("staging/cursor-shape" "cursor-shape-v1" false)
protocolnew("staging/ext-session-lock" "ext-session-lock-v1" false)
protocolnew("stable/tablet" "tablet-v2" false)
protocolnew("unstable/text-input" "text-input-unstable-v3" false)
protocolnew("staging/linux-drm-syncobj" "linux-drm-syncobj-v1" false)
protocolnew("protocols" "wlr-layer-shell-unstable-v1" true)

# Tests
if(BUILD_TESTING)
  enable_testing()

  # test apps
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

  add_executable(simpleSessionLock "tests/SimpleSessionLock.cpp")
  target_link_libraries(simpleSessionLock PRIVATE PkgConfig::deps hyprtoolkit)
  add_dependencies(tests simpleSessionLock)


  # GTest
  find_package(GTest CONFIG REQUIRED)
  include(GoogleTest)
  file(GLOB_RECURSE TESTFILES CONFIGURE_DEPENDS "tests/unit/*.cpp")
  add_executable(hyprtoolkit_tests ${TESTFILES})
  target_compile_options(hyprtoolkit_tests PRIVATE --coverage)
  target_link_options(hyprtoolkit_tests PRIVATE --coverage)
  target_include_directories(
    hyprtoolkit_tests
    PUBLIC "./include"
    PRIVATE "./src" "./src/include" "./protocols" "${CMAKE_BINARY_DIR}")
  target_link_libraries(
    hyprtoolkit_tests PRIVATE hyprtoolkit GTest::gtest_main OpenGL::EGL
                                     OpenGL::OpenGL PkgConfig::deps)

  gtest_discover_tests(hyprtoolkit_tests)

  # Add coverage to hyprtoolkit
  target_compile_options(hyprtoolkit PRIVATE --coverage)
  target_link_options(hyprtoolkit PRIVATE --coverage)
endif()

# Installation
install(TARGETS hyprtoolkit)
install(DIRECTORY "include/hyprtoolkit" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_BINARY_DIR}/hyprtoolkit.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
