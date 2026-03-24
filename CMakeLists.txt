cmake_minimum_required(VERSION 3.20)
project(chat-server)
set(CMAKE_CXX_STANDARD 23)
#Clangd compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# target_include_directories(${CMAKE_SOURCE_DIR}/include)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/)
#set the common libraries for the whole project
# file(GLOB COMMON_SRC "${CMAKE_SOURCE_DIR}/src/common/*.cpp")
set(GLOB COMMON_SRC "${CMAKE_SOURCE_DIR}/src/common/*.cpp")
target_include_directories(common_code PUBLIC {COMMON_SRC})
#Static linking. I dont want DLLs
add_library(common_code STATIC ${COMMON_SRC})


#include directories

#define server and client sources  
file(GLOB SERVER_SRC "${CMAKE_SOURCE_DIR}/src/server/*.cpp")
file(GLOB CLIENT_SRC "${CMAKE_SOURCE_DIR}/src/client/*.cpp")

target_include_directories(common_code PRIVATE {COMMON_SRC})
#Server 
add_executable(server ${SERVER_SRC})
target_include_directories(server PRIVATE {COMMON_SRC})
add_executable(client ${SERVER_SRC})
target_include_directories(client PRIVATE {COMMON_SRC})

#making sure i compile with c++ 23
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

