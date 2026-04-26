# Implementation Plan

Build order follows the dependency graph — lower layers first, no module depends on something above it.

## Phase 1 — Project Scaffold

Create the directory structure and the Makefile before writing any C.

**Deliverables:**
- `src/`, `src/maze/`, `src/engine/`, `src/structures/`
- `tests/auto/`, `tests/visual/`
- `mazes/`, `output/`
- `Makefile` (see `docs/makefile-patterns.md`)

**Done when:** `make clean && make` produces no errors (even with empty stubs).

## Phase 2 — Data Structures

No dependencies on other modules. Build and test these in isolation.

### 2a. Stack (`src/structures/stack.h` + `stack.c`)

The stack holds `int` values representing 1D cell indices during backtracking.

```c
typedef struct {
    int data[MAX_CELLS];
    int top;
} Stack;

void stack_init(Stack *s);
void stack_push(Stack *s, int value);
int  stack_pop(Stack *s);
int  stack_peek(const Stack *s);
int  stack_is_empty(const Stack *s);
```

**Test (`tests/auto/test_stack.c`):** push/pop sequence, empty-stack peek/pop guard, overflow guard.

### 2b. Sorted Linked List / Backpack (`src/structures/linked_list.h` + `linked_list.c`)

Sorted ascending; head is always the lowest-value treasure.

```c
typedef struct Node Node;
struct Node {
    int value;
    Node *next;
};

typedef struct {
    Node *head;
    int   size;
} LinkedList;

void list_init(LinkedList *l);
void list_insert(LinkedList *l, int value);   // inserts in sorted order
int  list_remove_head(LinkedList *l);         // removes lowest; returns value or -1 if empty
void list_print(const LinkedList *l);
void list_free(LinkedList *l);
```

**Test (`tests/auto/test_linked_list.c`):** insert several values and verify order, remove from empty list, remove-then-insert sequence.

## Phase 3 — Maze

Depends on: nothing (only `stdio.h`, `stdlib.h`, `string.h`).

### `src/maze/maze.h` + `maze.c`

```c
#define MAX_MAZE_SIZE 40
#define MAX_CELLS     (MAX_MAZE_SIZE * MAX_MAZE_SIZE)

typedef struct {
    char cells[MAX_CELLS];   // flattened row-major grid
    char visited[MAX_CELLS]; // 0 = unvisited, 1 = visited
    int  rows;
    int  cols;
    int  player_pos;         // 1D index of 'P'
    int  exit_pos;           // 1D index of 'S'
} Maze;

Maze *maze_load(const char *filepath);  // returns NULL on error
void  maze_free(Maze *m);
int   maze_index(const Maze *m, int row, int col); // row*cols + col
int   maze_is_valid(const Maze *m, int pos);       // in-bounds, not wall, not visited
char  maze_cell(const Maze *m, int pos);
```

**Maze file parsing rules:**
1. First line: `COLSxROWS` (e.g., `10x10`) — note cols comes first.
2. Reject if rows or cols > 40.
3. Each subsequent line is a row; pad or truncate to `cols` if needed.

**No automated test needed here** — validated through the visual test and backtrack test.

## Phase 4 — Engine

### 4a. Renderer (`src/engine/renderer.h` + `renderer.c`)

Depends on: `Maze`, `LinkedList`.

```c
void renderer_draw(const Maze *m, int current_pos, const LinkedList *backpack);
// Clears terminal, prints maze with current_pos marked '@', then backpack contents.
// Uses usleep() for the step delay (e.g., 80 ms).

void renderer_write_solution(const Stack *path, const Maze *m);
// Writes each cell index and its (row, col) to output/solution.txt.
```

Terminal clearing: use `printf("\033[H\033[J")` (ANSI escape — works on Linux/macOS terminals).

**Visual test (`tests/visual/visual_test_maze.c`):** load a maze, simulate a few manual moves, call `renderer_draw` at each step.

### 4b. Backtracking Engine (`src/engine/backtrack.h` + `backtrack.c`)

Depends on: `Maze`, `Stack`, `LinkedList`, `renderer`.

```c
typedef enum { UP = 0, DOWN, LEFT, RIGHT } Direction;

// Returns 1 if exit found, 0 if no solution.
int backtrack_run(Maze *maze, LinkedList *backpack);
```

**Algorithm:**
```
push player_pos onto path_stack
mark player_pos visited

loop:
    current = peek(path_stack)
    renderer_draw(maze, current, backpack)

    if maze_cell(current) == 'S': SUCCESS — write solution, return 1

    found_next = false
    for each direction (UP, DOWN, LEFT, RIGHT):
        next = current + direction_offset
        if maze_is_valid(maze, next):
            handle cell event at next (treasure → list_insert, trap → list_remove_head)
            push next onto path_stack
            mark next visited
            found_next = true
            break

    if !found_next:
        pop path_stack   // backtrack — do NOT undo cell events (treasures/traps are permanent)
        if stack_is_empty: return 0  // no solution
```

Direction offsets: `UP = -cols`, `DOWN = +cols`, `LEFT = -1`, `RIGHT = +1`.

**Trap with empty backpack:** call `list_remove_head`; it returns `-1` — log a warning to `stderr` and continue.

**Test (`tests/auto/test_backtrack.c`):** use a small hardcoded maze string (bypass file I/O), verify `backtrack_run` returns 1 and path hits the exit; test a maze with no solution returns 0.

## Phase 5 — Main Entry Point (`src/main.c`)

Depends on: everything.

```c
int main(int argc, char *argv[]) {
    // 1. Validate argc (expect exactly 1 argument: maze file path)
    // 2. maze_load(argv[1]) — exit on NULL
    // 3. Initialize LinkedList backpack
    // 4. backtrack_run(maze, &backpack)
    // 5. Print final summary to stdout
    // 6. list_free, maze_free
    // 7. return 0
}
```

Final summary format (from README):
```
=== EXIT REACHED ===
Total treasure value: 230 coins
Backpack: [15, 40, 75, 100]
```

## Phase 6 — Sample Mazes

Create the four maze files referenced in the README:
- `mazes/maze_10x10.txt`
- `mazes/maze_20x15.txt`
- `mazes/maze_30x10.txt`
- `mazes/maze_40x40.txt`

Each must have at least one valid path from `P` to `S`, at least one `T`, and at least one `A`.

## Dependency Graph

```
main.c
  ├── maze/maze.c
  ├── engine/backtrack.c
  │     ├── maze/maze.c
  │     ├── structures/stack.c
  │     ├── structures/linked_list.c
  │     └── engine/renderer.c
  │           ├── maze/maze.c
  │           └── structures/linked_list.c
  └── structures/linked_list.c   (for final summary printing)
```

Data structures and `maze.c` have no internal dependencies — start there.
