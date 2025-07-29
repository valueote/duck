cmake_minimum_required(VERSION 4.0.0)

set(CMAKE_CXX_COMPILER clang++)

project(duck)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

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

find_package(Boost REQUIRED COMPONENTS process)
set(CMAKE_BUILD_TYPE Debug)

include(FetchContent)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v6.1.9)
FetchContent_MakeAvailable(ftxui)

add_executable(
  duck
  src/main.cpp
  src/duck.ui.cpp
  src/duck.filemanager.cpp
  src/duck.inputhandler.cpp
  src/duck.contentprovider.cpp
  src/duck.core.cpp
  src/duck.colorscheme.cpp
  src/duck.scheduler.cpp)
target_include_directories(duck PRIVATE module)
target_sources(
  duck
  PUBLIC FILE_SET
         cxx_modules
         TYPE
         CXX_MODULES
         FILES
         module/duck.core.cppm
         module/duck.ui.cppm
         module/duck.filemanager.cppm
         module/duck.inputhandler.cppm
         module/duck.colorscheme.cppm
         module/duck.contentprovider.cppm
         module/duck.scheduler.cppm)

target_compile_features(duck PUBLIC cxx_std_23)

target_link_libraries(duck PRIVATE ftxui::screen ftxui::component ftxui::dom
                                   Boost::process)

target_link_libraries(duck PRIVATE STDEXEC::stdexec)
