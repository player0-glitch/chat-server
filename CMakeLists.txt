cmake_minimum_required(VERSION 3.20)
project(chat-server)
set(CMAKE_CXX_STANDARD 20)
#Clangd compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Server executable
add_executable(server ${CMAKE_SOURCE_DIR}/src/server/server.cpp)

#Client executable
add_executable(client ${CMAKE_SOURCE_DIR}/src/client/client.cpp)
#compile with some debug symbols
target_compile_options(server PRIVATE -Wall -Werror -g -O0)
target_compile_options(client PRIVATE -Wall -Werror -g -O0)

##TUI
include(FetchContent)   # to fetch repos online 
# set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui 
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG main #15587dad01a9ef0e5e79d97a16adf414f60669a5 
)
FetchContent_MakeAvailable(ftxui)
 
# project(ftxuiLANGUAGES CXX VERSION )
add_executable(clientUI ${CMAKE_SOURCE_DIR}/src/client/clientUI.cpp)
target_link_libraries(clientUI
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component )
