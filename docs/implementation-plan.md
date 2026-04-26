# Implementation Plan

Build order follows the dependency graph — lower layers first, no module depends on something above it.

## Phase 1 — Project Scaffold ✓

Create the directory structure and the Makefile before writing any C.

**Deliverables:**
- `src/`, `src/maze/`, `src/engine/`, `src/structures/`
- `tests/auto/`, `tests/visual/`
- `mazes/`, `output/`
- `Makefile` (see `docs/makefile-patterns.md`)

**Done when:** `make clean && make` produces no errors (even with empty stubs).

## Phase 2 — Data Structures ✓

No dependencies on other modules. Build and test these in isolation.

### 2a. Stack (`src/structures/stack.h` + `stack.c`) ✓

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

### 2b. Sorted Linked List / Backpack (`src/structures/linked_list.h` + `linked_list.c`) ✓

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

void list_init        (LinkedList *l);
void list_insert      (LinkedList *l, int value);   // inserts in sorted order
int  list_remove_head (LinkedList *l);              // removes lowest; returns value or -1 if empty
int  list_remove_value(LinkedList *l, int value);   // removes first node matching value (best-path undo)
void list_print       (const LinkedList *l);
void list_free        (LinkedList *l);
```

**Test (`tests/auto/test_linked_list.c`):** insert several values and verify order, remove from empty list, remove-then-insert sequence.

## Phase 3 — Maze ✓

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

## Phase 4 — Engine ✓

### 4a. Renderer (`src/engine/renderer.h` + `renderer.c`) ✓

Depends on: `Maze`, `LinkedList`, `Stack`.

```c
void renderer_set_delay     (unsigned int delay_us);
void renderer_draw          (const Maze *m, int current_pos,
                             const LinkedList *backpack, const Stack *path);
void renderer_print_solution(const Stack *path, const Maze *m);
void renderer_write_solution(const Stack *path, const Maze *m);
```

- `renderer_draw`: clears terminal (`\033[H\033[J`), prints maze with `@` at current position, `.` on corridor cells that are on the current path, and original characters elsewhere. Then prints backpack contents. Sleeps `step_delay_us` microseconds (default 80 ms).
- `renderer_print_solution`: prints the final solved maze with `.` on the solution path to stdout.
- `renderer_write_solution`: writes step-by-step path (index, row, col, cell char) to `output/solution.txt`.
- `renderer_set_delay`: sets the per-step delay (pass 0 in tests to disable).

### 4b. Backtracking Engine (`src/engine/backtrack.h` + `backtrack.c`) ✓

Depends on: `Maze`, `Stack`, `LinkedList`, `renderer`.

```c
typedef enum {
    BACKTRACK_FIRST = 0,  // stop at first exit found
    BACKTRACK_BEST        // exhaustive search, return path with highest treasure value
} BacktrackMode;

int backtrack_run(Maze *maze, LinkedList *backpack, BacktrackMode mode);
```

**BACKTRACK_FIRST — iterative DFS:**
```
push player_pos; mark visited
loop:
    current = peek(stack)
    renderer_draw(...)
    if cell == 'S': print solution, return 1
    for each direction:
        if neighbor valid:
            apply_event(neighbor, backpack)   // T → insert, A → remove_head
            mark neighbor visited; push; break
    if no neighbor found: pop (backtrack)
return 0 (no solution)
```

**BACKTRACK_BEST — recursive DFS with undo:**
```
explore(current):
    renderer_draw(...)
    if cell == 'S':
        compute total; if total > best: save path + backpack snapshot
        return
    for each direction:
        apply_event(neighbor, &events[neighbor])
        mark visited; push; explore(neighbor)
        pop; unmark visited; undo_event(events[neighbor])

Undo rules: T → list_remove_value(value); A → list_insert(value) if value != -1
```

**Column-wrap guard:** skip LEFT if `current % cols == 0`; skip RIGHT if `current % cols == cols-1`.

**Test (`tests/auto/test_backtrack.c`):** 8 tests — simple path, no solution, treasure, trap, dead-end backtrack (first mode); exit found, no solution, picks richer path (best mode). Mazes are inline strings, no file I/O. Delay set to 0 via `renderer_set_delay(0)`.

**Visual test (`tests/visual/visual_test_maze.c`):** accepts `[first|best]` as second arg; calls `backtrack_run` and prints summary.

## Phase 5 — Main Entry Point (`src/main.c`)

Depends on: everything.

```c
int main(int argc, char *argv[]) {
    // 1. Validate argc (expect maze file + optional [first|best])
    // 2. Parse mode: default BACKTRACK_FIRST, "best" → BACKTRACK_BEST
    // 3. srand(time(NULL))
    // 4. maze_load(argv[1]) — exit on NULL
    // 5. list_init(&backpack)
    // 6. backtrack_run(maze, &backpack, mode)
    // 7. Print final summary
    // 8. list_free, maze_free
}
```

Final summary format:
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
  │           ├── structures/linked_list.c
  │           └── structures/stack.c     (for on_path[] lookup)
  └── structures/linked_list.c   (for final summary printing)
```

Data structures and `maze.c` have no internal dependencies — start there.
