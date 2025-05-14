#ifndef TETRIS_TYPES_H
#define TETRIS_TYPES_H
// 常量定义
#define ROWS 20
#define COLS 10
#define MAX_PIECES 7
#define SHAPE_DIM 4

// 方块类型枚举
typedef enum {
    I, T, O, J, L, S, Z
} PieceType;

// 旋转角度枚举
typedef enum {
    DEG_0,
    DEG_90,
    DEG_180,
    DEG_270
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

// 预定义所有方块的所有旋转状态
extern Piece pieces[MAX_PIECES][4]; // 7 种方块，每种 4 种旋转

// --- 函数声明 ---
void init_pieces_definitions(void); // Renamed for clarity
PieceType char_to_piece(char c); // Renamed

void select_best_action(GameState *current_board_state, PieceType current_p, PieceType next_p, Rotation *selected_rotation,
                   int *selected_x_col); // Renamed
void suicide_place_piece(GameState *state, PieceType type, Rotation *rot, int *x_col); // Added


#endif // TETRIS_TYPES_H