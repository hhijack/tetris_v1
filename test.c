#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tetris.h"
#include "strategy.h"

int main()
{
    init_pieces();
    int count = 0; // 用户用于 "第%d个" 消息的 0 索引计数器

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
        count+=1;
        // 注意：`count++` 表示先打印当前 `count` 的值，然后再自增。
        // 所以，对于第一个方块，`count` 为 0，打印 0。然后 `count` 变为 1。
        //printf("这是第%d个方块的放置方式和得分\n", count++);
        fflush(stdout);

        if (state.top >= 16)
        { // 游戏结束条件
            printf("游戏结束，第%d个\n", count);
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
            printf("这是测试程序。");
            printf("此时方块最高在%d行\n", state.top);
            // `count` 已经在上面通过 `count++` 自增过了。
            // 所以，对于 "X" 消息，会使用自增后的值。
            printf("这是最后一个方块的放置方式和得分\n，第%d个\n", count);
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
