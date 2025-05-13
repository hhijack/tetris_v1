#ifndef TETRIS_TYPES_H
#define TETRIS_TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

// 常量定义
#define ROWS 20
#define COLS 10
#define MAX_PIECES 7

// 方块类型枚举
typedef enum {
    I, T, O, J, L, S, Z
} PieceType;

// 旋转角度枚举
typedef enum {
    DEG_0,
    DEG_90,
    DEG_180,
    DEG_270
} Rotation;

// 方块结构体
typedef struct {
    int shape[4][4]; // 4x4 的方块形状
    int width;       // 当前旋转状态下的宽度
    int height;      // 当前旋转状态下的高度
} Piece;

// 游戏状态
typedef struct {
    int grid[ROWS][COLS]; // 游戏网格
    int score;            // 当前得分
    int top;              // 当前最高堆积高度
} GameState;

extern Piece pieces[MAX_PIECES][4]; // 外部声明全局变量

void init_pieces(); // 初始化所有方块的旋转状态

PieceType char_to_piece(char c); // 字符转方块类型

void init_pieces(); // 初始化所有方块的旋转状态

PieceType char_to_piece(char c); // 字符转方块类型

#endif // TETRIS_TYPES_H