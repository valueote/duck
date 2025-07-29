cmake_minimum_required(VERSION 4.0.0)
set(CMAKE_CXX_COMPILER clang++)

project(duck)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_BUILD_TYPE Debug)

find_package(TBB REQUIRED)
find_package(Boost REQUIRED COMPONENTS process)

include(cmake/CPM.cmake)
cpmaddpackage(
  NAME
  stdexec
  GITHUB_REPOSITORY
  NVIDIA/stdexec
  GIT_TAG
  main
  OPTIONS
  "STDEXEC_BUILD_EXAMPLES OFF")

include(FetchContent)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v6.1.9)
FetchContent_MakeAvailable(ftxui)

add_executable(
  duck
  src/main.cpp
  src/ui.cpp
  src/filemanager.cpp
  src/inputhandler.cpp
  src/contentprovider.cpp
  src/duck.cpp
  src/colorscheme.cpp
  src/scheduler.cpp)

target_include_directories(duck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(duck PRIVATE ftxui::screen ftxui::dom ftxui::component
                                   Boost::process)

target_link_libraries(duck PRIVATE STDEXEC::stdexec)
