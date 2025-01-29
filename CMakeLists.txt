cmake_minimum_required(VERSION 3.20)
project(chat-server)
include(FetchContent)   # to fetch repos online 
set(CMAKE_CXX_STANDARD 20)
#Clangd compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Server executable
add_executable(server ${CMAKE_SOURCE_DIR}/src/server.cpp)

#Client executable
add_executable(client ${CMAKE_SOURCE_DIR}/src/client.cpp)
#compile with some debug symbols
target_compile_options(server PRIVATE -Wall -Werror -g -O0)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG main 5.0.0
)
FetchContent_MakeAvailable(ftxui)
 
project(ftxui-starter LANGUAGES CXX VERSION 1.0.0)
add_executable(ftxui-starter src/main.cpp)
target_link_libraries(ftxui-starter
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component )
