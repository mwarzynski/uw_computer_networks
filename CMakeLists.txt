cmake_minimum_required(VERSION 3.6)
project(computernetworks)

set(CMAKE_CXX_FLAGS "-Wall -O2 ${CMAKE_CXX_FLAGS}")

include_directories(include)

add_executable(client src/client.cc include/datagram.h include/parse.h)
add_executable(server src/server.cc include/datagram.h include/parse.h)
