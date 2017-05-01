cmake_minimum_required(VERSION 3.6)
project(computernetworks)

set(CMAKE_CXX_STANDARD 14)

set(CLIENT_SOURCE_FILES
        client.cc
        private/err.c
        err.h
        server.cc)

set(SERVER_SOURCE_FILES
        server.cc
        private/err.c
        err.h)

add_executable(client ${CLIENT_SOURCE_FILES})
add_executable(server ${SERVER_SOURCE_FILES})