cmake_minimum_required(VERSION 3.28)
project(Tetris C)

set(CMAKE_C_STANDARD 11)

add_executable(tetris Tetris/main.c
        Tetris/tetris.c
        Tetris/rules.c
        Tetris/strategy.c)
add_executable(tetris_v1 tetris_v1.c)
add_executable(tetris_v3 tetris_v3.c)
add_executable(tetris_v2 tetris_v2.c)
