cmake_minimum_required(VERSION 4.2)
project(SkyControl C)

set(CMAKE_C_STANDARD 17)

add_executable(SkyControl src/main.c
        include/flight.h
        src/flight.c
        include/queue.h
        src/queue.c
        include/stack.h
        src/stack.c
        include/tree.h
        src/tree.c)
