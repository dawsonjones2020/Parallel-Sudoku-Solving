#include <stdio.h>
#include <stdlib.h>
#include "sudoku.c"
#include "test_boards.h"

int get_mrv(SudokuState* s, int* out_r, int* out_c) {
    int min_options = BOARD_SIZE + 1;
    int best_r = -1, best_c = -1;

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (s->board[r * BOARD_SIZE + c] != 0) continue;

            int box_idx = get_box(r, c);
            int used_mask = s->row_mask[r] | s->col_mask[c] | s->box_mask[box_idx];
            int options_mask = full_mask & ~used_mask;
            int options_count = __builtin_popcount(options_mask);

            if (options_count < min_options) {
                min_options = options_count;
                best_r = r;
                best_c = c;
            }
        }
    }

    *out_r = best_r;
    *out_c = best_c;
    if (best_r == -1) return -1;
    return min_options;
}

int solve_recursive(SudokuState* s) {
    int r, c;
    int num_options = get_mrv(s, &r, &c);
    if (num_options == -1) return 1;
    if (num_options == 0) return 0;

    int box_idx = get_box(r, c);
    int used_mask = s->row_mask[r] | s->col_mask[c] | s->box_mask[box_idx];
    int options_mask = full_mask & ~used_mask;

    for (int num = 1; num <= BOARD_SIZE; num++) {
        if (options_mask & (1 << num)) {
            place(s, r, c, num);
            if (solve_recursive(s)) {
                return 1;
            }
            remove_num(s, r, c, num);
        }
    }

    return 0; // backtrack
}

int main() {
    full_mask = (1 << (BOARD_SIZE + 1)) - 2; // bits 1–board_size set

    SudokuState s;

    load_puzzle(&s, puzzle25_2);
    printf("Puzzle:\n");
    print_board(&s);

    if (solve_recursive(&s)) {
        printf("Solution:\n");
        print_board(&s);
    } else {
        printf("No solution exists.\n");
    }

    return 0;
}
