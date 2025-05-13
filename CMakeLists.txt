cmake_minimum_required(VERSION 3.28)
project(tetris_v1 C)

set(CMAKE_C_STANDARD 11)

add_executable(tetris_v1 main.c
)

add_executable(test_1 test.c
        strategy.c
        tetris.c
        test.c
        tetris_v3.c
)

add_executable(tetris_v2 tetris_v2_test.c)
add_executable(tetris_v3 tetris_v3.c)
