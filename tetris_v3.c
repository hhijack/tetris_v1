#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> // For LONG_MIN, INT_MAX, INT_MIN
#include <math.h>   // 用于 evaluate_state 中的 abs 函数


#define ROWS 20
#define COLS 10
#define MAX_PIECES 7

// 方块类型枚举
typedef enum
{
    I,
    T,
    O,
    J,
    L,
    S,
    Z
} PieceType;

// 旋转角度枚举
typedef enum
{
    DEG_0,
    DEG_90,
    DEG_180,
    DEG_270
} Rotation;

// 方块结构体
typedef struct
{
    int shape[4][4]; // 4x4 的方块形状
    int width;       // 当前旋转状态下的宽度
    int height;      // 当前旋转状态下的高度
} Piece;

// 游戏状态
typedef struct
{
    int grid[ROWS][COLS]; // 游戏网格
    int score;            // 当前得分
    int top;              // 当前最高堆积高度 (从底部开始的行数，为空时为 0, 即 ROWS - 最高方块所在行索引)
} GameState;

// 评估函数权重结构体
typedef struct {
    int score_multiplier;
    int aggregate_height_weight;    // 总高度惩罚
    int complete_lines_weight;      // 消行奖励 (乘以消行中的格子数)
    int holes_weight;               // 空洞惩罚
    int covered_holes_weight;       // 被覆盖的空洞惩罚 (更严重)
    int bumpiness_weight;           // 崎岖度惩罚
    int row_transitions_weight;     // 行变换惩罚
    int col_transitions_weight;     // 列变换惩罚
    int well_sum_sq_weight;         // 井的深度平方和惩罚
    int game_over_penalty_threshold;// 触发游戏结束惩罚的高度阈值
    int game_over_penalty_value;    // 游戏结束惩罚值
    int max_height_penalty_weight;  // 最大列高惩罚
} EvalWeights;


// 预定义所有方块的所有旋转状态
Piece pieces[MAX_PIECES][4]; // 7 种方块，每种 4 种旋转

// 示例权重，需要仔细调优！
const EvalWeights DEFAULT_WEIGHTS = {
        1,      // score_multiplier
        -50,    // aggregate_height_weight
        80,     // complete_lines_weight
        -120,   // holes_weight
        -250,   // covered_holes_weight (比普通洞更严重)
        -25,    // bumpiness_weight
        -30,    // row_transitions_weight
        -30,    // col_transitions_weight
        -90,    // well_sum_sq_weight
        16,     // game_over_penalty_threshold
        -500000,// game_over_penalty_value
        -10     // max_height_penalty_weight
};


/* 函数声明 */
void init_pieces(void);
PieceType char_to_piece(char c);
int can_place_piece(const GameState *state, PieceType type, Rotation rotation, int x_col, int y_row);
GameState place_piece(const GameState *state, PieceType type, Rotation rotation, int x_col);
int evaluate_state_with_weights(const GameState *state, const EvalWeights *weights);
void find_best_move(GameState *current_state, PieceType current_piece, PieceType next_piece, Rotation *best_rot, int *best_x);


void init_pieces()
{
    // I 型方块
    int i_shape_data[4][4][4] = {
            {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
            {{0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0}},
            {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
            {{0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0}}
    };
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[I][r].shape, i_shape_data[r], sizeof(i_shape_data[r]));
        if (r == DEG_0 || r == DEG_180) {
            pieces[I][r].width = 4; pieces[I][r].height = 1;
            int temp_shape[4][4]; memcpy(temp_shape, pieces[I][r].shape, sizeof(temp_shape));
            memset(pieces[I][r].shape, 0, sizeof(pieces[I][r].shape));
            memcpy(pieces[I][r].shape[0], temp_shape[1], 4 * sizeof(int));
        } else {
            pieces[I][r].width = 1; pieces[I][r].height = 4;
            int temp_shape[4][4]; memcpy(temp_shape, pieces[I][r].shape, sizeof(temp_shape));
            memset(pieces[I][r].shape, 0, sizeof(pieces[I][r].shape));
            for(int i=0; i<4; ++i) pieces[I][r].shape[i][0] = temp_shape[i][1];
        }
    }

    // T 型方块
    int t_shape_data[4][4][4] = {
            {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}, {{1,0,0,0}, {1,1,0,0}, {1,0,0,0}, {0,0,0,0}},
            {{1,1,1,0}, {0,1,0,0}, {0,0,0,0}, {0,0,0,0}}, {{0,1,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0}}
    };
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[T][r].shape, t_shape_data[r], sizeof(t_shape_data[r]));
        pieces[T][r].width = (r == DEG_0 || r == DEG_180) ? 3 : 2;
        pieces[T][r].height = (r == DEG_0 || r == DEG_180) ? 2 : 3;
    }

    // O 型方块
    int o_shape_data[4][4] = {{1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}};
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[O][r].shape, o_shape_data, sizeof(o_shape_data));
        pieces[O][r].width = 2; pieces[O][r].height = 2;
    }

    // J 型方块
    int j_shape_data[4][4][4] = {
            {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}, {{1,1,0,0}, {1,0,0,0}, {1,0,0,0}, {0,0,0,0}},
            {{1,1,1,0}, {0,0,1,0}, {0,0,0,0}, {0,0,0,0}}, {{0,1,0,0}, {0,1,0,0}, {1,1,0,0}, {0,0,0,0}}
    };
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[J][r].shape, j_shape_data[r], sizeof(j_shape_data[r]));
        pieces[J][r].width = (r == DEG_0 || r == DEG_180) ? 3 : 2;
        pieces[J][r].height = (r == DEG_0 || r == DEG_180) ? 2 : 3;
    }

    // L 型方块
    int l_shape_data[4][4][4] = {
            {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}, {{1,0,0,0}, {1,0,0,0}, {1,1,0,0}, {0,0,0,0}},
            {{1,1,1,0}, {1,0,0,0}, {0,0,0,0}, {0,0,0,0}}, {{1,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0}}
    };
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[L][r].shape, l_shape_data[r], sizeof(l_shape_data[r]));
        pieces[L][r].width = (r == DEG_0 || r == DEG_180) ? 3 : 2;
        pieces[L][r].height = (r == DEG_0 || r == DEG_180) ? 2 : 3;
    }

    // S 型方块
    int s_shape_data[4][4][4] = {
            {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}}, {{1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0}},
            {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}}, {{1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0}}
    };
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[S][r].shape, s_shape_data[r], sizeof(s_shape_data[r]));
        pieces[S][r].width = (r == DEG_0 || r == DEG_180) ? 3 : 2;
        pieces[S][r].height = (r == DEG_0 || r == DEG_180) ? 2 : 3;
    }

    // Z 型方块
    int z_shape_data[4][4][4] = {
            {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}, {{0,1,0,0}, {1,1,0,0}, {1,0,0,0}, {0,0,0,0}},
            {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}, {{0,1,0,0}, {1,1,0,0}, {1,0,0,0}, {0,0,0,0}}
    };
    for (int r = 0; r < 4; r++) {
        memcpy(pieces[Z][r].shape, z_shape_data[r], sizeof(z_shape_data[r]));
        pieces[Z][r].width = (r == DEG_0 || r == DEG_180) ? 3 : 2;
        pieces[Z][r].height = (r == DEG_0 || r == DEG_180) ? 2 : 3;
    }
}

PieceType char_to_piece(char c) {
    switch (c) {
        case 'I': return I; case 'T': return T; case 'O': return O;
        case 'J': return J; case 'L': return L; case 'S': return S;
        case 'Z': return Z; default: return I;
    }
}

int can_place_piece(const GameState *state, PieceType type, Rotation rotation, int x_col, int y_row) {
    const Piece *piece = &pieces[type][rotation];
    if (x_col < 0 || x_col + piece->width > COLS || y_row < 0 || y_row + piece->height > ROWS) return 0;
    for (int py = 0; py < piece->height; py++) {
        for (int px = 0; px < piece->width; px++) {
            if (piece->shape[py][px]) {
                if (y_row + py >= ROWS || x_col + px >= COLS || state->grid[y_row + py][x_col + px]) return 0;
            }
        }
    }
    return 1;
}

GameState place_piece(const GameState *state, PieceType type, Rotation rotation, int x_col) {
    GameState new_state = *state;
    const Piece *piece = &pieces[type][rotation];
    int y_drop_row = -1;

    for (int y_try = 0; y_try <= ROWS - piece->height; y_try++) {
        if (can_place_piece(&new_state, type, rotation, x_col, y_try)) y_drop_row = y_try;
        else break;
    }

    if (y_drop_row != -1) {
        for (int py = 0; py < piece->height; py++) {
            for (int px = 0; px < piece->width; px++) {
                if (piece->shape[py][px]) {
                    if (y_drop_row + py < ROWS && x_col + px < COLS)
                        new_state.grid[y_drop_row + py][x_col + px] = 1;
                }
            }
        }
    } else {
        new_state.top = ROWS + 1; // Mark as game over if cannot place
        return new_state;
    }

    int lines_cleared_count = 0;
    int write_row = ROWS - 1;
    for (int read_row = ROWS - 1; read_row >= 0; read_row--) {
        int is_full = 1;
        for (int c = 0; c < COLS; c++) {
            if (new_state.grid[read_row][c] == 0) {
                is_full = 0;
                break;
            }
        }
        if (!is_full) {
            if (write_row != read_row) {
                memcpy(new_state.grid[write_row], new_state.grid[read_row], COLS * sizeof(int));
            }
            write_row--;
        } else {
            lines_cleared_count++;
        }
    }
    for (int r = write_row; r >= 0; r--) {
        memset(new_state.grid[r], 0, COLS * sizeof(int));
    }

    switch (lines_cleared_count) {
        case 1: new_state.score += 100; break;
        case 2: new_state.score += 300; break;
        case 3: new_state.score += 500; break;
        case 4: new_state.score += 800; break;
    }

    new_state.top = 0;
    int found_top_block = 0;
    for (int r_idx = 0; r_idx < ROWS; r_idx++) {
        for (int c_idx = 0; c_idx < COLS; c_idx++) {
            if (new_state.grid[r_idx][c_idx]) {
                new_state.top = ROWS - r_idx;
                found_top_block = 1;
                break;
            }
        }
        if (found_top_block) break;
    }
    return new_state;
}

int evaluate_state_with_weights(const GameState *state, const EvalWeights *weights) {
    if (state->top > ROWS) return INT_MIN + 100;

    long long eval_score = (long long)state->score * weights->score_multiplier;
    int aggregate_height = 0;
    int max_col_height = 0;
    int heights[COLS] = {0};
    int holes = 0;
    int covered_holes = 0;
    int bumpiness = 0;
    int row_transitions = 0;
    int col_transitions = 0;
    int well_sum_sq = 0;

    for (int c = 0; c < COLS; c++) {
        heights[c] = 0;
        for (int r = 0; r < ROWS; r++) {
            if (state->grid[r][c]) {
                heights[c] = ROWS - r;
                break;
            }
        }
        aggregate_height += heights[c];
        if (heights[c] > max_col_height) max_col_height = heights[c];
    }

    for (int c = 0; c < COLS; c++) {
        int col_block_found = 0;
        for (int r = 0; r < ROWS; r++) { // From top to bottom
            if (state->grid[r][c]) {
                col_block_found = 1;
                if (r > 0 && !state->grid[r-1][c]) col_transitions++; //空->满
            } else { // grid[r][c] == 0 (empty)
                if (col_block_found) {
                    holes++;
                    if (r > 0 && state->grid[r-1][c]) { // If cell above is filled, this hole is covered
                        covered_holes++;
                    }
                }
                if (r > 0 && state->grid[r-1][c]) col_transitions++; //满->空
            }
        }
    }

    for (int c = 0; c < COLS - 1; c++) {
        bumpiness += abs(heights[c] - heights[c + 1]);
    }

    for (int r = 0; r < ROWS; r++) {
        if (!state->grid[r][0] && heights[0] >= ROWS - r) row_transitions++; // Left wall
        for (int c = 0; c < COLS - 1; c++) {
            if (state->grid[r][c] != state->grid[r][c+1]) row_transitions++;
        }
        if (!state->grid[r][COLS-1] && heights[COLS-1] >= ROWS - r) row_transitions++; // Right wall
    }

    for (int c = 0; c < COLS; ++c) {
        int current_well_depth = 0;
        for (int r = ROWS - heights[c] -1 ; r >=0 && r < ROWS; --r) { // Start from just below surface of column c
            if (state->grid[r][c] == 0) { // If it's an empty cell
                int left_is_wall = (c == 0) || (heights[c-1] >= ROWS - r);
                int right_is_wall = (c == COLS - 1) || (heights[c+1] >= ROWS - r);
                if (left_is_wall && right_is_wall) {
                    current_well_depth++;
                } else { // Not a well cell or well ends
                    if (current_well_depth > 0) {
                        for(int k=1; k<=current_well_depth; ++k) well_sum_sq += k; // Sum of 1 to depth
                    }
                    current_well_depth = 0; // Reset for next potential well segment
                    break; // Break from this column's depth check if not a well cell
                }
            } else { // Hit a block, well ends
                if (current_well_depth > 0) {
                    for(int k=1; k<=current_well_depth; ++k) well_sum_sq += k;
                }
                current_well_depth = 0;
                break;
            }
        }
        if (current_well_depth > 0) { // If well continues to the bottom
            for(int k=1; k<=current_well_depth; ++k) well_sum_sq += k;
        }
    }


    int completed_lines = 0;
    for (int r = 0; r < ROWS; r++) {
        int is_line_full = 1;
        for (int c = 0; c < COLS; c++) {
            if (state->grid[r][c] == 0) {
                is_line_full = 0;
                break;
            }
        }
        if (is_line_full) completed_lines++;
    }

    eval_score += (long long)weights->aggregate_height_weight * aggregate_height;
    eval_score += (long long)weights->complete_lines_weight * completed_lines * COLS; // Reward for cells in cleared lines
    eval_score += (long long)weights->holes_weight * holes;
    eval_score += (long long)weights->covered_holes_weight * covered_holes;
    eval_score += (long long)weights->bumpiness_weight * bumpiness;
    eval_score += (long long)weights->row_transitions_weight * row_transitions;
    eval_score += (long long)weights->col_transitions_weight * col_transitions;
    eval_score += (long long)weights->well_sum_sq_weight * well_sum_sq;
    eval_score += (long long)weights->max_height_penalty_weight * max_col_height;

    if (max_col_height >= weights->game_over_penalty_threshold) {
        eval_score += weights->game_over_penalty_value;
        eval_score += (long long)(max_col_height - weights->game_over_penalty_threshold) * (weights->game_over_penalty_value / 10);
    }

    if (eval_score > INT_MAX) eval_score = INT_MAX;
    if (eval_score < INT_MIN) eval_score = INT_MIN;
    return (int)eval_score;
}

void find_best_move(GameState *current_state, PieceType current_piece, PieceType next_piece, Rotation *best_rot, int *best_x) {
    long best_overall_eval = LONG_MIN;
    *best_rot = DEG_0;
    *best_x = 0;
    int found_any_valid_move_for_current = 0;
    const EvalWeights *eval_weights = &DEFAULT_WEIGHTS;

    for (int r_curr = 0; r_curr < 4; r_curr++) {
        Rotation current_rotation = (Rotation)r_curr;
        const Piece *p_curr = &pieces[current_piece][current_rotation];
        for (int x_curr = 0; x_curr <= COLS - p_curr->width; x_curr++) {
            int can_place_curr_flag = 0;
            for (int y_try = 0; y_try <= ROWS - p_curr->height; y_try++) {
                if (can_place_piece(current_state, current_piece, current_rotation, x_curr, y_try)) {
                    can_place_curr_flag = 1;
                    break;
                }
            }
            if (!can_place_curr_flag && p_curr->height > 0) continue;

            GameState state_after_current = place_piece(current_state, current_piece, current_rotation, x_curr);
            long current_move_eval_final;

            if (state_after_current.top > ROWS) {
                current_move_eval_final = evaluate_state_with_weights(&state_after_current, eval_weights);
            } else {
                long best_eval_for_next_piece = LONG_MIN;
                int found_any_move_for_next = 0;
                for (int r_next = 0; r_next < 4; r_next++) {
                    Rotation next_rotation = (Rotation)r_next;
                    const Piece *p_next = &pieces[next_piece][next_rotation];
                    for (int x_next = 0; x_next <= COLS - p_next->width; x_next++) {
                        int can_place_next_flag = 0;
                        for (int y_try_next = 0; y_try_next <= ROWS - p_next->height; y_try_next++) {
                            if (can_place_piece(&state_after_current, next_piece, next_rotation, x_next, y_try_next)) {
                                can_place_next_flag = 1;
                                break;
                            }
                        }
                        if (!can_place_next_flag && p_next->height > 0) continue;

                        GameState state_after_next = place_piece(&state_after_current, next_piece, next_rotation, x_next);
                        long eval_of_next = evaluate_state_with_weights(&state_after_next, eval_weights);
                        if (!found_any_move_for_next || eval_of_next > best_eval_for_next_piece) {
                            best_eval_for_next_piece = eval_of_next;
                        }
                        found_any_move_for_next = 1;
                    }
                }
                if (found_any_move_for_next) {
                    current_move_eval_final = best_eval_for_next_piece;
                } else {
                    current_move_eval_final = evaluate_state_with_weights(&state_after_current, eval_weights);
                }
            }

            if (!found_any_valid_move_for_current || current_move_eval_final > best_overall_eval) {
                best_overall_eval = current_move_eval_final;
                *best_rot = current_rotation;
                *best_x = x_curr;
                found_any_valid_move_for_current = 1;
            } else if (current_move_eval_final == best_overall_eval) {
                if (current_rotation < *best_rot) {
                    *best_rot = current_rotation; *best_x = x_curr;
                } else if (current_rotation == *best_rot) {
                    const Piece *current_best_p_info = &pieces[current_piece][*best_rot];
                    int current_center_offset = abs((x_curr + p_curr->width / 2) - (COLS / 2));
                    int best_center_offset = abs((*best_x + current_best_p_info->width / 2) - (COLS / 2));
                    if (current_center_offset < best_center_offset) {
                        *best_x = x_curr;
                    } else if (current_center_offset == best_center_offset && x_curr < *best_x) {
                        *best_x = x_curr;
                    }
                }
            }
        }
    }

    if (!found_any_valid_move_for_current) {
        *best_rot = DEG_0;
        const Piece *default_p = &pieces[current_piece][DEG_0];
        *best_x = (COLS - default_p->width) / 2;
        if (*best_x < 0) *best_x = 0;
        if (*best_x + default_p->width > COLS) *best_x = COLS - default_p->width;
        if (*best_x < 0) *best_x = 0;
        if (default_p->width > COLS) *best_x = 0;
    }
}

int main() {
    init_pieces();
    GameState state;
    memset(&state, 0, sizeof(state));
    char initial_input_buffer[256];

    if (fgets(initial_input_buffer, sizeof(initial_input_buffer), stdin) == NULL) return 1;
    initial_input_buffer[strcspn(initial_input_buffer, "\n\r")] = 0;
    if (strlen(initial_input_buffer) == 0) return 1;
    if (initial_input_buffer[0] == 'X' || initial_input_buffer[0] == 'E') return 0;

    PieceType current_piece_type = char_to_piece(initial_input_buffer[0]);
    PieceType next_piece_type = I; // Default, will be overwritten
    int char_idx = 1;

    if (char_idx < strlen(initial_input_buffer)) {
        if (initial_input_buffer[char_idx] != 'X' && initial_input_buffer[char_idx] != 'E') {
            next_piece_type = char_to_piece(initial_input_buffer[char_idx]);
            char_idx++;
        }
    }

    while (1) {
        Rotation best_rot_val = DEG_0;
        int best_x_val = 0;
        find_best_move(&state, current_piece_type, next_piece_type, &best_rot_val, &best_x_val);

        GameState prev_state_for_x = state;
        PieceType piece_for_x_command = current_piece_type;
        state = place_piece(&state, current_piece_type, best_rot_val, best_x_val);

        printf("%d %d\n", best_rot_val, best_x_val);
        printf("%d\n", state.score);
        fflush(stdout);

        if (state.top >= ROWS-4 || (state.top > ROWS-4 && evaluate_state_with_weights(&state, &DEFAULT_WEIGHTS) == INT_MIN + 100) ) {
            break;
        }

        char next_input_char_val;
        int scanf_ret;
        if (char_idx < strlen(initial_input_buffer) && initial_input_buffer[char_idx] != 0) {
            next_input_char_val = initial_input_buffer[char_idx];
            char_idx++;
            scanf_ret = 1;
        } else {
            scanf_ret = scanf(" %c", &next_input_char_val);
        }

        if (scanf_ret != 1) break;

        if (next_input_char_val == 'X') {
            Rotation final_best_rot;
            int final_best_x;
            // For X, the "next piece" for lookahead during find_best_move is less defined.
            // We can pass a common piece like I, or the current next_piece_type.
            // Or, ideally, have a version of find_best_move that doesn't do lookahead for this specific call.
            // Here, we pass the current next_piece_type.
            find_best_move(&prev_state_for_x, piece_for_x_command, next_piece_type, &final_best_rot, &final_best_x);
            GameState final_state = place_piece(&prev_state_for_x, piece_for_x_command, final_best_rot, final_best_x);
            printf("%d %d\n", final_best_rot, final_best_x);
            printf("%d\n", final_state.score);
            // printf("这是主程序。"); // As per original user code's commented out line
            fflush(stdout);
            break;
        }
        if (next_input_char_val == 'E') exit(0);

        current_piece_type = next_piece_type;
        next_piece_type = char_to_piece(next_input_char_val);
    }
    return 0;
}