#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define BOARD_SIZE 25
int full_mask;

typedef struct {
    int board[BOARD_SIZE * BOARD_SIZE];     // 0 = empty
    int row_mask[BOARD_SIZE];
    int col_mask[BOARD_SIZE];
    int box_mask[BOARD_SIZE];
} SudokuState;

int get_box(int r, int c) {
    int box_size = sqrt(BOARD_SIZE);
    return (r / box_size) * box_size + (c / box_size);
}

void init_masks(SudokuState* s) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        s->row_mask[i] = 0;
        s->col_mask[i] = 0;
        s->box_mask[i] = 0;
    }

    for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++) {
        int val = s->board[i];
        if (val != 0) {
            int r = i / BOARD_SIZE;
            int c = i % BOARD_SIZE;
            int b = get_box(r, c);
            int bit = 1 << val;

            s->row_mask[r] |= bit;
            s->col_mask[c] |= bit;
            s->box_mask[b] |= bit;
        }
    }
}

void place(SudokuState* s, int r, int c, int num) {
    int idx = r * BOARD_SIZE + c;
    int bit = 1 << num;

    s->board[idx] = num;
    s->row_mask[r] |= bit;
    s->col_mask[c] |= bit;
    s->box_mask[get_box(r,c)] |= bit;
}

void remove_num(SudokuState* s, int r, int c, int num) {
    int idx = r * BOARD_SIZE + c;
    int bit = ~(1 << num);

    s->board[idx] = 0;
    s->row_mask[r] &= bit;
    s->col_mask[c] &= bit;
    s->box_mask[get_box(r,c)] &= bit;
}

void load_puzzle(SudokuState* s, int input[BOARD_SIZE][BOARD_SIZE]) {
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            s->board[r*BOARD_SIZE + c] = input[r][c];
        }
    }
    // initialize masks from board
    init_masks(s);
}

void print_board(SudokuState* s) {
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (c % (int)sqrt(BOARD_SIZE) == 0 && c != 0) printf("| ");
            printf("%d ", s->board[r * BOARD_SIZE + c]);
        }
        // print horizontal box separator
        printf("\n");
        if ((r + 1) % (int)sqrt(BOARD_SIZE) == 0 && r != BOARD_SIZE - 1){
            for (int i = 0; i < BOARD_SIZE * 2 + (int)sqrt(BOARD_SIZE); i++){
                printf("-");
            }
            printf("\n");
        }
    }
}

// int main() {
    // board_size = 9;
    // full_mask = (1 << (board_size + 1)) - 2; // bits 1–board_size set

    // SudokuState s;

    // load_puzzle(&s, puzzle1);
    // printf("Puzzle:\n");
    // print_board(&s);

    // return 0;
// }
