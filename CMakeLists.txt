cmake_minimum_required(VERSION 4.0.0)

project(duck)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_BUILD_TYPE Debug)

find_package(TBB REQUIRED)

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
  src/app.cpp
  src/duck.cpp
  src/ui.cpp
  src/file_manager.cpp
  src/input_handler.cpp
  src/content_provider.cpp
  src/colorscheme.cpp
  src/scheduler.cpp
  src/event_bus.cpp
  src/utils.cpp)

target_include_directories(duck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(duck PRIVATE ftxui::screen ftxui::dom ftxui::component)
target_link_libraries(duck PRIVATE STDEXEC::stdexec TBB::tbb)

add_executable(
  duck_tests EXCLUDE_FROM_ALL
  src/app.cpp
  src/duck.cpp
  src/ui.cpp
  src/file_manager.cpp
  src/input_handler.cpp
  src/content_provider.cpp
  src/colorscheme.cpp
  src/scheduler.cpp
  src/event_bus.cpp
  src/utils.cpp
  tests/test_main.cpp
  tests/file_manager_test.cpp)
target_include_directories(
  duck_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
                     ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(
  duck_tests PRIVATE ftxui::screen ftxui::dom ftxui::component STDEXEC::stdexec
                     TBB::tbb)
