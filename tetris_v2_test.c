#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define ROWS 20
#define COLS 10
#define MAX_PIECES 7

// 方块类型枚举
typedef enum{
    I,T,O,J,L,S,Z
} PieceType;

// 旋转角度枚举
typedef enum
{
    DEG_0,DEG_90,DEG_180,DEG_270
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
    int top;              // 当前最高堆积高度 (从底部开始的行数，为空时为 0)
} GameState;

// 预定义所有方块的所有旋转状态
Piece pieces[MAX_PIECES][4]; // 7 种方块，每种 4 种旋转

// 函数预定义
void init_pieces(void);
PieceType char_to_piece(char c);
int can_place_piece(const GameState *state, PieceType type, Rotation rotation, int x_col, int y_row);
GameState place_piece(const GameState *state, PieceType type, Rotation rotation, int x_col);
int evaluate_state(const GameState *state);
void find_best_move(GameState *current_state, PieceType current_piece, PieceType next_piece_type, Rotation *best_rot, int *best_x);

int main()
{
    init_pieces();

    clock_t start_time = clock();
    double time_limit_seconds = 9.8;

    GameState state;
    memset(&state, 0, sizeof(state));
    state.score = 0;
    state.top = 0;

    char initial_input_buffer[10];
    if (fgets(initial_input_buffer, sizeof(initial_input_buffer), stdin) == NULL)
    {
        fprintf(stderr, "DEBUG: 游戏因初始fgets读取错误或EOF结束。\n");
        return 1;
    }
    initial_input_buffer[strcspn(initial_input_buffer, "\n")] = 0;

    if (strlen(initial_input_buffer) == 0)
    {
        fprintf(stderr, "DEBUG: 游戏因初始输入为空结束。\n");
        return 1;
    }

    if (initial_input_buffer[0] == 'X')
    {
        fprintf(stderr, "DEBUG: 游戏因初始输入 'X' 结束。\n");
        return 0;
    }
    if (initial_input_buffer[0] == 'E')
    {
        fprintf(stderr, "DEBUG: 游戏因初始输入 'E' 结束。\n");
        exit(0);
    }

    PieceType current_piece = char_to_piece(initial_input_buffer[0]);
    PieceType next_piece;

    if (strlen(initial_input_buffer) >= 2)
    {
        if (initial_input_buffer[1] == 'X' || initial_input_buffer[1] == 'E')
        {
            next_piece = I;
        }
        else
        {
            next_piece = char_to_piece(initial_input_buffer[1]);
        }
    }
    else
    {
        next_piece = I;
    }

    int game_loop_count = 0; // 追踪循环次数

    while (1)
    {
        game_loop_count++;
        // fprintf(stderr, "DEBUG: Main loop iteration %d, Current piece: %c, Next piece: %c, Score: %d, Top: %d\n",
        //        game_loop_count, "ITJLOSZ"[current_piece], "ITJLOSZ"[next_piece], state.score, state.top);


        clock_t current_time_loop_start = clock();
        double elapsed_seconds = (double)(current_time_loop_start - start_time) / CLOCKS_PER_SEC;
        if (elapsed_seconds > time_limit_seconds) {
            fprintf(stderr, "DEBUG: 游戏因超时结束 (%.2f 秒)。当前分数: %d\n", elapsed_seconds, state.score);
            fflush(stdout);
            return 0;
        }

        Rotation best_rot_val = DEG_0;
        int best_x_val = 0;

        // 检查是否已经无法放置 (在 find_best_move 之前)
        // 这种情况通常是上一轮放置后就顶出了，或者棋盘已满
        // find_best_move 应该能处理这种情况并可能返回一个导致游戏结束的移动
        // 如果 state.top > ROWS (来自上一次 place_piece)，则游戏可能已经结束
        if (state.top > ROWS) { // ROWS+1 表示由 place_piece 标记的游戏结束
            fprintf(stderr, "DEBUG: 游戏在尝试放置方块前已结束 (state.top = %d > ROWS)。当前分数: %d\n", state.top, state.score);
            fflush(stdout);
            return 0; // 或者 break; 然后在循环外处理
        }


        find_best_move(&state, current_piece, next_piece, &best_rot_val, &best_x_val);
        GameState prev_state_before_place = state; // 保存放置前的状态用于比较
        state = place_piece(&state, current_piece, best_rot_val, best_x_val);

        // 检查 place_piece 是否导致游戏结束
        if (state.top > ROWS) { // ROWS+1 means game over from place_piece
            fprintf(stderr, "DEBUG: 游戏因无法放置方块 '%c' (rot:%d, x:%d) 而结束。盘面已满或阻塞。当前分数: %d\n",
                    "ITJLOSZ"[current_piece], best_rot_val, best_x_val, prev_state_before_place.score); // 使用放置前的分数
            // 可以打印一下最终的盘面状态进行调试
            // for (int r = 0; r < ROWS; ++r) {
            //     for (int c = 0; c < COLS; ++c) {
            //         fprintf(stderr, "%d ", prev_state_before_place.grid[r][c]);
            //     }
            //     fprintf(stderr, "\n");
            // }
            fflush(stdout);
            // 输出题目要求的最后一次放置尝试和得分（如果规则要求）
            // 如果超时不得分，这里的输出可能不被OJ计分，但对调试有用
            printf("%d %d\n", best_rot_val, best_x_val);
            printf("%d\n", prev_state_before_place.score); // 输出导致游戏结束的这一步之前的分数
            fflush(stdout);
            return 0; // 游戏结束
        }


        printf("%d %d\n", best_rot_val, best_x_val);
        printf("%d\n", state.score);
        fflush(stdout);

        char next_piece_char_input;
        if (scanf(" %c", &next_piece_char_input) != 1)
        {
            fprintf(stderr, "DEBUG: 游戏因读取下一个方块字符时遇到EOF或错误而结束。当前分数: %d\n", state.score);
            fflush(stdout);
            break;
        }

        if (next_piece_char_input == 'X')
        {
            fprintf(stderr, "DEBUG: 接收到 'X' 指令，准备结束游戏。当前分数: %d\n", state.score);
            clock_t current_time_X = clock();
            elapsed_seconds = (double)(current_time_X - start_time) / CLOCKS_PER_SEC;
            if (elapsed_seconds > time_limit_seconds) {
                fprintf(stderr, "DEBUG: 'X'处理前时间超出限制 (%.2f 秒)，程序终止。当前分数: %d\n", elapsed_seconds, state.score);
                fflush(stdout);
                return 0;
            }

            Rotation final_best_rot;
            int final_best_x;
            GameState state_before_final_X_place = state;
            find_best_move(&state, current_piece, I, &final_best_rot, &final_best_x);
            state = place_piece(&state, current_piece, final_best_rot, final_best_x);

            if (state.top > ROWS) {
                fprintf(stderr, "DEBUG: 'X'指令的最后一次放置方块 '%c' (rot:%d, x:%d) 导致游戏结束。分数: %d\n",
                        "ITJLOSZ"[current_piece], final_best_rot, final_best_x, state_before_final_X_place.score);
                printf("%d %d\n", final_best_rot, final_best_x);
                printf("%d\n", state_before_final_X_place.score); // 输出导致结束前的分数
            } else {
                printf("%d %d\n", final_best_rot, final_best_x);
                printf("%d\n", state.score);
            }
            fflush(stdout);
            fprintf(stderr, "DEBUG: 游戏因 'X' 指令正常结束。\n");
            break;
        }

        if (next_piece_char_input == 'E')
        {
            fprintf(stderr, "DEBUG: 游戏因 'E' 指令结束。当前分数: %d\n", state.score);
            fflush(stdout); // 确保所有输出已刷新
            exit(0); // 'E' 通常是立即退出
        }

        current_piece = next_piece;
        next_piece = char_to_piece(next_piece_char_input);
    }

    // 如果循环通过 break 正常退出 (例如 EOF 或 'X')
    // fprintf(stderr, "DEBUG: 游戏主循环结束。最终分数: %d\n", state.score);
    fflush(stdout);
    return 0;
}
//初始化方块
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

    if (y_drop_row == -1) {
        // 无法放置，返回原始状态并标记为游戏结束
        new_state.top = ROWS + 1;
        return new_state;
    }

    for (int py = 0; py < piece->height; py++)
    {
        for (int px = 0; px < piece->width; px++)
        {
            if (piece->shape[py][px])
            {
                if (y_drop_row + py >= 0 && y_drop_row + py < ROWS && x_col + px < COLS) {
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

// 评估游戏状态的好坏
int evaluate_state(const GameState *state)
{
    // 增加实际得分的权重
    int eval_score = state->score * 15;  // 从10提高到15

    int aggregate_height = 0;
    int max_col_height = 0;
    int heights[COLS] = {0};
    int holes = 0;
    int bumpiness = 0;

    // 新增评估指标
    int adjacent_holes = 0;  // 相邻空洞计数
    int center_height_bonus = 0; // 中间列高度奖励
    int i_channel_count = 0; // I型方块通道计数 <--- Make sure this is declared (it is in your original code)
    int potential_lines = 0; // 潜在可消除行
    int complete_lines = 0;  // 完整行计数

    // 计算各列高度
    for (int c = 0; c < COLS; c++)
    {
        heights[c] = 0;
        for (int r = 0; r < ROWS; r++)
        {
            if (state->grid[r][c])
            {
                heights[c] = ROWS - r;
                break;
            }
        }
        aggregate_height += heights[c];
        if (heights[c] > max_col_height)
        {
            max_col_height = heights[c];
        }

        // 奖励中间列（准备放I型方块）
        if (c >= 3 && c <= 6) {
            center_height_bonus += heights[c];
        }
    }

    // 计算空洞
    int hole_map[ROWS][COLS] = {0};
    for (int c = 0; c < COLS; c++)
    {
        int block_found = 0;
        for (int r = 0; r < ROWS; r++)
        {
            if (state->grid[r][c])
            {
                block_found = 1;
            }
            else if (block_found)
            {
                holes++;
                hole_map[r][c] = 1; // 标记空洞位置
            }
        }
    }

    // 使用一个visited数组标记已访问空洞
    int visited[ROWS][COLS] = {0};
    adjacent_holes = 0;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (hole_map[r][c] && !visited[r][c]) {
                // 发现新空洞组，计数+1
                adjacent_holes++;
                // 标记所有连通空洞为已访问
                // (这里需要用DFS或BFS递归标记)
            }
        }
    }

    // 计算凹凸不平度
    for (int c = 0; c < COLS - 1; c++)
    {
        bumpiness += abs(heights[c] - heights[c + 1]);
    }

    // 计算完整行
    for (int r = 0; r < ROWS; r++)
    {
        int is_line_full = 1;
        for (int c = 0; c < COLS; c++)
        {
            if (!state->grid[r][c])
            {
                is_line_full = 0;
                break;
            }
        }
        if (is_line_full)
            complete_lines++;
    }

    // 计算潜在可消除行
    for (int r = 0; r < ROWS; r++) {
        int block_count = 0;
        for (int c = 0; c < COLS; c++) {
            if (state->grid[r][c]) {
                block_count++;
            }
        }
        // 如果一行已经有7个或更多方块，有潜力被完成
        if (block_count >= 7 && block_count < COLS) {
            potential_lines++;
        }
    }

    // START OF REPLACEMENT: Original "检查I型方块通道" block is removed.
    // New logic for i_channel_count (detecting "deep wells"):
    i_channel_count = 0; // Reset/Initialize before calculation
    for (int c = 0; c < COLS; c++) {
        int current_col_h = heights[c];
        // The following two lines for left_neighbor_h and right_neighbor_h
        // are not strictly used by the if conditions below as they directly access heights[c-1] and heights[c+1].
        // You can keep them or remove them if you simplify the conditions.
        // int left_neighbor_h = (c > 0) ? heights[c-1] : current_col_h + 4;
        // int right_neighbor_h = (c < COLS - 1) ? heights[c+1] : current_col_h + 4;

        if (COLS == 1) {
            // No wells possible in a 1-column board by this definition
        } else if (c == 0) { // 最左列的井 (只需要右边有墙)
            if (heights[c+1] >= current_col_h + 4) {
                i_channel_count++;
            }
        } else if (c == COLS - 1) { // 最右列的井 (只需要左边有墙)
            if (heights[c-1] >= current_col_h + 4) {
                i_channel_count++;
            }
        } else { // 中间列的井 (需要两边都有墙) (c > 0 && c < COLS - 1 implicitly true here)
            if (heights[c-1] >= current_col_h + 4 && heights[c+1] >= current_col_h + 4) {
                i_channel_count++;
            }
        }
    }
    // END OF REPLACEMENT

    // 权重调整
    eval_score += (-30 * aggregate_height); // 减轻高度惩罚
    eval_score += (80 * complete_lines);    // 增加消行奖励
    eval_score += (-75 * holes);            // 增加空洞惩罚
    eval_score += (-20 * adjacent_holes);   // 惩罚相邻空洞
    eval_score += (-15 * bumpiness);        // 减轻凹凸惩罚
    eval_score += (5 * center_height_bonus); // 奖励中间列较高
    eval_score += (50 * i_channel_count);   // 奖励I型方块通道 (now rewards deep wells)
    eval_score += (25 * potential_lines);   // 奖励潜在可消除行

    // 高度惩罚
    if (state->top >= 16) {
        eval_score -= 500000;  // 游戏结束惩罚
    }

    return eval_score;
}

// 寻找最佳放置位置
void find_best_move(GameState *current_state, PieceType current_piece, PieceType next_piece_type, Rotation *best_rot, int *best_x)
{
    // 函数参数名改为next_piece_type，避免与局部变量冲突
    long best_eval_score = LONG_MIN;
    *best_rot = DEG_0;
    *best_x = 0;
    int found_any_move = 0;

    // 当前方块权重与下一个方块权重
    const float CURRENT_WEIGHT = 1.0;
    const float NEXT_WEIGHT = 0.85; // 考虑下一个方块的影响

    // 针对特定方块的策略调整
    float piece_bonus[MAX_PIECES] = {0};
    piece_bonus[I] = 300; // I型方块最灵活
    piece_bonus[O] = 100; // O型方块稳定但不灵活
    piece_bonus[T] = 200; // T型方块适合T-spin
    piece_bonus[J] = 150; // J和L型方块相对通用
    piece_bonus[L] = 150;
    piece_bonus[S] = 50;  // S和Z型方块比较难用
    piece_bonus[Z] = 50;

    // 在搜索中考虑下一个方块
    for (int rot_idx = 0; rot_idx < 4; rot_idx++)
    {
        Rotation current_rotation = (Rotation)rot_idx;
        const Piece *piece = &pieces[current_piece][current_rotation];

        for (int x_col = 0; x_col <= COLS - piece->width; x_col++)
        {
            // 检查当前方块是否能放置
            GameState temp_check_state = *current_state;
            int can_spawn = 0;
            for (int y_try_spawn = 0; y_try_spawn <= ROWS - piece->height; ++y_try_spawn)
            {
                if (can_place_piece(&temp_check_state, current_piece, current_rotation, x_col, y_try_spawn))
                {
                    can_spawn = 1;
                    break;
                }
            }
            if (!can_spawn && piece->height > 0)
            {
                continue; // 尝试下一个x_col或旋转
            }

            GameState next_state = place_piece(current_state, current_piece, current_rotation, x_col);
            int current_eval_score = evaluate_state(&next_state);
            current_eval_score += piece_bonus[current_piece];
            found_any_move = 1;

            // 考虑下一个方块的最佳放置
            long next_best_score = LONG_MIN;
            int found_next_move = 0;

            for (int next_rot_idx = 0; next_rot_idx < 4; next_rot_idx++)
            {
                Rotation next_rotation = (Rotation)next_rot_idx;
                const Piece *next_piece_obj = &pieces[next_piece_type][next_rotation];

                for (int next_x_col = 0; next_x_col <= COLS - next_piece_obj->width; next_x_col++)
                {
                    // 检查下一个方块是否能放置
                    int can_place_next = 0;
                    for (int y_try = 0; y_try <= ROWS - next_piece_obj->height; y_try++) {
                        if (can_place_piece(&next_state, next_piece_type, next_rotation, next_x_col, y_try)) {
                            can_place_next = 1;
                            break;
                        }
                    }

                    if (can_place_next) {
                        GameState future_state = place_piece(&next_state, next_piece_type, next_rotation, next_x_col);
                        long next_score = evaluate_state(&future_state);
                        next_score += piece_bonus[next_piece_type];

                        if (!found_next_move || next_score > next_best_score) {
                            next_best_score = next_score;
                            found_next_move = 1;
                        }
                    }
                }
            }

            // 计算综合评分
            long combined_score;
            if (found_next_move) {
                combined_score = (long)(CURRENT_WEIGHT * (long long)current_eval_score +
                                        NEXT_WEIGHT * (long long)next_best_score);
            } else {
                combined_score = current_eval_score;
            }

            if (!found_any_move || combined_score > best_eval_score)
            {
                best_eval_score = combined_score;
                *best_rot = current_rotation;
                *best_x = x_col;
            }
            else if (combined_score == best_eval_score)
            {
                if (current_rotation < *best_rot)
                {
                    *best_rot = current_rotation;
                    *best_x = x_col;
                }
                else if (current_rotation == *best_rot && abs(x_col + piece->width / 2 - COLS / 2) < abs(*best_x + pieces[current_piece][*best_rot].width / 2 - COLS / 2))
                {
                    *best_x = x_col;
                }
            }
        }
    }

    if (!found_any_move)
    {
        *best_rot = DEG_0;
        *best_x = 0;
        if (pieces[current_piece][DEG_0].width <= COLS)
            *best_x = 0;
        else
            *best_x = COLS - pieces[current_piece][DEG_0].width;
        if (*best_x < 0)
            *best_x = 0;
    }
}