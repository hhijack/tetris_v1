#include "rules.h"
#include "tetris.h"
#include <string.h>
#include <stdlib.h>

static inline int get_col_height(const GameState *state, int col) {
    for (int r = 0; r < ROWS; ++r) {
        if (state->grid[r][col]) return ROWS - r;
    }
    return 0;
}
int is_move_possible(const GameState *state, PieceType type, Rotation rotation, int x_col, int y_row_check) {
    const Piece *p = &pieces[type][rotation];
    if (x_col < 0 || x_col + p->width > COLS || y_row_check < 0 || y_row_check + p->height > ROWS) return 0;
    for (int r_offset = 0; r_offset < p->height; ++r_offset) {
        for (int c_offset = 0; c_offset < p->width; ++c_offset) {
            if (p->shape[r_offset][c_offset] && state->grid[y_row_check + r_offset][x_col + c_offset]) return 0;
        }
    }
    return 1;
}

// 更新游戏状态的顶部高度
static void update_board_top(GameState *state_to_update) {
    state_to_update->top = 0;
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            if (state_to_update->grid[r][c]) {
                state_to_update->top = ROWS - r;
                return;
            }
        }
    }
}

// 清除已填满的行并返回清除的行数
static int clear_filled_lines(GameState *state_to_update) {
    int cleared_count = 0;
    for (int r = ROWS - 1; r >= 0;) {
        int line_full = 1;
        for (int c = 0; c < COLS; ++c) {
            if (state_to_update->grid[r][c] == 0) {
                line_full = 0;
                break;
            }
        }
        if (line_full) {
            cleared_count++;
            for (int sr = r; sr > 0; --sr) {
                memcpy(state_to_update->grid[sr], state_to_update->grid[sr - 1], COLS * sizeof(int));
            }
            memset(state_to_update->grid[0], 0, COLS * sizeof(int));
            // 不递减r，重新检查当前行
        } else {
            r--;
        }
    }
    return cleared_count;
}

// 尝试将方块放置到指定位置并返回新的游戏状态
GameState attempt_to_place_piece(const GameState *initial_state, PieceType type, Rotation rotation, int x_col_target) {
    GameState resulting_state = *initial_state; // 复制初始状态
    const Piece *p = &pieces[type][rotation];
    int landing_row = -1;

    for (int y = 0; y <= ROWS - p->height; ++y) {
        if (is_move_possible(&resulting_state, type, rotation, x_col_target, y)) {
            landing_row = y;
        } else {
            break;
        }
    }

    if (landing_row == -1) {
        resulting_state.top = ROWS + 1; // 游戏结束
        return resulting_state;
    }

    for (int r_offset = 0; r_offset < p->height; ++r_offset) {
        for (int c_offset = 0; c_offset < p->width; ++c_offset) {
            if (p->shape[r_offset][c_offset]) {
                if (landing_row + r_offset >= 0 && landing_row + r_offset < ROWS &&
                    x_col_target + c_offset >= 0 && x_col_target + c_offset < COLS) {
                    resulting_state.grid[landing_row + r_offset][x_col_target + c_offset] = 1;
                }
            }
        }
    }

    int lines_cleared_count = clear_filled_lines(&resulting_state);
    if (lines_cleared_count == 1) resulting_state.score += 100;
    else if (lines_cleared_count == 2) resulting_state.score += 300;
    else if (lines_cleared_count == 3) resulting_state.score += 500;
    else if (lines_cleared_count >= 4) resulting_state.score += 800; // 使用>=4以确保安全

    update_board_top(&resulting_state);
    return resulting_state;
}


static void compute_board_metrics(const GameState *state, BoardMetrics *metrics) {
    memset(metrics, 0, sizeof(BoardMetrics)); // 初始化所有为0

    for (int c = 0; c < COLS; ++c) {
        metrics->heights[c] = get_col_height(state, c); // 使用辅助函数
        metrics->aggregate_h += metrics->heights[c];
        if (metrics->heights[c] > metrics->max_h) metrics->max_h = metrics->heights[c];
        if (c >= 3 && c <= 6) metrics->center_h_bonus += metrics->heights[c];
    }

    int temp_hole_map[ROWS][COLS] = {0};
    for (int c = 0; c < COLS; ++c) {
        int block_found = 0;
        for (int r = 0; r < ROWS; ++r) {
            if (state->grid[r][c]) block_found = 1;
            else if (block_found) {
                metrics->num_holes++;
                temp_hole_map[r][c] = 1;
            }
        }
    }

    int temp_visited[ROWS][COLS] = {0};
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            if (temp_hole_map[r][c] && !temp_visited[r][c]) {
                metrics->adj_holes++; // 仍然只是一个简单的计数，没有BFS/DFS
                // BFS/DFS标记访问过的部分会在这里
            }
        }
    }

    for (int c = 0; c < COLS - 1; ++c) metrics->bump += abs(metrics->heights[c] - metrics->heights[c + 1]);

    for (int r = 0; r < ROWS; ++r) {
        int line_full = 1;
        int blocks_in_line = 0;
        for (int c = 0; c < COLS; ++c) {
            if (state->grid[r][c]) blocks_in_line++; else line_full = 0;
        }
        if (line_full) metrics->complete_ls++;
        if (blocks_in_line >= 7 && !line_full) metrics->potential_ls++;
    }

    for (int c = 0; c < COLS; ++c) {
        if (COLS == 1) break;
        int cur_h = metrics->heights[c];
        if (c == 0) { if (metrics->heights[c + 1] >= cur_h + 4) metrics->i_channels++; }
        else if (c == COLS - 1) { if (metrics->heights[c - 1] >= cur_h + 4) metrics->i_channels++; }
        else {
            if (metrics->heights[c - 1] >= cur_h + 4 && metrics->heights[c + 1] >= cur_h + 4)
                metrics->i_channels++;
        }
    }
}

// 计算当前游戏状态的评分
long long int calculate_state_score(const GameState *current_state_eval) {
    if (current_state_eval->top > ROWS) return LLONG_MIN + 10; // 游戏结束时的特殊评分

    BoardMetrics metrics;
    compute_board_metrics(current_state_eval, &metrics);

    long long final_score_val = (long long) current_state_eval->score * 15;
    final_score_val += (-30LL * metrics.aggregate_h);
    final_score_val += (80LL * metrics.complete_ls);
    final_score_val += (-75LL * metrics.num_holes);
    final_score_val += (-20LL * metrics.adj_holes); // 使用adj_holes作为计算值
    final_score_val += (-15LL * metrics.bump);
    final_score_val += (5LL * metrics.center_h_bonus);
    final_score_val += (50LL * metrics.i_channels);
    final_score_val += (25LL * metrics.potential_ls);

    if (current_state_eval->top >= 16 && current_state_eval->top <= ROWS) {
        final_score_val -= 500000LL;
    }

    if (final_score_val > INT_MAX) final_score_val = INT_MAX;
    if (final_score_val < LLONG_MIN + 10 && current_state_eval->top <= ROWS) final_score_val = LLONG_MIN + 10;

    return (long) final_score_val;
}
