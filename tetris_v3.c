#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define ROWS 15
#define COLS 10
#define MAX_PIECES 7
#define SHAPE_DIM 4 // Max dimension for piece shape array

// 方块类型和旋转角度
typedef enum {
    I, T, O, J, L, S, Z
} PieceType;
typedef enum {
    DEG_0, DEG_90, DEG_180, DEG_270
} Rotation;

typedef struct {
    int shape[SHAPE_DIM][SHAPE_DIM];
    int width;
    int height;
} Piece;

typedef struct {
    int grid[ROWS][COLS];
    int score;
    int top; // 0 if empty, ROWS if full at top, ROWS+1 for game over by placement
} GameState;

Piece pieces[MAX_PIECES][4];

// --- 函数声明 ---
void init_pieces_definitions(void); // Renamed for clarity
PieceType convert_char_to_piece_type(char c); // Renamed
int is_move_possible(const GameState *state, PieceType type, Rotation rotation, int x_col, int y_row_check); // Renamed
GameState
attempt_to_place_piece(const GameState *initial_state, PieceType type, Rotation rotation, int x_col_target); // Renamed
long calculate_state_score(const GameState *current_state_eval); // Renamed
void
select_best_action(GameState *current_board_state, PieceType current_p, PieceType next_p, Rotation *selected_rotation,
                   int *selected_x_col); // Renamed
void suicide_place_piece(GameState *state, PieceType type, Rotation *rot, int *x_col); // Added

// --- 辅助函数 (一些被声明为 static inline) ---
static inline int get_col_height(const GameState *state, int col) {
    for (int r = 0; r < ROWS; ++r) {
        if (state->grid[r][col]) return ROWS - r;
    }
    return 0;
}

// 主函数 - 游戏逻辑入口
int main() {
    init_pieces_definitions();

    GameState current_game_board;
    memset(&current_game_board, 0, sizeof(current_game_board));

    char line_buffer[30]; // 用于读取输入的缓冲区
    if (!fgets(line_buffer, sizeof(line_buffer), stdin)) return 1;
    line_buffer[strcspn(line_buffer, "\n")] = 0;

    if (strlen(line_buffer) == 0) return 1;
    if (line_buffer[0] == 'X') return 0;
    if (line_buffer[0] == 'E') exit(0);

    PieceType active_piece = convert_char_to_piece_type(line_buffer[0]);
    PieceType upcoming_piece = (strlen(line_buffer) >= 2 && line_buffer[1] != 'X' && line_buffer[1] != 'E')
                               ? convert_char_to_piece_type(line_buffer[1]) : I;

    int suicide_mode = 0; // 你可以通过某种方式设置它，比如超时检测
    clock_t start_time = clock(); // 记录起始时间

    for (;;) { // 主循环
        // 检查是否超时（9.8秒=9800毫秒）
        clock_t now = clock();
        double elapsed = (double) (now - start_time) / CLOCKS_PER_SEC;
        if (elapsed > 9.8) {
            suicide_mode = 1;
            //printf("已超时\n");
        }

        Rotation chosen_rotation = DEG_0;
        int chosen_x_col = 0;

        if (suicide_mode) {
            suicide_place_piece(&current_game_board, active_piece, &chosen_rotation, &chosen_x_col);
        } else {
            select_best_action(&current_game_board, active_piece, upcoming_piece, &chosen_rotation, &chosen_x_col);
        }

        GameState state_before_move = current_game_board;
        current_game_board = attempt_to_place_piece(&current_game_board, active_piece, chosen_rotation, chosen_x_col);

        printf("%d %d\n", chosen_rotation, chosen_x_col);
        printf("%d\n", current_game_board.score);
        fflush(stdout);

        char next_command_char;
        if (scanf(" %c", &next_command_char) != 1) break; // 检查EOF

        if (next_command_char == 'X') {
            Rotation final_r;
            int final_x;
            GameState state_before_final_X_move = current_game_board;
            select_best_action(&current_game_board, active_piece, I, &final_r, &final_x); // 使用I作为下一个方块的占位符
            current_game_board = attempt_to_place_piece(&current_game_board, active_piece, final_r, final_x);

            printf("%d %d\n", final_r, final_x);
            printf("%d\n", current_game_board.score);
            fflush(stdout);
            break;
        }
        if (next_command_char == 'E') exit(0);

        active_piece = upcoming_piece;
        upcoming_piece = convert_char_to_piece_type(next_command_char);
    }
    return 0;
}

// --- 函数实现 ---
void init_pieces_definitions(void) {

    // I 型方块
    // 0度: 1111 (宽4, 高1)
    int i_shape[4][4][4] = {
            {{1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 0 度
            {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}, // 90 度 (旋转后，宽1, 高4)
            {{1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 180 度 (同0度形状，但可以是不同定义)
            {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}  // 270 度 (同90度形状)
    };
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[I][r].shape, i_shape[r], sizeof(i_shape[r]));
        pieces[I][r].width = (r == DEG_0 || r == DEG_180) ? 4 : 1;
        pieces[I][r].height = (r == DEG_0 || r == DEG_180) ? 1 : 4;
    }

    // T 型方块
    // 0度: (宽3, 高2)
    // 010
    // 111
    int t_shape[4][4][4] = {
            {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 0 度
            {{1, 0, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}}, // 90 度 (杆在左，宽2, 高3)
            {{1, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 180 度 (倒T，宽3, 高2)
            {{0, 1, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}  // 270 度 (杆在右，宽2, 高3)
    };
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[T][r].shape, t_shape[r], sizeof(t_shape[r]));
        pieces[T][r].width = (r == DEG_0 || r == DEG_180) ? 3 : 2;
        pieces[T][r].height = (r == DEG_0 || r == DEG_180) ? 2 : 3;
    }

    // O 型方块
    // 0度: (宽2, 高2)
    // 11
    // 11
    int o_shape[4][4] = {{1, 1, 0, 0},
                         {1, 1, 0, 0},
                         {0, 0, 0, 0},
                         {0, 0, 0, 0}};
    for (int r = 0; r < 4; r++) // O型旋转不变
    {
        memcpy(pieces[O][r].shape, o_shape, sizeof(o_shape));
        pieces[O][r].width = 2;
        pieces[O][r].height = 2;
    }

    // J 型方块
    // 0度: (宽3, 高2)
    // 100
    // 111
    int j_shape[4][4][4] = {
            {{1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 0 度
            {{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}}, // 90 度 (尾巴朝下，宽2, 高3)
            {{1, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 180 度 (倒J，宽3, 高2)
            {{0, 1, 0, 0}, {0, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}}  // 270 度 (尾巴朝上，宽2, 高3)
    };
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[J][r].shape, j_shape[r], sizeof(j_shape[r]));
        pieces[J][r].width = (r == DEG_0 || r == DEG_180) ? 3 : 2;
        pieces[J][r].height = (r == DEG_0 || r == DEG_180) ? 2 : 3;
    }

    // L 型方块
    // 0度: (宽3, 高2)
    // 001
    // 111
    int l_shape[4][4][4] = {
            {{0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 0 度
            {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}}, // 90 度 (尾巴朝下，宽2, 高3)
            {{1, 1, 1, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 180 度 (倒L，宽3, 高2)
            {{1, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}  // 270 度 (尾巴朝上，宽2, 高3)
    };
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[L][r].shape, l_shape[r], sizeof(l_shape[r]));
        pieces[L][r].width = (r == DEG_0 || r == DEG_180) ? 3 : 2;
        pieces[L][r].height = (r == DEG_0 || r == DEG_180) ? 2 : 3;
    }

    // S 型方块
    // 0度: (宽3, 高2)
    // 011
    // 110
    int s_shape[4][4][4] = {
            {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 0 度
            {{1, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}, // 90 度 (竖直，宽2, 高3)
            {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 180 度 (同0度形状，S型180度旋转后形状不变)
            {{1, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}  // 270 度 (同90度形状)
    };
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[S][r].shape, s_shape[r], sizeof(s_shape[r]));
        pieces[S][r].width = (r == DEG_0 || r == DEG_180) ? 3 : 2;
        pieces[S][r].height = (r == DEG_0 || r == DEG_180) ? 2 : 3;
    }

    // Z 型方块
    // 0度: (宽3, 高2)
    // 110
    // 011
    int z_shape[4][4][4] = {
            {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 0 度
            {{0, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}}, // 90 度 (竖直，宽2, 高3)
            {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 180 度 (同0度形状，Z型180度旋转后形状不变)
            {{0, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}}  // 270 度 (同90度形状)
    };
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[Z][r].shape, z_shape[r], sizeof(z_shape[r]));
        pieces[Z][r].width = (r == DEG_0 || r == DEG_180) ? 3 : 2;
        pieces[Z][r].height = (r == DEG_0 || r == DEG_180) ? 2 : 3;
    }
}

PieceType convert_char_to_piece_type(char c) {
    switch (c) {
        case 'I':
            return I;
        case 'T':
            return T;
        case 'O':
            return O;
        case 'J':
            return J;
        case 'L':
            return L;
        case 'S':
            return S;
        case 'Z':
            return Z;
        default:
            return I;
    }
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


// --- evaluate_state (calculate_state_score) 函数实现 ---
typedef struct {
    int heights[COLS];
    int aggregate_h;
    int max_h;
    int num_holes;
    int bump;
    int adj_holes; // 简化，需要BFS/DFS进行真实计数
    int center_h_bonus;
    int i_channels;
    int potential_ls;
    int complete_ls;
} BoardMetrics;

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
long calculate_state_score(const GameState *current_state_eval) {
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


// --- select_best_action (find_best_move) 函数实现 ---
typedef struct {
    Rotation rot;
    int x;
    long long score; // 综合评分
    long long current_eval; // 当前方块放置后的评分
    long long next_eval;    // 下一个方块放置后的评分（最佳情况）
} MoveOption;


static int compare_move_options(const void *a, const void *b) {
    const MoveOption *opt_a = (const MoveOption *) a;
    const MoveOption *opt_b = (const MoveOption *) b;
    if (opt_b->score > opt_a->score) return 1;
    if (opt_b->score < opt_a->score) return -1;
    // 破局
    if (opt_a->rot < opt_b->rot) return -1;
    if (opt_a->rot > opt_b->rot) return 1;
    // 进一步的破局可以通过x（更靠近中心）来添加，如果需要
    // 为了简单，目前如果评分相等则只通过旋转来破局
    return 0;
}


// 选择最佳行动（旋转和列位置）
void
select_best_action(GameState *current_board_state, PieceType current_p, PieceType next_p, Rotation *selected_rotation,
                   int *selected_x_col) {
    MoveOption candidate_moves[4 * COLS]; // 最大可能的移动数量
    int num_candidate_moves = 0;

    const float W_CUR = 1.0f;
    const float W_NXT = 0.85f;
    float p_bonus[MAX_PIECES] = {300, 200, 100, 150, 150, 50, 50}; // I,T,O,J,L,S,Z 顺序

    for (int r_idx = 0; r_idx < 4; ++r_idx) {
        Rotation current_r = (Rotation) r_idx;
        const Piece *p_shape = &pieces[current_p][current_r];
        for (int x_try = 0; x_try <= COLS - p_shape->width; ++x_try) {
            int can_spawn_curr = 0;
            for (int y_s = 0; y_s <= ROWS - p_shape->height; ++y_s) {
                if (is_move_possible(current_board_state, current_p, current_r, x_try, y_s)) {
                    can_spawn_curr = 1;
                    break;
                }
            }
            if (!can_spawn_curr && p_shape->height > 0) continue;

            GameState state_after_curr = attempt_to_place_piece(current_board_state, current_p, current_r, x_try);
            long long eval_curr = calculate_state_score(&state_after_curr);
            if (eval_curr > LLONG_MIN + 9) eval_curr += p_bonus[current_p];

            long long best_eval_nxt = LLONG_MIN;
            int found_nxt_move = 0;

            if (state_after_curr.top <= ROWS) {
                for (int next_r_idx = 0; next_r_idx < 4; ++next_r_idx) {
                    Rotation next_r = (Rotation) next_r_idx;
                    const Piece *next_p_shape = &pieces[next_p][next_r];
                    for (int next_x_try = 0; next_x_try <= COLS - next_p_shape->width; ++next_x_try) {
                        int can_spawn_nxt_p = 0;
                        for (int y_ns = 0; y_ns <= ROWS - next_p_shape->height; ++y_ns) {
                            if (is_move_possible(&state_after_curr, next_p, next_r, next_x_try, y_ns)) {
                                can_spawn_nxt_p = 1;
                                break;
                            }
                        }
                        if (!can_spawn_nxt_p && next_p_shape->height > 0) continue;

                        GameState state_after_nxt = attempt_to_place_piece(&state_after_curr, next_p, next_r,
                                                                           next_x_try);
                        long long eval_nxt_move = calculate_state_score(&state_after_nxt);
                        if (eval_nxt_move > LLONG_MIN + 9) eval_nxt_move += p_bonus[next_p];

                        if (!found_nxt_move || eval_nxt_move > best_eval_nxt) {
                            best_eval_nxt = eval_nxt_move;
                            found_nxt_move = 1;
                        }
                    }
                }
            } else {
                best_eval_nxt = eval_curr; // 当前移动导致游戏结束
                found_nxt_move = 1;
            }

            MoveOption current_opt;
            current_opt.rot = current_r;
            current_opt.x = x_try;
            current_opt.current_eval = eval_curr;
            current_opt.next_eval = best_eval_nxt;

            if (found_nxt_move) {
                if (best_eval_nxt == LLONG_MIN && state_after_curr.top <= ROWS) { // 当前移动可行，但没有下一个移动
                    current_opt.next_eval = eval_curr - 500000LL; // 处罚
                }
                current_opt.score = (long long) (W_CUR * current_opt.current_eval + W_NXT * current_opt.next_eval);
            } else { // 当前移动可行，但没有任何下一个移动
                current_opt.score = eval_curr - 500000LL;
            }

            if (num_candidate_moves < 4 * COLS) { // 确保不超出数组范围
                candidate_moves[num_candidate_moves++] = current_opt;
            }
        }
    }

    if (num_candidate_moves == 0) { // 没有找到任何有效移动（例如，起始时棋盘完全被堵住）
        *selected_rotation = DEG_0;
        const Piece *def_p = &pieces[current_p][DEG_0];
        *selected_x_col = (def_p->width <= COLS) ? (COLS - def_p->width) / 2 : 0;
        if (*selected_x_col < 0) *selected_x_col = 0;
        return;
    }

    // 排序移动以找到最佳的一个，根据评分和破局规则
    long long final_best_score = LLONG_MIN;
    int best_idx = -1;

    for (int i = 0; i < num_candidate_moves; ++i) {
        if (i == 0 || candidate_moves[i].score > final_best_score) {
            final_best_score = candidate_moves[i].score;
            *selected_rotation = candidate_moves[i].rot;
            *selected_x_col = candidate_moves[i].x;
            best_idx = i;
        } else if (candidate_moves[i].score == final_best_score) {
            if (candidate_moves[i].rot < *selected_rotation) {
                *selected_rotation = candidate_moves[i].rot;
                *selected_x_col = candidate_moves[i].x;
                best_idx = i;
            } else if (candidate_moves[i].rot == *selected_rotation) {
                const Piece *p_shape_cand = &pieces[current_p][candidate_moves[i].rot];
                const Piece *p_shape_best = &pieces[current_p][*selected_rotation];
                int cand_dist = abs(candidate_moves[i].x + p_shape_cand->width / 2 - COLS / 2);
                int best_dist = abs(*selected_x_col + p_shape_best->width / 2 - COLS / 2);
                if (cand_dist < best_dist) {
                    *selected_x_col = candidate_moves[i].x;
                    best_idx = i;
                }
            }
        }
    }
    if (best_idx == -1 && num_candidate_moves > 0) { // 如果没有设置最佳索引但存在移动，则回退
        *selected_rotation = candidate_moves[0].rot;
        *selected_x_col = candidate_moves[0].x;
    }
}

void suicide_place_piece(GameState *state, PieceType type, Rotation *rot, int *x_col) {
    for (int r = 0; r < 4; ++r) {
        const Piece *p = &pieces[type][r];
        for (int x = 0; x <= COLS - p->width; ++x) {
            // 尽量往左、往上放
            for (int y = 0; y <= ROWS - p->height; ++y) {
                if (is_move_possible(state, type, r, x, y)) {
                    *rot = r;
                    *x_col = x;
                    return;
                }
            }
        }
    }
    // 如果所有都放不下，默认返回0,0
    *rot = 0;
    *x_col = 0;
}
