#include <math.h>
#include <string.h>
#include <limits.h>
#include "tetris.h"
#include "strategy.h"

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
