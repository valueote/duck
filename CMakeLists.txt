cmake_minimum_required(VERSION 4.0.0)
project(duck)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
find_package(PkgConfig)

set(CMAKE_BUILD_TYPE Debug)

include(FetchContent)

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v6.1.9)
FetchContent_MakeAvailable(ftxui)

add_executable(duck src/main.cpp src/ui.cpp src/filemanager.cpp
                    src/inputhandler.cpp src/contentprovider.cpp src/duck.cpp)

target_include_directories(duck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(
  duck
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component)
