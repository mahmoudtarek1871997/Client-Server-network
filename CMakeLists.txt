cmake_minimum_required(VERSION 3.8)
project(Client_Server_HTTP)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES Client.cpp Client.h Server/Server.cpp Server/Server.h)
add_executable(Client_Server_HTTP ${SOURCE_FILES})