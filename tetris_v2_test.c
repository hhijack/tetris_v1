#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#define ROWS 20
#define COLS 10
#define MAX_PIECES 7

// 方块类型枚举
typedef enum {
    I, T, O, J, L, S, Z
} PieceType;

// 旋转角度枚举
typedef enum {
    DEG_0, DEG_90, DEG_180, DEG_270
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
    int well_sum_sq_weight;         // 井的深度"和"惩罚 (1+2+...+depth)
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
        -250,   // covered_holes_weight
        -25,    // bumpiness_weight
        -30,    // row_transitions_weight
        -30,    // col_transitions_weight
        -90,    // well_sum_sq_weight
        16,     // game_over_penalty_threshold
        -500000,// game_over_penalty_value
        -10     // max_height_penalty_weight
};

typedef struct {
    int x;
    int rotation;
    int score;
} Move;

// 函数声明
void init_pieces(void);
PieceType char_to_piece(char c);
int can_place_piece(const GameState *state, PieceType type, Rotation rotation, int x_col, int y_row);
GameState place_piece(const GameState *state, PieceType type, Rotation rotation, int x_col);
int evaluate_state(const GameState *state);
void find_best_move(GameState *current_state, PieceType type, PieceType next_piece, Rotation *best_rot, int *best_x);
int evaluate_with_lookahead(GameState *current_state, PieceType current_piece, PieceType next_piece);
Move find_best_move_lookahead(GameState *current_state, PieceType current_piece, PieceType next_piece);

int main() {
    // 初始化游戏状态
    GameState state;
    memset(&state, 0, sizeof(state));
    init_pieces();

    // 读取第一行输入：当前方块和下一个方块
    char current_char, next_char;
    if (scanf(" %c%c", &current_char, &next_char) != 2) {
        return 1;
    }

    // 检查是否是结束命令
    if (current_char == 'X' || current_char == 'E') {
        return 0;
    }

    PieceType current_piece = char_to_piece(current_char);
    PieceType next_piece = char_to_piece(next_char);

    // 主游戏循环
    while (1) {
        // 寻找最佳移动
        Rotation best_rot;
        int best_x;

        // 使用预测方法
        find_best_move(&state, current_piece, next_piece, &best_rot, &best_x);

        // 保存当前状态（用于X命令）
        GameState prev_state = state;
        PieceType prev_piece = current_piece;

        // 放置方块
        state = place_piece(&state, current_piece, best_rot, best_x);

        // 输出移动和分数
        printf("%d %d\n", best_rot, best_x);
        printf("%d\n", state.score);
        fflush(stdout);

        // 检查游戏是否结束
        if (state.top >= 16) {
            break;
        }

        // 读取下一个输入
        char input;
        if (scanf(" %c", &input) != 1) {
            break;
        }

        // 处理特殊命令
        if (input == 'X') {
            // 重新计算最佳移动
            find_best_move(&prev_state, prev_piece, next_piece, &best_rot, &best_x);
            state = place_piece(&prev_state, prev_piece, best_rot, best_x);
            printf("%d %d\n", best_rot, best_x);
            printf("%d\n", state.score);
            fflush(stdout);
            break;
        }
        if (input == 'E') {
            return 0;
        }

        // 更新方块
        current_piece = next_piece;
        next_piece = char_to_piece(input);
    }

    return 0;
}

// 评估游戏状态的好坏
int evaluate_state(const GameState *state)
{
    int eval_score = state->score * 200;  // 再次提高得分权重

    int aggregate_height = 0;
    int max_col_height = 0;
    int heights[COLS] = {0};
    int holes = 0;
    int covered_holes = 0;
    int bumpiness = 0;
    int row_transitions = 0;
    int col_transitions = 0;
    int well_sum = 0;
    int deep_wells = 0;
    int complete_lines = 0;
    int left_col_height = 0;
    int right_col_height = 0;
    int center_weight = 0;
    int tetris_ready = 0;

    // 计算各列高度和总高度
    for (int c = 0; c < COLS; c++) {
        heights[c] = 0;
        for (int r = 0; r < ROWS; r++) {
            if (state->grid[r][c]) {
                heights[c] = ROWS - r;
                break;
            }
        }
        aggregate_height += heights[c];
        if (heights[c] > max_col_height) {
            max_col_height = heights[c];
        }

        // 边缘列高度记录
        if (c < COLS / 2) {
            left_col_height += heights[c];
        } else {
            right_col_height += heights[c];
        }

        // 越靠近中间的列，权重越大（鼓励在中间放置，方便两边消行）
        int col_center_score = 10 - abs((c - (COLS-1)/2.0) * 2);
        center_weight += heights[c] * col_center_score;
    }

    // 计算空洞和列变换
    for (int c = 0; c < COLS; c++) {
        int block_found = 0;
        int hole_found = 0;
        int consecutive_holes = 0;
        for (int r = 0; r < ROWS; r++) {
            if (state->grid[r][c]) {
                block_found = 1;
                if (r > 0 && !state->grid[r-1][c]) col_transitions++;
                if (hole_found) {
                    covered_holes++;
                    hole_found = 0;
                    consecutive_holes = 0;
                }
            } else {
                if (block_found) {
                    holes++;
                    hole_found = 1;
                    consecutive_holes++;
                    if (r > 0 && state->grid[r-1][c]) col_transitions++;

                    // 连续空洞惩罚更高
                    if (consecutive_holes > 1) {
                        covered_holes += consecutive_holes - 1;
                    }
                }
            }
        }
    }

    // 计算崎岖度
    for (int c = 0; c < COLS - 1; c++) {
        int diff = abs(heights[c] - heights[c + 1]);
        bumpiness += diff;

        // 大的高度差惩罚更多（平方惩罚）
        if (diff > 1) {
            bumpiness += diff * diff / 2;
        }
    }

    // 计算行变换和完整行
    for (int r = 0; r < ROWS; r++) {
        int line_blocks = 0;
        if (state->grid[r][0] == 0 && heights[0] > (ROWS - 1 - r)) row_transitions++;
        for (int c = 0; c < COLS - 1; c++) {
            if (state->grid[r][c]) line_blocks++;
            if ((heights[c] > (ROWS - 1 - r)) || (heights[c+1] > (ROWS - 1 - r))) {
                if (state->grid[r][c] != state->grid[r][c+1]) row_transitions++;
            }
        }
        if (state->grid[r][COLS-1]) line_blocks++;
        if (state->grid[r][COLS-1] == 0 && heights[COLS-1] > (ROWS - 1 - r)) row_transitions++;

        // 计算完整行
        if (line_blocks == COLS) {
            complete_lines++;
        }
            // 接近完整的行也有奖励
        else if (line_blocks >= COLS - 1) {
            eval_score += 80 * line_blocks;
        }
            // 检测Tetris准备情况
        else if (line_blocks >= COLS - 2) {
            tetris_ready++;
        }
    }

    // 计算井
    for (int c = 0; c < COLS; c++) {
        for (int r = 0; r < ROWS; r++) {
            if (state->grid[r][c] == 0) {
                int left_blocked = (c == 0) || (c > 0 && heights[c-1] >= ROWS - r);
                int right_blocked = (c == COLS - 1) || (c < COLS-1 && heights[c+1] >= ROWS - r);
                if (left_blocked && right_blocked) {
                    int well_depth = 0;
                    for (int wr = r; wr < ROWS; wr++) {
                        if (state->grid[wr][c] == 0) {
                            int L = (c == 0) || (c > 0 && heights[c-1] >= ROWS - wr);
                            int R = (c == COLS - 1) || (c < COLS-1 && heights[c+1] >= ROWS - wr);
                            if (L && R) {
                                well_depth++;
                            } else {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                    if (well_depth > 0) {
                        // 使用平方和惩罚
                        int well_penalty = (well_depth * (well_depth + 1) * (2 * well_depth + 1)) / 6;
                        well_sum += well_penalty;

                        // 超过3个深度的井严重惩罚
                        if (well_depth > 3) {
                            deep_wells += well_depth - 3;
                        }

                        r += well_depth - 1;
                    }
                }
            }
        }
    }

    // 平衡性奖励（两边高度差不要太大）
    int balance_score = -abs(left_col_height - right_col_height) * 10;

    // 优化后的权重
    eval_score += (-25 * aggregate_height);      // 进一步降低总高度惩罚
    eval_score += (2000 * complete_lines);       // 再次提高完整行奖励
    eval_score += (300 * tetris_ready);          // 奖励接近Tetris的状态
    eval_score += (-55 * holes);                 // 增加空洞惩罚
    eval_score += (-150 * covered_holes);        // 大幅增加被覆盖空洞惩罚
    eval_score += (-18 * bumpiness);             // 调整崎岖度惩罚
    eval_score += (-12 * row_transitions);       // 调整行变换惩罚
    eval_score += (-12 * col_transitions);       // 调整列变换惩罚
    eval_score += (-120 * well_sum);             // 增加井的惩罚
    eval_score += (-350 * deep_wells);           // 严重惩罚深井
    eval_score += (8 * center_weight);           // 鼓励在中间放置方块
    eval_score += balance_score;                 // 添加平衡性奖励
    eval_score += (-6 * max_col_height);         // 低惩罚最大高度

    // I型方块需要保留位置，高度惩罚降低
    if (max_col_height < ROWS / 2) {
        eval_score += 300;  // 低高度奖励
    }

    // 游戏结束惩罚
    if (state->top >= 16) {
        eval_score -= 1000000;
    }

    // 极高堆积惩罚
    if (max_col_height > ROWS * 3/4) {
        eval_score -= 20000 * (max_col_height - ROWS * 3/4);
    }

    return eval_score;
}

// 多步预测评估
int evaluate_with_lookahead(GameState *current_state, PieceType current_piece, PieceType next_piece) {
    // 第一步：尝试当前方块的所有可能性
    Move best_current_move = {0, DEG_0, INT_MIN};

    for (int rot_idx = 0; rot_idx < 4; rot_idx++) {
        Rotation current_rotation = (Rotation)rot_idx;
        const Piece *piece = &pieces[current_piece][current_rotation];

        for (int x_col = 0; x_col <= COLS - piece->width; x_col++) {
            // 检查是否可以放置
            int can_spawn = 0;
            for (int y_try = 0; y_try <= ROWS - piece->height; y_try++) {
                if (can_place_piece(current_state, current_piece, current_rotation, x_col, y_try)) {
                    can_spawn = 1;
                    break;
                }
            }
            if (!can_spawn) continue;

            // 模拟放置并评估
            GameState next_state = place_piece(current_state, current_piece, current_rotation, x_col);

            // 第二步：尝试下一个方块的所有可能性
            int best_next_eval = INT_MIN;

            for (int next_rot_idx = 0; next_rot_idx < 4; next_rot_idx++) {
                Rotation next_rotation = (Rotation)next_rot_idx;
                const Piece *next_piece_obj = &pieces[next_piece][next_rotation];

                for (int next_x = 0; next_x <= COLS - next_piece_obj->width; next_x++) {
                    // 检查是否可以放置
                    int next_can_spawn = 0;
                    for (int next_y = 0; next_y <= ROWS - next_piece_obj->height; next_y++) {
                        if (can_place_piece(&next_state, next_piece, next_rotation, next_x, next_y)) {
                            next_can_spawn = 1;
                            break;
                        }
                    }
                    if (!next_can_spawn) continue;

                    // 模拟放置并评估
                    GameState after_next_state = place_piece(&next_state, next_piece, next_rotation, next_x);
                    int next_eval = evaluate_state(&after_next_state);

                    if (next_eval > best_next_eval) {
                        best_next_eval = next_eval;
                    }
                }
            }

            // 如果没有找到下一步可行方案，只评估当前状态
            int final_eval = best_next_eval != INT_MIN ?
                             evaluate_state(&next_state) * 0.6 + best_next_eval * 0.4 :
                             evaluate_state(&next_state);

            if (final_eval > best_current_move.score) {
                best_current_move.score = final_eval;
                best_current_move.x = x_col;
                best_current_move.rotation = current_rotation;
            }
        }
    }

    return best_current_move.score;
}

// 使用多步预测找最佳移动
Move find_best_move_lookahead(GameState *current_state, PieceType current_piece, PieceType next_piece) {
    Move best_move = {0, DEG_0, INT_MIN};

    for (int rot_idx = 0; rot_idx < 4; rot_idx++) {
        Rotation current_rotation = (Rotation)rot_idx;
        const Piece *piece = &pieces[current_piece][current_rotation];

        for (int x_col = 0; x_col <= COLS - piece->width; x_col++) {
            // 检查是否可以放置
            int can_spawn = 0;
            for (int y_try = 0; y_try <= ROWS - piece->height; y_try++) {
                if (can_place_piece(current_state, current_piece, current_rotation, x_col, y_try)) {
                    can_spawn = 1;
                    break;
                }
            }
            if (!can_spawn) continue;

            // 模拟放置并评估
            GameState next_state = place_piece(current_state, current_piece, current_rotation, x_col);
            int current_eval = evaluate_state(&next_state);

            // 特殊方块策略
            // T型方块特殊评估（T-spin机会）
            if (current_piece == T) {
                // 检测T-spin机会（周围至少有3个方块）
                int surrounded_blocks = 0;
                for (int c = x_col - 1; c <= x_col + piece->width; c++) {
                    for (int r = 0; r < ROWS; r++) {
                        if (c >= 0 && c < COLS && next_state.grid[r][c]) {
                            surrounded_blocks++;
                        }
                    }
                }
                if (surrounded_blocks >= 3) {
                    current_eval += 800; // 增加T-spin奖励
                }
            }

            // I型方块特殊评估（Tetris机会）
            if (current_piece == I && (current_rotation == DEG_0 || current_rotation == DEG_180)) {
                // 检查是否可以获得Tetris（消除4行）
                int tetris_potential = 0;
                for (int r = 0; r < ROWS; r++) {
                    int filled = 1;
                    for (int c = 0; c < COLS; c++) {
                        if (!next_state.grid[r][c]) {
                            filled = 0;
                            break;
                        }
                    }
                    if (filled) tetris_potential++;
                }
                if (tetris_potential == 4) {
                    current_eval += 3000; // 增加Tetris奖励
                }
                else if (tetris_potential > 0) {
                    current_eval += 500 * tetris_potential; // 部分消行也有奖励
                }
            }

            // 考虑下一个方块
            // 预测下一个方块的最佳放置
            int next_best_eval = INT_MIN;
            for (int next_rot = 0; next_rot < 4; next_rot++) {
                const Piece *next_piece_obj = &pieces[next_piece][next_rot];
                for (int next_x = 0; next_x <= COLS - next_piece_obj->width; next_x++) {
                    int next_can_place = 0;
                    for (int next_y = 0; next_y <= ROWS - next_piece_obj->height; next_y++) {
                        if (can_place_piece(&next_state, next_piece, next_rot, next_x, next_y)) {
                            next_can_place = 1;
                            break;
                        }
                    }
                    if (!next_can_place) continue;

                    GameState after_next_state = place_piece(&next_state, next_piece, next_rot, next_x);
                    int next_eval = evaluate_state(&after_next_state);
                    if (next_eval > next_best_eval) {
                        next_best_eval = next_eval;
                    }
                }
            }

            // 如果找到了下一步的评估，结合当前和下一步的评估
            if (next_best_eval != INT_MIN) {
                current_eval = current_eval * 0.7 + next_best_eval * 0.3;
            }

            if (current_eval > best_move.score) {
                best_move.score = current_eval;
                best_move.x = x_col;
                best_move.rotation = current_rotation;
            }
        }
    }

    // 如果没有找到有效移动，使用默认值
    if (best_move.score == INT_MIN) {
        best_move.rotation = DEG_0;
        best_move.x = (COLS - pieces[current_piece][DEG_0].width) / 2;
        if (best_move.x < 0) best_move.x = 0;
        best_move.score = 0;
    }

    return best_move;
}

// 寻找最佳放置位置
void find_best_move(GameState *current_state, PieceType type, PieceType next_piece, Rotation *best_rot, int *best_x)
{
    Move best_move = find_best_move_lookahead(current_state, type, next_piece);
    *best_rot = best_move.rotation;
    *best_x = best_move.x;
}

void init_pieces()
{
    // I 型方块
    // 0度: 1111 (宽4, 高1)
    int i_shape[4][4][4] = {
            {{1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 0 度
            {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}, // 90 度 (旋转后，宽1, 高4)
            {{1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // 180 度 (同0度形状，但可以是不同定义)
            {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}  // 270 度 (同90度形状)
    };
    for (int r = 0; r < 4; r++)
    {
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
    for (int r = 0; r < 4; r++)
    {
        memcpy(pieces[T][r].shape, t_shape[r], sizeof(t_shape[r]));
        pieces[T][r].width = (r == DEG_0 || r == DEG_180) ? 3 : 2;
        pieces[T][r].height = (r == DEG_0 || r == DEG_180) ? 2 : 3;
    }

    // O 型方块
    // 0度: (宽2, 高2)
    // 11
    // 11
    int o_shape[4][4] = {{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
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
    for (int r = 0; r < 4; r++)
    {
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
    for (int r = 0; r < 4; r++)
    {
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
    for (int r = 0; r < 4; r++)
    {
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
    for (int r = 0; r < 4; r++)
    {
        memcpy(pieces[Z][r].shape, z_shape[r], sizeof(z_shape[r]));
        pieces[Z][r].width = (r == DEG_0 || r == DEG_180) ? 3 : 2;
        pieces[Z][r].height = (r == DEG_0 || r == DEG_180) ? 2 : 3;
    }
}

// 将字符转换为 PieceType
PieceType char_to_piece(char c)
{
    switch (c)
    {
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
            fprintf(stderr, "警告: 未知方块字符 '%c'，默认使用 I 型方块。\n", c);
            return I;
    }
}

// 检查方块是否可以放置在指定位置
int can_place_piece(const GameState *state, PieceType type, Rotation rotation, int x_col, int y_row)
{
    const Piece *piece = &pieces[type][rotation];

    if (x_col < 0 || x_col + piece->width > COLS || y_row < 0 || y_row + piece->height > ROWS)
    {
        return 0;
    }

    for (int py = 0; py < piece->height; py++)
    {
        for (int px = 0; px < piece->width; px++)
        {
            if (piece->shape[py][px])
            {
                if (state->grid[y_row + py][x_col + px])
                {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// 模拟放置方块并返回新的游戏状态
GameState place_piece(const GameState *state, PieceType type, Rotation rotation, int x_col)
{
    GameState new_state = *state;
    const Piece *piece = &pieces[type][rotation];

    int y_drop_row = -1; // 哨兵值：如果方块甚至无法放置在 y=0 的位置。
    // 理想情况下，在调用 place_piece 之前应该检查这一点，
    // 或者 find_best_move 应该确保放置位置有效。
    // 假设 x_col 对于方块宽度是有效的。

    for (int y_try = 0; y_try <= ROWS - piece->height; y_try++)
    {
        if (can_place_piece(&new_state, type, rotation, x_col, y_try))
        {
            y_drop_row = y_try;
        }
        else
        {
            break;
        }
    }

    // 如果 y_drop_row 仍然是 -1，意味着方块甚至无法放置在 y=0 的位置。
    // 这是游戏结束的场景，或者是 find_best_move 给出的无效移动。
    // 为了健壮性，可能需要处理这种情况，但俄罗斯方块 AI 通常会避免此类移动。
    // 原始代码隐式假设能找到有效的放置行。

    if (y_drop_row == -1)
    { // 如果 find_best_move 正常工作，这种情况不应发生
        // 这表明立即游戏结束的情况，通常是方块生成在已有方块上。
        // 为了简单起见，我们继续执行，评估时会进行严重惩罚。
        // 更健壮的系统可能会在这里设置一个游戏结束标志。
        // 原始代码的游戏结束条件是放置后 `new_state.top >= 15`。
        // 为避免问题，让我们确保如果 y_drop_row 有效，就放置方块。
        // 如果 find_best_move 找不到任何旋转和 x 坐标的有效放置位置，那么游戏结束。
        // 这里我们假设 y_drop_row 会从 find_best_move 得到有效值。
        // 如果由于某种原因，选择的 x_col 和旋转无法放置（例如，x_col 一开始就无效），
        // y_drop_row 可能保持未初始化状态，或者指向无效位置。
        // find_best_move 应该只考虑可放置的移动。
        // 寻找 y_drop_row 的循环应该确保它要么是有效行，要么如果无法放置则保持初始值。
        // 如果 y_drop_row = -1，表明 find_best_move 选择了不可能的移动（例如，放置在棋盘外的列）
        // 或者由于碰撞方块无法放置在 y=0 的位置，这意味着游戏结束。
    }

    for (int py = 0; py < piece->height; py++)
    {
        for (int px = 0; px < piece->width; px++)
        {
            if (piece->shape[py][px])
            {
                if (y_drop_row + py < ROWS && x_col + px < COLS)
                { // 安全边界检查
                    new_state.grid[y_drop_row + py][x_col + px] = 1;
                }
            }
        }
    }

    int lines_cleared = 0;
    for (int r = ROWS - 1; r >= 0;)
    {
        int is_line_full = 1;
        for (int c = 0; c < COLS; c++)
        {
            if (new_state.grid[r][c] == 0)
            {
                is_line_full = 0;
                break;
            }
        }
        if (is_line_full)
        {
            lines_cleared++;
            for (int sr = r; sr > 0; sr--)
            {
                memcpy(new_state.grid[sr], new_state.grid[sr - 1], COLS * sizeof(int));
            }
            memset(new_state.grid[0], 0, COLS * sizeof(int));
            // 不递减 r，重新检查当前行索引
        }
        else
        {
            r--;
        }
    }

    switch (lines_cleared)
    {
        case 1:
            new_state.score += 100;
            break;
        case 2:
            new_state.score += 300;
            break;
        case 3:
            new_state.score += 500;
            break;
        case 4:
            new_state.score += 800;
            break;
    }

    new_state.top = 0;
    for (int r_idx = 0; r_idx < ROWS; r_idx++)
    {
        for (int c_idx = 0; c_idx < COLS; c_idx++)
        {
            if (new_state.grid[r_idx][c_idx])
            {
                new_state.top = ROWS - r_idx;
                goto found_max_height_place_piece;
            }
        }
    }
    found_max_height_place_piece:;

    return new_state;
}