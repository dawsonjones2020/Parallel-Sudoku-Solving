# Parallel-Sudoku-Solving
Final Project for HPC Spring 2026

## Sudoku.c

[sudoku.c](src/sudoku.c) handles sudoku board and functions. I will probably keep this separate from the solving logic and it should function on its own. I will create a new file for solving.

### Board Representation

A sudoku board is 9x9 and represented in 1D array of 81 integers.
- Rows are numbered top to bottom (0 to 8)
- Columns are numbered left to right (0 to 8)
- Boxes (the 3x3 subgrids) are numbers in normal reading order i.e. top left to bottom right (0 to 8)

To represent the state of the board at any time, there is a SudokuState struct:

```c
typedef struct {
    int board[81];     // 0 = empty, 1–9 = filled
    int row_mask[9];   // bitmask of used numbers in each row
    int col_mask[9];   // bitmask of used numbers in each column
    int box_mask[9];   // bitmask of used numbers in each 3x3 box
} SudokuState;
```

- board[81] stores the Sudoku numbers in row-major order. Cells not yet filled in are set to 0
- The other arrays are bitmaks used to track which numbers are already placed
    - Each mask is an integer where bit positions 1-9 represent values 1-9. Bit 0 is unused and never changes
    - Example: if a row has numbers 1,3,5 already, the rowmask would be `0b0000101010`.

Hopefully the bitmasks are more efficient than storing domains in each individual cell. We can still calculate all candidates for a given cell:

```c
// get box number
int b = get_box(r, c);

// find all numbers the row col or box already has
int used = s->row_mask[r] | s->col_mask[c] | s->box_mask[b];

// candidates are all values remaining
int candidates = (~used) & FULL_MASK; // FULL_MASK = 0x3FE
```

### Functions Currently Implemented

Currently, the program can load in a board, initialize the masks, and print it. There are also two helper functions to place and remove a value.

- `get_box(r, c)` - returns the box number of a cell
- `init_masks(s)` - takes a state, sets the masks based on the given numbers - used when making a board
- `place(s, r, c, num)` - places a number num in row r and column c in the board state s
- `remove_num(s, r, c, num)` - removes a number num from the board s at row r column c
- `load_puzzle(s, input)` - loads a puzzle into the state. I stored an example puzzle in `test_boards.h`
- `print_board(s)` - prints the board

### Functions To Be Implemented

We still need to be able to generate puzzles randomly. I have also probably overlooked some helper function we will need at some point so feel free to add helper functions as needed, just make sure they are commented and documented.

- `generate_puzzle(s)` - generate a random, solvable puzzle so we don't have to keep loading premade puzzles
- probably some more

**I generated some of this readme using GPT**