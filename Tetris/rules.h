#ifndef TETRIS_RULES_H
#define TETRIS_RULES_H
#include <limits.h>
#include "tetris.h"

typedef struct {
    int heights[COLS];
    int aggregate_h;
    int max_h;
    int num_holes;
    int bump;
    int adj_holes;
    int center_h_bonus;
    int i_channels;
    int potential_ls;
    int complete_ls;
} BoardMetrics;

int is_move_possible(const GameState *state, PieceType type, Rotation rotation, int x_col, int y_row_check); // Renamed
GameState
attempt_to_place_piece(const GameState *initial_state, PieceType type, Rotation rotation, int x_col_target); // Renamed
long long int calculate_state_score(const GameState *current_state_eval); // Renamed

#endif //TETRIS_V1_RULES_H
