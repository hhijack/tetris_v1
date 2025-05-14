#include <math.h>
#include <limits.h>
#include "tetris.h"
#include "strategy.h"
#include "rules.h"
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
void select_best_action(GameState *current_board_state, PieceType current_p, PieceType next_p, Rotation *selected_rotation,
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
