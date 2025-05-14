#ifndef TETRIS_GAME_H
#define TETRIS_GAME_H

#include "tetris.h"

typedef struct {
    Rotation rot;
    int x;
    long long score; // 综合评分
    long long current_eval; // 当前方块放置后的评分
    long long next_eval;    // 下一个方块放置后的评分（最佳情况）
} MoveOption;

void select_best_action(GameState *current_board_state, PieceType current_p, PieceType next_p, Rotation *selected_rotation,
                        int *selected_x_col);
void suicide_place_piece(GameState *state, PieceType type, Rotation *rot, int *x_col);
#endif // TETRIS_GAME_H
