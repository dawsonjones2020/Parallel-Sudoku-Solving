#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

// MAKE SURE YOU CHANGE THE POUND DEFINES WHEN RUNNING DIFFERENT SIZES

// #define N 3
// #define LENGTH 9
// #define N 4
// #define LENGTH 16
#define N 5
#define LENGTH 25
// #define N 6
// #define LENGTH 36

// i tried to use pound defines so it can be easily read by yall (i originally just put numbers in the mpi calls)
// tags are used so the master core can get all the info it needs by probing
#define NO_WORK_TAG 0
#define TASK_TAG 1
#define REQUEST_TAG 2
#define SOLUTION_TAG 3
#define STOP_TAG 4


int rank;
int worldSize = 0;


typedef struct Task{
    long puzzle[LENGTH*LENGTH];
} Task;

Task taskQueue[10000]; // 10000 can be changed, i was too lazy to make a queue that can grow so i made it really big

long qfront = 0;
long qback = 0;

int solved = 0;

// i put the if statements to make the output look neat
void printPuzzle(long puzzle[]){
    for (long r = 0; r < LENGTH; r++){
        for (long c=0; c < LENGTH; c++){
            if ((c+1)%N == 0){
                printf("%-5ld", puzzle[r*LENGTH+c]);
            } 
            else{
                printf("%-3ld", puzzle[r*LENGTH+c]);
            }
        }
        if ((r+1)%N == 0){
            printf("\n");
        }
        printf("\n");
    }
}

// the box mask indexing got kinda annoying to type so i made it a function
long boxIdx(int r, int c){
    return (r/N)*N + (c/N);
}

// initializes the masks
void maskInit(long puzzle[], long rowMask[], long colMask[], long boxMask[]){
    // rowMask = calloc(LENGTH, sizeof(int));
    // colMask = calloc(LENGTH, sizeof(int));
    // boxMask = calloc(LENGTH, sizeof(int));

    memset(rowMask, 0, sizeof(long)*LENGTH);
    memset(colMask, 0, sizeof(long)*LENGTH);
    memset(boxMask, 0, sizeof(long)*LENGTH);

    for (long i = 0; i < LENGTH*LENGTH; i++){
        long val = puzzle[i];

        // when a value is found, flip the corresponding bit
        if (val!=0){
            long r = i / LENGTH;
            long c = i % LENGTH;
            long bit = 1 << val;

            rowMask[r] |= bit;
            colMask[c] |= bit;
            boxMask[boxIdx(r,c)] |= bit;
        }
    }
}

// gets the next empty cell, kinda crazy
void nextEmptyCell(long puzzle[], int *r, int *c){
    for (int i = 0; i < LENGTH*LENGTH; i++){
        if (puzzle[i] == 0){
            *r = i / LENGTH;
            *c = i % LENGTH;
            return;
        }
    }
}

int sudokuSolve(long puzzle[], long rowMask[], long colMask[], long boxMask[]){
    int r = -1;
    int c = -1;
    nextEmptyCell(puzzle, &r, &c);
    // printf("%d %d", r, c);
    if (r == -1 || c == -1){
        return -1;
    }

    // contains all the bits for numbers that wont work
    long taken = rowMask[r] | colMask[c] | boxMask[boxIdx(r,c)];

    for (long n = 1; n <= N*N; n++){
        long bit = 1 << n;

        if ((taken & bit) == 0){ // if no overlap, then continue onwards
            puzzle[r*LENGTH+c] = n;
            rowMask[r] |= bit;
            colMask[c] |= bit;
            boxMask[boxIdx(r,c)] |= bit;

            if (sudokuSolve(puzzle, rowMask, colMask, boxMask)){
                return 1;
            }

            // undo because of failure
            puzzle[r*LENGTH+c] = 0;
            rowMask[r] ^= bit;
            colMask[c] ^= bit;
            boxMask[boxIdx(r,c)] ^= bit;
        }
    }
    return 0;
}

int loadPuzzle(const char* filename, Task* t){
    FILE* f = fopen(filename, "r");

    if (f == NULL){
        perror("fopen");
        return 0;
    }

    for (int i = 0; i < LENGTH * LENGTH; i++){
        if (fscanf(f, "%ld", &t->puzzle[i]) != 1){
            printf("Invalid puzzle format\n");
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return 1;
}

void master(char* filename){
    // Task init = { .puzzle = {
    //     5,3,0,0,7,0,0,0,0,
    //     6,0,0,1,9,5,0,0,0,
    //     0,9,8,0,0,0,0,6,0,
    //     8,0,0,0,6,0,0,0,3,
    //     4,0,0,8,0,3,0,0,1,
    //     7,0,0,0,2,0,0,0,6,
    //     0,6,0,0,0,0,2,8,0,
    //     0,0,0,4,1,9,0,0,5,
    //     0,0,0,0,8,0,0,7,9
    // }};

    // Task init = {.puzzle = {
    //     0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0,
    //     0,0,0,0,  0,0,3,0,  0,0,0,0,  0,0,0,0,
    //     0,0,0,0,  0,0,0,0,  0,5,0,0,  0,0,0,0,
    //     0,0,0,0,  0,0,0,0,  0,0,0,0,  7,0,0,0,

    //     0,0,0,0,  0,0,0,0,  0,0,0,8,  0,0,0,0,
    //     0,0,0,0,  0,0,0,0,  0,0,0,0,  0,9,0,0,
    //     0,0,0,0,  0,0,0,0, 10,0,0,0,  0,0,0,0,
    //     0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,11,0,

    //     0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,12,
    //     0,0,0,0, 13,0,0,0,  0,0,0,0,  0,0,0,0,
    //     0,0,0,14, 0,0,0,0,  0,0,0,0,  0,0,0,0,
    //     0,15,0,0, 0,0,0,0,  0,0,0,0,  0,0,0,0,

    //     16,0,0,0, 0,0,0,0,  0,0,0,0,  0,0,0,0,
    //     0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0,
    //     0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0,
    //     0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0
    // }};

    Task init;

    if (!loadPuzzle(filename, &init)){
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    long rowMask[LENGTH];
    long colMask[LENGTH];
    long boxMask[LENGTH];
    maskInit(init.puzzle, rowMask, colMask, boxMask);
    int r = -1;
    int c = -1;
    nextEmptyCell(init.puzzle, &r, &c);
    // if (r == -1 || c == -1){
    //     return;
    // }


    double start = MPI_Wtime();

    // configures the initial task(s), one task per possibility
    long taken = rowMask[r] | colMask[c] | boxMask[boxIdx(r,c)];
    for (int n = 1; n <= LENGTH; n++){
        long bit = 1 << (long)n;
        if ((taken & bit) == 0){
            Task t = init;
            t.puzzle[r*LENGTH+c] = n;
            taskQueue[qback] = t;
            qback++;
        }
    }
    

    while (!solved){
        // master keeps looping here to give out work
        // checks for incoming message
        MPI_Status status;
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        // the tag tells the master branch if a worker found the solution
        int src = status.MPI_SOURCE;

        if (status.MPI_TAG == REQUEST_TAG){
            // they did not find it, so just toss the message
            MPI_Recv(NULL, 0, MPI_INTEGER, src, REQUEST_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // checks to see if there are any tasks left
            if (qfront != qback){
                // if there is work, pop a task from the queue and send it to the worker
                Task t = taskQueue[qfront];
                qfront++;
                MPI_Send(&t, sizeof(Task), MPI_BYTE, src, TASK_TAG, MPI_COMM_WORLD);
            }
            else{
                // if no tasks left, tell the worker to come back later
                MPI_Send(NULL, 0, MPI_INTEGER, src, NO_WORK_TAG, MPI_COMM_WORLD);
            }
        }

        else if (status.MPI_TAG == SOLUTION_TAG){
            // it got solved yay! now tell everyone it got solved and print the solution
            double end = MPI_Wtime();
            solved = 1;
            MPI_Bcast(&solved, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
            Task solution;
            MPI_Recv(&solution, sizeof(Task), MPI_BYTE, src, SOLUTION_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            printf("\nSolution:\n\n");
            printPuzzle(solution.puzzle);
            printf("\nFound in %lf seconds\n", end-start);
            
            // there is probably a better way, but ctrl+c works
            for (int i = 1; i < worldSize; i++){
                MPI_Send(NULL, 0, MPI_INTEGER, i, STOP_TAG, MPI_COMM_WORLD);
            }
        }
    }
}

void worker(){
    long rowMask[LENGTH];
    long colMask[LENGTH];
    long boxMask[LENGTH];
    Task t;
    MPI_Status status;

    while (!solved){
        // worker tells the master it wants to do stuff
        MPI_Send(NULL, 0, MPI_INTEGER, 0, REQUEST_TAG, MPI_COMM_WORLD);
        MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        // if master tells the worker to stop, it stops, because the solution was found
        // there are multiple things to tell the workers to stop but sometimes they keep going after the solution gets printed so just ctrl+c if it stalls after printing
        if (status.MPI_TAG == STOP_TAG){
            MPI_Recv(NULL, 0, MPI_INTEGER, 0, STOP_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            break;
        }

        // if no work, come back later
        else if (status.MPI_TAG == NO_WORK_TAG){
            MPI_Recv(NULL, 0, MPI_INTEGER, 0, NO_WORK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            continue;
        }

        // there is work! take in the current task, initialize the buffers for the state of the puzzle, and try it out
        else if (status.MPI_TAG == TASK_TAG){
            MPI_Recv(&t, sizeof(Task), MPI_BYTE, 0, TASK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            maskInit(t.puzzle, rowMask, colMask, boxMask);

            // if it finds the solution, it tells the master
            if (sudokuSolve(t.puzzle, rowMask, colMask, boxMask)){
                MPI_Send(&t, sizeof(Task), MPI_BYTE, 0, SOLUTION_TAG, MPI_COMM_WORLD);
                break;
            }
        }
    }
}

int main(int argc, char **argv){
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    if (worldSize < 2){
        if (rank == 0){
            printf("Use 2 or more MPI processes\n");
        }

        MPI_Finalize();
        return -1;
    }

    if (argc < 2){
        if (rank == 0){
            printf("use 2 or more cores\n");
        }

        MPI_Finalize();
        return -1;
    }

    if (rank == 0){
        master(argv[1]);
    }
    else{
        worker();
    }

    MPI_Finalize();
    return 0;
}