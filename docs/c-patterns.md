# C Coding Patterns

Conventions used across this project.

## File Organization

Every module is a pair of files:
- `module.h` — public interface: includes, constants, type definitions, function prototypes only. Include only what the header itself directly uses (`<stddef.h>` for `size_t`, not the whole `<stdlib.h>`).
- `module.c` — implementation: includes its own header first (`<module.h>`), then any others it needs.

All headers live in `include/`. Compilation uses `-Iinclude`, so every file includes with angle brackets:
```c
#include <maze.h>        /* not "maze/maze.h" */
#include <linked_list.h>
```

Header guard format (macro name = filename in `UPPER_SNAKE_CASE`, `.` → `_`):
```c
#ifndef MODULE_NAME_H
#define MODULE_NAME_H

// ...

#endif /* MODULE_NAME_H */
```

File-level doc block at the top of every header:
```c
/**
 * @file stack.h
 * @brief Dynamic integer stack used by the backtracking engine.
 *
 * Capacity grows automatically via realloc; call stack_free() when done.
 * stack_pop() and stack_peek() return -1 when the stack is empty.
 */
```

## Naming

| Kind | Convention | Example |
|---|---|---|
| Types | `PascalCase` | `Stack`, `Node`, `Maze` |
| Functions | `module_verb[_noun]` | `stack_push`, `maze_load`, `list_insert` |
| Parameters / locals | `lower_snake_case` | `cell_count`, `head_node`, `index` |
| Constants / macros | `UPPER_SNAKE_CASE` | `MAX_MAZE_SIZE`, `CELL_WALL` |

The `module_` prefix on every function acts as a namespace — C has no namespaces, so without it a `push` in one module would collide with a `push` in another.

## Struct Patterns

Non-self-referential (fields are heap-allocated pointers; `maze_free` frees them):
```c
typedef struct {
    char *cells;           /**< Heap-allocated flat grid, size rows*cols. */
    char *visited;         /**< Heap-allocated; 0/1 per cell. */
    char *reachable;       /**< Heap-allocated; set by BFS at load time. */
    int  *treasure_values; /**< Heap-allocated; pre-assigned coin values. */
    int   rows;
    int   cols;
    int   player_pos;
    int   exit_pos;
} Maze;
```

Self-referential (node contains pointer to its own type):
```c
typedef struct Node {
    int          value;
    struct Node *next;  /* typedef alias not yet available inside the body */
} Node;
```

Inline member documentation uses `/**< ... */` (note the `<`), which Doxygen attaches to the member on its left rather than to the element below:
```c
typedef struct {
    Node  *head; /**< First node, or NULL when empty. */
    int    size; /**< Current number of nodes.        */
} LinkedList;
```

## Constants

Defined in the header of the module that owns them. Cell-type constants live in `defs.h`; `MAX_MAZE_SIZE` and `MAX_CELLS` were removed when the maze became fully dynamic (arrays are heap-allocated to `rows * cols` at load time):
```c
#define CELL_WALL      '#'
#define CELL_CORRIDOR  ' '
#define CELL_PLAYER    'P'
#define CELL_TREASURE  'T'
#define CELL_TRAP      'A'
#define CELL_EXIT      'S'
```

## Return Conventions

Three patterns depending on what a function does:

**Status functions** return `int` — `1` on success/true, `0` on failure/false:
```c
int stack_is_empty(const Stack *s);
int list_remove_value(LinkedList *l, int value);  /* 1 if found, 0 if not */
```

**Void mutators** that cannot fail (malloc/realloc failure calls `exit()`):
```c
void list_insert(LinkedList *l, int value);  /* exits on malloc failure */
void stack_push(Stack *s, int value);        /* reallocs capacity; exits on failure */
```

**Pop / peek functions** return the value directly and `-1` on empty as a diagnostic sentinel:
```c
int stack_pop(Stack *s);             /* -1 on empty */
int list_remove_head(LinkedList *l); /* -1 on empty */
```

## `const` Correctness

Add `const` to every pointer parameter the function does not modify. This is machine-checked documentation:
```c
/* Read-only — takes const */
int  stack_is_empty  (const Stack *s);
int  stack_peek      (const Stack *s);
void list_print      (const LinkedList *l);

/* Mutating — no const */
void stack_push      (Stack *s, int value);
int  list_remove_head(LinkedList *l);
```

## `size_t` for Sizes and Indices

Use `size_t` (from `<stddef.h>`) for counts, sizes, and index parameters — it is unsigned and matches the platform's address space, eliminating signed/unsigned comparison warnings:
```c
/* example */
size_t count_passable_cells(const Maze *m);
```

In this project, indices into the 1D maze array use plain `int` because offsets can be negative (direction arithmetic); the grid size is `rows * cols` which always fits in `int` for any realistic maze.

## Memory Management

- Node allocation/deallocation is handled entirely inside `linked_list.c`; callers never touch raw `Node` pointers.
- `Maze` is heap-allocated by `maze_load` and freed by `maze_free` — caller owns it.
- Always check `malloc`:
```c
Node *node = malloc(sizeof(Node));
if (!node) {
    fprintf(stderr, "malloc failed\n");
    exit(EXIT_FAILURE);
}
```

## Error Handling

- Functions that open files print to `stderr` and return `NULL` or `0`:
```c
FILE *f = fopen(path, "r");
if (!f) {
    fprintf(stderr, "Error: cannot open maze file '%s'\n", path);
    return NULL;
}
```
- Functions receiving invalid arguments from internal callers may assert or fail silently — internal callers are trusted.
- All user-facing messages go to `stderr`; normal program output goes to `stdout`.

## Function Signatures

Pass structs by pointer, never by value:
```c
int backtrack_run(Maze *maze, LinkedList *backpack, BacktrackMode mode, DisplayMode display);
```

Prefer returning a result directly over writing to a global. Use output parameters only when a function genuinely needs to return two things:
```c
int stack_pop(Stack *s);                        /* good */
void stack_pop(Stack *s, int *out_value);       /* avoid unless two returns are needed */
```

## Doxygen Function Comments

```c
/**
 * @brief Insert @p value into the list in ascending sorted order.
 *
 * @param l     Target list (must not be NULL).
 * @param value Value to insert.
 * @return 1 on success, 0 if memory allocation failed.
 */
int list_insert(LinkedList *l, int value);
```

Key tags: `@brief` (one-line summary), `@param name` (one per parameter), `@return` (what and when), `@note` (non-obvious constraint), `@p name` / `@c token` (monospace in prose).
