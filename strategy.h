#ifndef TETRIS_GAME_H
#define TETRIS_GAME_H

#include "tetris.h"

int can_place_piece(const GameState *state, PieceType type, Rotation rotation, int x_col, int y_row);
GameState place_piece(const GameState *state, PieceType type, Rotation rotation, int x_col);
int evaluate_state(const GameState *state);
void find_best_move(GameState *current_state, PieceType type, Rotation *best_rot, int *best_x);

#endif // TETRIS_GAME_H
