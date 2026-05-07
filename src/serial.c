#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sudoku.c"
#include "test_boards.h"

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

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Forgot the args dummy\n");
        return 1;
    }

    full_mask = (1 << (BOARD_SIZE + 1)) - 2; // bits 1–board_size set

    SudokuState s;

    load_puzzle_from_file(&s, argv[1]);
    printf("Puzzle:\n");
    print_board(&s);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (solve_recursive(&s)) {
        printf("Solution:\n");
        print_board(&s);
    } else {
        printf("No solution exists.\n");
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Completed in %f seconds.\n", elapsed);

    return 0;
}
