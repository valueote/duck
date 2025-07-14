cmake_minimum_required(VERSION 3.25)
project(duck)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
find_package(PkgConfig)

set(CMAKE_BUILD_TYPE Debug)

add_executable(duck src/main.cpp
  src/ui.cpp
  src/fs_utils.cpp
  src/input_handler.cpp
  src/file_manager.cpp
)
pkg_check_modules(NOTCURSES REQUIRED IMPORTED_TARGET notcurses)

target_link_libraries(duck PRIVATE PkgConfig::NOTCURSES)
