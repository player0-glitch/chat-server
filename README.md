
# Chat Server

A traditional client-server with TCP/IP protocol where the server act as a middle-man that broadcast client messages to other connected clients.

## Table of content
1 [Installation](#Installation)

2 [Usage](#Usage)

3 [Acknowledgements](#Acknowledgments)

## Installation
### Pre-requisites

- CMake 
- C++

### Steps

To get started, clone this repo onto your local machine. Make sure your have the necessary dependencies.

```bash
git clone https://github.com/player0-glitch/chat-server
mkdir build
```
### Note
You can run CMake how you see fit, this is just my recommendation as this was developed on Linux (Windows and MacOS were not tested).

```bash
#Recommended command to let your system choose a build tool 
cmake . -B build

#To use Makefiles as your build tool
cmake -G "Unix Makefiles" -B build

#To use Ninja as your build tool 
cmake -G "Ninja" -B build
make 
```
This build both the client and the server into the build directory.

Make sure to run the server first. The server runs on any available network inteface on the local machine, [see IPADDR_ANY](https://man7.org/linux/man-pages/man7/ip.7.html#:~:text=When%20INADDR_ANY%20is%0A%20%20%20%20%20%20%20specified,set%0A%20%20%20%20%20%20%20to%20INADDR_ANY.)
To run the client 3 command-line arguements are needed
```bash
./client <user-name> <server IP address> <port>
```





