#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include "sudoku.c"
#include "test_boards.h"

#define NTHREADS 32
#define MAX_STATES 10000

typedef struct {
    SudokuState states[MAX_STATES];
    int top;
    int bottom;
    pthread_mutex_t lock;
} ThreadStack;

ThreadStack stacks[NTHREADS];
atomic_int solved = 0;
SudokuState solution;

int popped = 0;

void push(ThreadStack *ts, SudokuState s) {
    pthread_mutex_lock(&ts->lock);
    ts->states[ts->top++] = s;
    pthread_mutex_unlock(&ts->lock);
}

// ret 0 if no task available
// ret 1 if task successfully grabbed
int pop(ThreadStack *ts, SudokuState *ret_state) {
    pthread_mutex_lock(&ts->lock);
    if (ts->top == ts->bottom) {
        pthread_mutex_unlock(&ts->lock);
        return 0; // stack is empty
    }
    *ret_state = ts->states[--ts->top];
    pthread_mutex_unlock(&ts->lock);
    printf("Popped %d\n", popped);
    popped++;
    return 1;
}

// ret 0 if no task available to steal
// ret 1 if task stolen
int steal(ThreadStack *ts, SudokuState *ret_state) {
    pthread_mutex_lock(&ts->lock);
    // if stack empty
    if (ts->top == ts->bottom) {
        pthread_mutex_unlock(&ts->lock);
        return 0;
    }

    *ret_state = ts->states[ts->bottom++];
    pthread_mutex_unlock(&ts->lock);
    return 1;
}

void init_states(SudokuState *s, SudokuState *states, int *count, int max_states, int depth) {
    if (*count >= max_states) return;

    // higher depth is better for constrained boards
    // lower is better for open boards (2 for 250+)
    if (depth >= 2) {
        states[(*count)++] = *s;
        return;
    }

    int r, c;
    int num_options = get_maxrv(s, &r, &c);
    if (num_options <= 0) return;

    int box_idx = get_box(r, c);
    int used_mask = s->row_mask[r] | s->col_mask[c] | s->box_mask[box_idx];
    int options_mask = full_mask & ~used_mask;

    // expand tree
    for (int num = 1; num <= BOARD_SIZE; num++) {
        if (*count >= max_states) return;
        if (options_mask & (1 << num)) {
            SudokuState child = *s;
            place(&child, r, c, num);
            init_states(&child, states, count, max_states, depth + 1);
            if (*count >= max_states) return;
        } 
    }
}


void solve_recursive(SudokuState* s) {
    if (atomic_load(&solved)) return;

    int r, c;
    int num_options = get_mrv(s, &r, &c);
    if (num_options == 0) return;
    if (num_options == -1) {
        if (!atomic_exchange(&solved, 1)) solution = *s; // can change later
        return;
    }

    int box_idx = get_box(r, c);
    int used_mask = s->row_mask[r] | s->col_mask[c] | s->box_mask[box_idx];
    int options_mask = full_mask & ~used_mask;

    for (int num = 1; num <= BOARD_SIZE; num++) {
        if (options_mask & (1 << num)) {
            SudokuState child = *s;
            place(&child, r, c, num);
            solve_recursive(&child);
            if (atomic_load(&solved)) return;
        }
    }
}

void* worker(void* arg) {
    int id = *(int*)arg;
    ThreadStack *my_stack = &stacks[id];

    SudokuState s;

    while (!atomic_load(&solved)) {
        if (!pop(my_stack, &s)) {
            // attempt a steal
            int stolen = 0;
            for (int i = 0; i < NTHREADS; i++) {
                if (i == id) continue;
                if (steal(&stacks[i], &s)) {
                    stolen = 1;
                    break;
                }
            }
            if (!stolen) continue; // no tasks in anyones queue. just keep looping
        }
        solve_recursive(&s);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("you forgot the args dummy\n");
        return 1;
    }

    full_mask = (1 << (BOARD_SIZE + 1)) - 2; // bits 1–board_size set

    SudokuState s;
    load_puzzle_from_file(&s, argv[1]);

    printf("Puzzle:\n");
    print_board(&s);

    // start stacks
    for (int i = 0; i < NTHREADS; i++) {
        stacks[i].top = 0;
        stacks[i].bottom = 0;
        pthread_mutex_init(&stacks[i].lock, NULL);
    }

    // create inital states
    SudokuState initial_states[1000];
    int state_count = 0;
    init_states(&s, initial_states, &state_count, 4 * NTHREADS, 0);
    printf("Generated %d states.\n", state_count);

    // gives states to thread stacks
    for (int i = 0; i < state_count; i++) {
        push(&stacks[i % NTHREADS], initial_states[i]);
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_t threads[NTHREADS];
    int ids[NTHREADS];
    for (int i = 0; i < NTHREADS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }

    for (int i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    if (solved) {
        printf("Solution:\n");
        print_board(&solution);
    } else {
        printf("No solution exists.\n");
    }

    printf("Completed in %f seconds.\n", elapsed);

    return 0;
}