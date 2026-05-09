cmake_minimum_required(VERSION 4.2)
project(SkyControl C)

set(CMAKE_C_STANDARD 17)

add_executable(SkyControl src/main.c
        include/flight.h)
