// Sudoku generator + solver (single file)
// Uses bitmasks + MRV + randomized backtracking

#include <stdio.h>
#include <stdlib.h>
#include "test_boards.h"

#define FULL_MASK 0x3FE  // bits 1–9 set (0b1111111110)

typedef struct {
    int board[81];     // 0 = empty
    int row_mask[9];
    int col_mask[9];
    int box_mask[9];
} SudokuState;

int get_box(int r, int c) {
    return (r / 3) * 3 + (c / 3);
}

void init_masks(SudokuState* s) {
    for (int i = 0; i < 9; i++) {
        s->row_mask[i] = 0;
        s->col_mask[i] = 0;
        s->box_mask[i] = 0;
    }

    for (int i = 0; i < 81; i++) {
        int val = s->board[i];
        if (val != 0) {
            int r = i / 9;
            int c = i % 9;
            int b = get_box(r, c);
            int bit = 1 << val;

            s->row_mask[r] |= bit;
            s->col_mask[c] |= bit;
            s->box_mask[b] |= bit;
        }
    }
}

void place(SudokuState* s, int r, int c, int num) {
    int idx = r * 9 + c;
    int bit = 1 << num;

    s->board[idx] = num;
    s->row_mask[r] |= bit;
    s->col_mask[c] |= bit;
    s->box_mask[get_box(r,c)] |= bit;
}

void remove_num(SudokuState* s, int r, int c, int num) {
    int idx = r * 9 + c;
    int bit = ~(1 << num);

    s->board[idx] = 0;
    s->row_mask[r] &= bit;
    s->col_mask[c] &= bit;
    s->box_mask[get_box(r,c)] &= bit;
}

void load_puzzle(SudokuState* s, int input[9][9]) {
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            s->board[r*9 + c] = input[r][c];
        }
    }
    // initialize masks from board
    init_masks(s);
}

void print_board(SudokuState* s) {
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            if (c % 3 == 0 && c != 0) printf("| ");
            printf("%d ", s->board[r * 9 + c]);
        }
        if (r % 3 == 2 && r != 8) printf("\n------+-------+------");
        printf("\n");
    }
}

int main() {

    SudokuState s;

    load_puzzle(&s, puzzle1);
    printf("Puzzle:\n");
    print_board(&s);

    return 0;
}
