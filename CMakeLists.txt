cmake_minimum_required(VERSION 3.6)
project(computernetworks)

set(CMAKE_CXX_FLAGS "-Wall -O2 ${CMAKE_CXX_FLAGS}")

add_executable(client client.cc datagram.h parse.h)
add_executable(server server.cc datagram.h parse.h)
