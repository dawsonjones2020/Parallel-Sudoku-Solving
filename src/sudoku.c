#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define BOARD_SIZE 25
int full_mask;

// stores sudoku state
typedef struct {
    int board[BOARD_SIZE * BOARD_SIZE];     // 0 = empty
    int row_mask[BOARD_SIZE];
    int col_mask[BOARD_SIZE];
    int box_mask[BOARD_SIZE];
} SudokuState;

// calculates box number based on row col
int get_box(int r, int c) {
    int box_size = sqrt(BOARD_SIZE);
    return (r / box_size) * box_size + (c / box_size);
}

// inits masks to 0, then sets given digits to 1
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

// get minimum remaining values
// good heuristic for choosing cell in DFS
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

// gets maximum remaining values, almost identical to get_mrv
// only used by my parallel solver which needs HIGHER branching factors
// to try and maximize initial task generation
int get_maxrv(SudokuState* s, int* out_r, int* out_c) {
    int max_options = -1;
    int best_r = -1, best_c = -1;

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (s->board[r * BOARD_SIZE + c] != 0) continue;

            int box_idx = get_box(r, c);
            int used_mask = s->row_mask[r] | s->col_mask[c] | s->box_mask[box_idx];
            int options_mask = full_mask & ~used_mask;
            int options_count = __builtin_popcount(options_mask);

            // take HIGHER instead of lower
            if (options_count > max_options) {
                max_options = options_count;
                best_r = r;
                best_c = c;
            }
        }
    }

    *out_r = best_r;
    *out_c = best_c;
    if (best_r == -1) return -1;
    return max_options;
}

// places number on board, updates masks
void place(SudokuState* s, int r, int c, int num) {
    int idx = r * BOARD_SIZE + c;
    int bit = 1 << num;

    s->board[idx] = num;
    s->row_mask[r] |= bit;
    s->col_mask[c] |= bit;
    s->box_mask[get_box(r,c)] |= bit;
}

// remomves number on board, updates masks
void remove_num(SudokuState* s, int r, int c, int num) {
    int idx = r * BOARD_SIZE + c;
    int bit = ~(1 << num);

    s->board[idx] = 0;
    s->row_mask[r] &= bit;
    s->col_mask[c] &= bit;
    s->box_mask[get_box(r,c)] &= bit;
}

// when I was loading from .h file
void load_puzzle(SudokuState* s, int input[BOARD_SIZE][BOARD_SIZE]) {
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            s->board[r*BOARD_SIZE + c] = input[r][c];
        }
    }
    // initialize masks from board
    init_masks(s);
}

// loads puzzle from file
// needed for testing multiple files in hammer wo doing it for each test case
void load_puzzle_from_file(SudokuState* s, const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("Can't find file\n");
        exit(1);
    }
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (fscanf(fp, "%d", &s->board[r * BOARD_SIZE + c]) != 1) {
                fprintf(stderr, "Bad read at (%d,%d)\n", r, c);
                fclose(fp);
                exit(1);
            }
        }
    }
    fclose(fp);
    init_masks(s);
}

// prints board (kind of ugly, but it works)
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

// for testing sudoku logic, not for solving logic
// int main() {
    // board_size = 9;
    // full_mask = (1 << (board_size + 1)) - 2; // bits 1–board_size set

    // SudokuState s;

    // load_puzzle(&s, puzzle1);
    // printf("Puzzle:\n");
    // print_board(&s);

    // return 0;
// }
