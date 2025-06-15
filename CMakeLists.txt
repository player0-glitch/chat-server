cmake_minimum_required(VERSION 3.20)
project(chat-server)
set(CMAKE_CXX_STANDARD 23)
#Clangd compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#include directories
include_directories(${CMAKE_SOURCE_DIR}/include/)
#Server executable
add_executable(server ${CMAKE_SOURCE_DIR}/src/server/server.cpp)

#Client executable
add_executable(client ${CMAKE_SOURCE_DIR}/src/client/client.cpp)
#compile with some debug symbol
#making sure i compile with c++ 20
target_compile_options(server PRIVATE -std=c++23 -Wall -Werror -Wextra -fno-inline -g -O0 )
target_compile_options(client PRIVATE -std=c++23 -Wall -Werror -Wextra -fno-inline -g -O0 )

##TUI
# include(FetchContent)   # to fetch repos online 
# set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
# FetchContent_Declare(ftxui 
  # GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  # GIT_TAG main #15587dad01a9ef0e5e79d97a16adf414f60669a5 
# )
# FetchContent_MakeAvailable(ftxui)
# set(SOURCES 
#   src/client/clientUI.cpp
#   src/client/Scroller.cpp) 
# # project(ftxuiLANGUAGES CXX VERSION )
#
# add_executable(clientUI ${SOURCES}) 
# target_link_libraries(clientUI
#   ftxui::screen
#   ftxui::dom
#   ftxui::component 
# )
#
##### Testing the online code from the ftxui website
# add_executable(test ${CMAKE_SOURCE_DIR}/test/test.cpp)
# target_link_libraries(test 
#   PRIVATE ftxui::screen
#   PRIVATE ftxui::dom
#   PRIVATE ftxui::component)
