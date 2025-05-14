#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tetris.h"
#include "rules.h"
#include "strategy.h"
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

    PieceType active_piece = char_to_piece(line_buffer[0]);
    PieceType upcoming_piece = (strlen(line_buffer) >= 2 && line_buffer[1] != 'X' && line_buffer[1] != 'E')
                               ? char_to_piece(line_buffer[1]) : I;

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
        upcoming_piece = char_to_piece(next_command_char);
    }
    return 0;
}