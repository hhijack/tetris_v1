#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_PIECES 7 // 方块种类数量
#define SEQUENCE_LENGTH 100000 // 生成的随机序列长度

// 随机生成一个包含 I, T, O, J, L, S, Z 的序列
void generate_random_sequence(char *sequence, int length) {
    const char pieces[NUM_PIECES] = {'I', 'T', 'O', 'J', 'L', 'S', 'Z'};
    for (int i = 0; i < length; i++) {
        int random_index = rand() % NUM_PIECES; // 随机选择一个方块
        sequence[i] = pieces[random_index];
        //printf("%d;",i);
    }
    sequence[length] = '\0'; // 确保字符串以 NULL 结尾
}

int main() {
    // 初始化随机数种子
    srand((unsigned int)time(NULL));

    // 定义存储随机序列的数组
    char random_sequence[SEQUENCE_LENGTH + 1];

    // 生成随机序列
    generate_random_sequence(random_sequence, SEQUENCE_LENGTH);

    // 输出随机序列
    printf("%sX\n", random_sequence);

    return 0;
}
//./random_sequence | ./cmake-build-debug/tetris_v3