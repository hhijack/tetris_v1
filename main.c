#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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
void find_best_move(GameState *current_state, PieceType type, Rotation *best_rot, int *best_x);

int main()
{
    init_pieces();
    //int count = 0; // 用户用于 "第%d个" 消息的 0 索引计数器

    GameState state;
    memset(&state, 0, sizeof(state));
    state.score = 0;
    state.top = 0;

    char initial_input_buffer[10]; // 用于初始 fgets 的缓冲区
    if (fgets(initial_input_buffer, sizeof(initial_input_buffer), stdin) == NULL)
    {
        fprintf(stderr, "读取初始输入时出错。\n");
        return 1;
    }
    initial_input_buffer[strcspn(initial_input_buffer, "\n")] = 0; // 移除换行符

    if (strlen(initial_input_buffer) == 0)
    {
        fprintf(stderr, "初始输入为空。\n");
        return 1;
    }

    // 处理第一个字符就表示终止的情况
    if (initial_input_buffer[0] == 'X')
    {
        // 没有方块被放置，直接退出。如果需要，可以打印特定消息。
        return 0;
    }
    if (initial_input_buffer[0] == 'E')
    {
        exit(0);
    }

    PieceType current_piece = char_to_piece(initial_input_buffer[0]);
    PieceType next_piece;

    if (strlen(initial_input_buffer) >= 2)
    {
        // 检查第二个字符是否为控制字符，再转换为方块类型
        if (initial_input_buffer[1] == 'X' || initial_input_buffer[1] == 'E')
        {
            // 这意味着初始字符串中只有一个方块，后面跟着一个命令。
            // 如果走此路径，`next_piece` 将由循环中的第一个 `scanf` 确定。
            // 现在，将其设置为一个占位符，因为如果循环不立即退出，它会被使用。
            next_piece = I; // 占位符，行为取决于循环逻辑
        }
        else
        {
            next_piece = char_to_piece(initial_input_buffer[1]);
        }
    }
    else
    {
        // 初始输入只有一个方块字符。`next_piece` 将在第一次循环迭代中通过 `scanf` 读取。
        next_piece = I; // 占位符值，将被覆盖
    }

    while (1)
    {
        Rotation best_rot_val = DEG_0;
        int best_x_val = 0;
        find_best_move(&state, current_piece, &best_rot_val, &best_x_val);
        state = place_piece(&state, current_piece, best_rot_val, best_x_val);

        printf("%d %d\n", best_rot_val, best_x_val);
        printf("%d\n", state.score);
        //printf("此时方块最高在%d行\n", state.top);
        // 注意：`count++` 表示先打印当前 `count` 的值，然后再自增。
        // 所以，对于第一个方块，`count` 为 0，打印 0。然后 `count` 变为 1。
        //printf("这是第%d个方块的放置方式和得分\n", count++);
        fflush(stdout);

        if (state.top >= 16)
        { // 游戏结束条件
            break;
        }

        char next_piece_char_input;
        // 如果初始输入只有一个方块，或者第二个字符是 X/E，
        // 那么 `next_piece` 是一个占位符。我们必须读取真正的下一个方块。
        // 用户原来的循环结构总是在这里进行 scanf 操作。
        if (scanf(" %c", &next_piece_char_input) != 1)
        {
            // 读取下一个方块字符时遇到 EOF 或错误
            break;
        }

        if (next_piece_char_input == 'X')
        {
            // 用户特定的 X 块：重新计算并重新放置 `current_piece`
            // 这很不寻常，因为 `current_piece` 刚刚被放置，`state` 也已更新。
            // 重新放置它将在 *已经修改过的* 游戏状态上进行。
            Rotation final_best_rot;
            int final_best_x;
            // 问题描述：“在退出前重新计算最后一个方块的放置方式和得分”
            // 这里指的是 `current_piece`（刚刚放置的那个方块）。
            find_best_move(&state, current_piece, &final_best_rot, &final_best_x);
            state = place_piece(&state, current_piece, final_best_rot, final_best_x); // 游戏状态再次被修改

            printf("%d %d\n", final_best_rot, final_best_x);
            printf("%d\n", state.score);
            //printf("这是主程序。");
            //printf("此时方块最高在%d行\n", state.top);
            // `count` 已经在上面通过 `count++` 自增过了。
            // 所以，对于 "X" 消息，会使用自增后的值。
            //printf("这是最后一个方块的放置方式和得分\n，第%d个\n", count);
            fflush(stdout);
            break;
        }

        if (next_piece_char_input == 'E')
        {
            exit(0);
        }

        // 为下一次迭代做准备
        current_piece = next_piece;                        // 上一次迭代（或初始输入）中的 'next' 方块
        next_piece = char_to_piece(next_piece_char_input); // 通过 scanf 读取的新方块
    }

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

// 评估游戏状态的好坏
int evaluate_state(const GameState *state)
{
    int eval_score = state->score * 10;

    int aggregate_height = 0;
    int max_col_height = 0;
    int heights[COLS] = {0};
    int holes = 0;
    int bumpiness = 0;

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
    }

    for (int c = 0; c < COLS; c++)
    {
        int block_found_in_col = 0;
        for (int r = 0; r < ROWS; r++)
        {
            if (state->grid[r][c])
            {
                block_found_in_col = 1;
            }
            else if (block_found_in_col && r < ROWS - 1)
            { // 空洞必须在方块下方，且不能在最后一行（如果为空）
                if (heights[c] > (ROWS - r))
                { // 只计算严格位于列表面下方的空洞
                    holes++;
                }
            }
        }
    }

    for (int c = 0; c < COLS - 1; c++)
    {
        bumpiness += abs(heights[c] - heights[c + 1]);
    }

    int complete_lines = 0;
    for (int r_eval = 0; r_eval < ROWS; r_eval++)
    {
        int full = 1;
        for (int c_eval = 0; c_eval < COLS; c_eval++)
        {
            if (!state->grid[r_eval][c_eval])
            {
                full = 0;
                break;
            }
        }
        if (full)
            complete_lines++;
    }
    // 常见启发式算法的系数（例如 Pierre Dellacherie 的算法）
    // 这些是示例权重，调优很重要。
    eval_score += (-45 * aggregate_height);
    eval_score += (34 * complete_lines); // 消除行加分
    eval_score += (-32 * holes);
    eval_score += (-18 * bumpiness);
    // eval_score += (-79 * state->top); // state->top 本质上就是 max_col_height

    // 如果最高方块太高，进行严重惩罚（游戏结束条件）
    // state->top 是整体最高点。max_col_height 类似。
    if (state->top >= 16)
    {
        eval_score -= 50000;
    }
    return eval_score;
}

// 寻找最佳放置位置
void find_best_move(GameState *current_state, PieceType type, Rotation *best_rot, int *best_x)
{
    long best_eval_score = LONG_MIN;
    *best_rot = DEG_0;
    *best_x = 0;
    int found_any_move = 0;

    for (int rot_idx = 0; rot_idx < 4; rot_idx++)
    {
        Rotation current_rotation = (Rotation)rot_idx;
        const Piece *piece = &pieces[type][current_rotation];

        for (int x_col = 0; x_col <= COLS - piece->width; x_col++)
        {
            // 检查方块是否能在顶部（y=0）的这个 x_col 和旋转状态下放置
            // 这是一个快速检查，避免在立即不可能的情况下进行完整模拟。
            // 然而，place_piece 本身会寻找下落行。
            // 我们需要确保模拟放置是可能的。
            GameState temp_check_state = *current_state;
            int can_spawn = 0;
            for (int y_try_spawn = 0; y_try_spawn <= ROWS - piece->height; ++y_try_spawn)
            {
                if (can_place_piece(&temp_check_state, type, current_rotation, x_col, y_try_spawn))
                {
                    can_spawn = 1;
                    break;
                }
            }
            if (!can_spawn && piece->height > 0)
            {             // 如果无法生成（且不是空方块）
                // 这个（旋转，x_col）组合从一开始就不可能，或者会立即导致游戏结束。
                // 考虑某些俄罗斯方块版本中方块在第 0 行上方生成的情况。这里假设在网格内生成。
                continue; // 尝试下一个 x_col 或旋转
            }

            GameState next_potential_state = place_piece(current_state, type, current_rotation, x_col);
            int current_eval_score = evaluate_state(&next_potential_state);
            found_any_move = 1;

            if (current_eval_score > best_eval_score)
            {
                best_eval_score = current_eval_score;
                *best_rot = current_rotation;
                *best_x = x_col;
            }
                // 平局决胜：优先选择较小的旋转角度，然后是更居中的 x 坐标
            else if (current_eval_score == best_eval_score)
            {
                if (current_rotation < *best_rot)
                {
                    *best_rot = current_rotation;
                    *best_x = x_col;
                }
                else if (current_rotation == *best_rot && abs(x_col + piece->width / 2 - COLS / 2) < abs(*best_x + pieces[type][*best_rot].width / 2 - COLS / 2))
                {
                    *best_x = x_col;
                }
            }
        }
    }
    if (!found_any_move)
    {
        // 这意味着当前方块没有可行的移动（例如，立即游戏结束）
        // 默认将其放置在 0,0 位置，旋转角度为 0，这可能会导致游戏结束状态。
        *best_rot = DEG_0;
        *best_x = 0;
        // 或者，如果 pieces[type][DEG_0].width > COLS，调整 best_x
        if (pieces[type][DEG_0].width <= COLS)
            *best_x = 0;
        else
            *best_x = COLS - pieces[type][DEG_0].width; // 如果太宽，尝试适应
        if (*best_x < 0)
            *best_x = 0;

        // 更好的游戏结束检查应该是 find_best_move 没有返回有效选项。
    }
}