#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <backtrack.h>
#include <stack.h>
#include <renderer.h>
#include <defs.h>

/* ── Shared helper ──────────────────────────────────────────────────────── */

static void apply_event(Maze *maze, int pos, LinkedList *backpack,
                        char *ev_type, int *ev_val) {
    char cell = maze_cell(maze, pos);
    *ev_type = 0;
    *ev_val  = 0;

    if (cell == CELL_TREASURE) {
        int v    = (rand() % 100) + 1;
        *ev_type = 'T';
        *ev_val  = v;
        list_insert(backpack, v);
    } else if (cell == CELL_TRAP) {
        int lost = list_remove_head(backpack);
        *ev_type = 'A';
        *ev_val  = lost;
        if (lost == -1)
            fprintf(stderr, "Warning: trap with empty backpack\n");
        else
            fprintf(stderr, "Trap! Lost %d coins\n", lost);
    }
}

static void undo_event(LinkedList *backpack, char ev_type, int ev_val) {
    if (ev_type == 'T') {
        list_remove_value(backpack, ev_val);
    } else if (ev_type == 'A' && ev_val != -1) {
        list_insert(backpack, ev_val);
    }
}

static int is_wrap(int current, int d, int cols) {
    if (d == 2 && current % cols == 0)        return 1; /* LEFT  from col 0        */
    if (d == 3 && current % cols == cols - 1) return 1; /* RIGHT from last col     */
    return 0;
}

/* ── FIRST-PATH: iterative DFS, stop at first exit ──────────────────────── */

static int run_first(Maze *maze, LinkedList *backpack, Stack *path) {
    int cols = maze->cols;
    int offsets[4] = {-cols, +cols, -1, +1};

    while (!stack_is_empty(path)) {
        int current = stack_peek(path);
        renderer_draw(maze, current, backpack, path);

        if (maze_cell(maze, current) == CELL_EXIT) {
            renderer_print_solution(path, maze);
            renderer_write_solution(path, maze, backpack);
            return 1;
        }

        int found = 0;
        for (int d = 0; d < 4; d++) {
            if (is_wrap(current, d, cols)) continue;
            int next = current + offsets[d];
            if (!maze_is_valid(maze, next)) continue;

            char ev_type; int ev_val;
            apply_event(maze, next, backpack, &ev_type, &ev_val);
            maze->visited[next] = 1;
            stack_push(path, next);
            found = 1;
            break;
        }

        if (!found) stack_pop(path);
    }
    return 0;
}

/* ── BEST-PATH: recursive DFS with undo, tracks highest-value solution ───── */

typedef struct {
    Stack path;
    int   backpack_values[MAX_CELLS];
    int   backpack_size;
    int   total_value;
    int   found;
} Solution;

/* Per-cell event log (one entry per cell, reused across recursive branches). */
typedef struct { char type; int value; } CellEvent;

static void explore(Maze *maze, Stack *path, LinkedList *backpack,
                    CellEvent *events, Solution *best) {
    int current = stack_peek(path);
    renderer_draw(maze, current, backpack, path);

    if (maze_cell(maze, current) == CELL_EXIT) {
        int total = 0;
        for (Node *n = backpack->head; n; n = n->next) total += n->value;

        if (!best->found || total > best->total_value) {
            best->found       = 1;
            best->total_value = total;
            best->path        = *path; /* copy */
            best->backpack_size = 0;
            for (Node *n = backpack->head; n; n = n->next)
                best->backpack_values[best->backpack_size++] = n->value;
        }
        return;
    }

    int cols = maze->cols;
    int offsets[4] = {-cols, +cols, -1, +1};

    for (int d = 0; d < 4; d++) {
        if (is_wrap(current, d, cols)) continue;
        int next = current + offsets[d];
        if (!maze_is_valid(maze, next)) continue;

        apply_event(maze, next, backpack, &events[next].type, &events[next].value);
        maze->visited[next] = 1;
        stack_push(path, next);

        explore(maze, path, backpack, events, best);

        stack_pop(path);
        maze->visited[next] = 0;
        undo_event(backpack, events[next].type, events[next].value);
    }
}

static int run_best(Maze *maze, LinkedList *backpack, Stack *path) {
    CellEvent events[MAX_CELLS];
    memset(events, 0, sizeof(events));

    Solution best;
    memset(&best, 0, sizeof(best));

    explore(maze, path, backpack, events, &best);

    if (!best.found) return 0;

    /* Restore winning path into caller's stack */
    *path = best.path;

    /* Rebuild backpack with winning haul */
    list_free(backpack);
    list_init(backpack);
    for (int i = 0; i < best.backpack_size; i++)
        list_insert(backpack, best.backpack_values[i]);

    renderer_print_solution(path, maze);
    renderer_write_solution(path, maze, backpack);
    return 1;
}

/* ── Public entry point ─────────────────────────────────────────────────── */

int backtrack_run(Maze *maze, LinkedList *backpack, BacktrackMode mode) {
    Stack path;
    stack_init(&path);
    stack_push(&path, maze->player_pos);
    maze->visited[maze->player_pos] = 1;

    if (mode == BACKTRACK_BEST)
        return run_best(maze, backpack, &path);
    return run_first(maze, backpack, &path);
}
