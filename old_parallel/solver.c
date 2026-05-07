#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include "sudoku.c"
#include "test_boards.h"

#define NTHREADS 32
#define MAX_QUEUE 10000
#define MAX_DEPTH 2

typedef struct {
    SudokuState state;
    int depth;
} Task;

Task queue[MAX_QUEUE];
int q_head = 0;
int q_tail = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

atomic_int solved = 0;
SudokuState solution;

void enqueue(Task* t) {
    pthread_mutex_lock(&mutex);

    // if queue full, wait to add
    while ((q_tail - q_head) >= MAX_QUEUE && !atomic_load(&solved)) {
        pthread_cond_wait(&cond, &mutex);
    }

    if (!atomic_load(&solved)) {
        queue[q_tail % MAX_QUEUE] = *t;
        q_tail++;
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);
}

int dequeue(Task* t) {
    pthread_mutex_lock(&mutex);

    // if queue is empty, but not solved, wait on new task
    while (!solved && q_head == q_tail) {
        pthread_cond_wait(&cond, &mutex);
    }

    if (atomic_load(&solved)) {
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    *t = queue[q_head % MAX_QUEUE];
    q_head++;

    pthread_mutex_unlock(&mutex);
    return 1;
}

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
    if (num_options == 0) return 0;
    if (num_options == -1) return 1;

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

void* worker(void* arg) {
    Task task;

    while (!solved) {
        if (!dequeue(&task)) continue;
        if (solved) break;

        int r, c;
        int num_options = get_mrv(&task.state, &r, &c);

        // dead end
        if (num_options == 0) continue;

        // -1 ret from num_options means solved
        if (num_options == -1) {
            pthread_mutex_lock(&mutex);
            if (!atomic_exchange(&solved, 1)) {
                solution = task.state;
                atomic_store(&solved, 1);
                pthread_cond_broadcast(&cond);
            }
            pthread_mutex_unlock(&mutex);
            break;
        }

        int box_idx = get_box(r, c);
        int used_mask = task.state.row_mask[r] | task.state.col_mask[c] | task.state.box_mask[box_idx];
        int options_mask = full_mask & ~used_mask;

        // if max split depth is not reached and task queue is not full
        // add new task to pool
        if (task.depth < MAX_DEPTH && (q_tail - q_head) < MAX_QUEUE) {
            for (int num = 1; num <= BOARD_SIZE; num++) {
                if (options_mask & (1 << num)) {
                    Task child;
                    child.state = task.state;
                    child.depth = task.depth + 1;

                    place(&child.state, r, c, num);
                    enqueue(&child);
                }
            }
        } else {
            if (solve_recursive(&task.state)) {
                pthread_mutex_lock(&mutex);
                if (!atomic_load(&solved)) {
                    solution = task.state;
                    atomic_store(&solved, 1);
                    pthread_cond_broadcast(&cond);
                }
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
    }

    return NULL;
}

int main() {
    full_mask = (1 << (BOARD_SIZE + 1)) - 2; // bits 1–board_size set

    SudokuState s;
    load_puzzle(&s, puzzle16);

    printf("Puzzle:\n");
    print_board(&s);

    // start task queue w starting puzzle
    Task t;
    t.state = s;
    t.depth = 0;
    enqueue(&t);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_t threads[NTHREADS];
    for (int i = 0; i < NTHREADS; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
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