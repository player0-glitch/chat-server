cmake_minimum_required(VERSION 3.30.5)
project(chat-server)
set(CMAKE_CXX_STANDARD 20)
#Clangd compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Server executable
add_executable(server ${CMAKE_SOURCE_DIR}/src/server.cpp)

#Client executable
add_executable(client ${CMAKE_SOURCE_DIR}/src/client.cpp)
#compile with some debug symbols
target_compile_options(server PRIVATE -Wall -Werror -g -O0)

#source files
