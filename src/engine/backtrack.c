#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <backtrack.h>
#include <stack.h>
#include <renderer.h>
#include <defs.h>

/* ── Shared helpers ─────────────────────────────────────────────────────── */

static void apply_event(Maze *maze, int pos, LinkedList *backpack,
                        char *ev_type, int *ev_val) {
    char cell = maze_cell(maze, pos);
    *ev_type = 0;
    *ev_val  = 0;

    if (cell == CELL_TREASURE) {
        int v    = maze->treasure_values[pos]; /* pre-assigned at maze load */
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
    if (d == 2 && current % cols == 0)        return 1;
    if (d == 3 && current % cols == cols - 1) return 1;
    return 0;
}

/* ── Interactive helpers ─────────────────────────────────────────────────── */

static void wait_enter(void) {
    printf("  [Press Enter to continue]");
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void build_step_desc(char *buf, int bufsize,
                             const char *prefix, int pos, int cols,
                             char ev_type, int ev_val) {
    int r = pos / cols, c = pos % cols;
    if (ev_type == 'T')
        snprintf(buf, bufsize, "%s (%d, %d)  —  Treasure found! +%d coins", prefix, r, c, ev_val);
    else if (ev_type == 'A' && ev_val != -1)
        snprintf(buf, bufsize, "%s (%d, %d)  —  Trap! Lost %d coins", prefix, r, c, ev_val);
    else if (ev_type == 'A')
        snprintf(buf, bufsize, "%s (%d, %d)  —  Trap! (backpack was empty)", prefix, r, c);
    else
        snprintf(buf, bufsize, "%s (%d, %d)", prefix, r, c);
}

/* ── FIRST-PATH: iterative DFS ──────────────────────────────────────────── */

static int run_first(Maze *maze, LinkedList *backpack, Stack *path, DisplayMode display) {
    int cols = maze->cols;
    int offsets[4] = {-cols, +cols, -1, +1};

    char step_desc[128];
    build_step_desc(step_desc, sizeof(step_desc),
                    "Starting at", maze->player_pos, cols, 0, 0);

    while (!stack_is_empty(path)) {
        int current = stack_peek(path);

        if (display != DISPLAY_NONE)
            renderer_draw(maze, current, backpack, path);

        if (display == DISPLAY_INTERACTIVE) {
            printf("  %s\n", step_desc);
            if (maze_cell(maze, current) == CELL_EXIT)
                printf("  Exit reached!\n");
        }

        if (maze_cell(maze, current) == CELL_EXIT) {
            if (display == DISPLAY_INTERACTIVE) wait_enter();
            renderer_print_solution(path, maze);
            renderer_write_solution(path, maze, backpack);
            return 1;
        }

        if (display == DISPLAY_INTERACTIVE) wait_enter();

        int found = 0;
        for (int d = 0; d < 4; d++) {
            if (is_wrap(current, d, cols))   continue;
            int next = current + offsets[d];
            if (!maze_is_valid(maze, next))  continue;
            if (!maze->reachable[next])      continue; /* reachability pruning */

            char ev_type; int ev_val;
            apply_event(maze, next, backpack, &ev_type, &ev_val);
            maze->visited[next] = 1;
            stack_push(path, next);
            found = 1;
            build_step_desc(step_desc, sizeof(step_desc),
                            "Moved to", next, cols, ev_type, ev_val);
            break;
        }

        if (!found) {
            build_step_desc(step_desc, sizeof(step_desc),
                            "Dead end at", current, cols, 0, 0);
            int len = strlen(step_desc);
            snprintf(step_desc + len, sizeof(step_desc) - len, "  —  Backtracking...");
            stack_pop(path);
        }
    }
    return 0;
}

/* ── BEST-PATH: recursive DFS with undo + branch-and-bound ──────────────── */

typedef struct {
    Stack path;
    int   backpack_values[MAX_CELLS];
    int   backpack_size;
    int   total_value;
    int   found;
} Solution;

typedef struct { char type; int value; } CellEvent;

static void explore(Maze *maze, Stack *path, LinkedList *backpack,
                    CellEvent *events, Solution *best, DisplayMode display,
                    const char *arrived_msg,
                    int remaining_treasure, int current_total) {
    /* Branch-and-bound: prune if even collecting all remaining treasure
       can't beat the best solution found so far.                         */
    if (best->found && current_total + remaining_treasure <= best->total_value)
        return;

    int current = stack_peek(path);
    int cols    = maze->cols;

    if (display != DISPLAY_NONE)
        renderer_draw(maze, current, backpack, path);

    if (display == DISPLAY_INTERACTIVE) {
        printf("  %s\n", arrived_msg);
        wait_enter();
    }

    if (maze_cell(maze, current) == CELL_EXIT) {
        if (!best->found || current_total > best->total_value) {
            best->found         = 1;
            best->total_value   = current_total;
            best->path          = *path;
            best->backpack_size = 0;
            for (Node *n = backpack->head; n; n = n->next)
                best->backpack_values[best->backpack_size++] = n->value;
        }

        if (display == DISPLAY_INTERACTIVE)
            printf("  Exit! Total so far: %d coins. Searching for better paths...\n",
                   current_total);
        return;
    }

    int offsets[4] = {-cols, +cols, -1, +1};

    for (int d = 0; d < 4; d++) {
        if (is_wrap(current, d, cols))   continue;
        int next = current + offsets[d];
        if (!maze_is_valid(maze, next))  continue;
        if (!maze->reachable[next])      continue; /* reachability pruning */

        apply_event(maze, next, backpack, &events[next].type, &events[next].value);
        maze->visited[next] = 1;
        stack_push(path, next);

        /* Update totals for the recursive call */
        int new_remaining = remaining_treasure;
        int new_total     = current_total;
        char ev_type = events[next].type;
        int  ev_val  = events[next].value;
        if (ev_type == 'T') {
            new_remaining -= ev_val;
            new_total     += ev_val;
        } else if (ev_type == 'A' && ev_val != -1) {
            new_total -= ev_val;
        }

        char msg[128];
        build_step_desc(msg, sizeof(msg), "Moved to", next, cols, ev_type, ev_val);

        explore(maze, path, backpack, events, best, display, msg,
                new_remaining, new_total);

        stack_pop(path);
        maze->visited[next] = 0;
        undo_event(backpack, ev_type, ev_val);

        if (display == DISPLAY_INTERACTIVE) {
            char back_msg[128];
            build_step_desc(back_msg, sizeof(back_msg),
                            "Backtracked to", current, cols, 0, 0);
            printf("  %s\n", back_msg);
        }
    }
}

static int run_best(Maze *maze, LinkedList *backpack, Stack *path, DisplayMode display) {
    CellEvent events[MAX_CELLS];
    memset(events, 0, sizeof(events));

    Solution best;
    memset(&best, 0, sizeof(best));

    /* Sum all pre-assigned treasure values as the initial upper bound */
    int total_treasure = 0;
    for (int i = 0; i < maze->rows * maze->cols; i++)
        total_treasure += maze->treasure_values[i];

    char start_msg[64];
    snprintf(start_msg, sizeof(start_msg),
             "Starting at (%d, %d)",
             maze->player_pos / maze->cols,
             maze->player_pos % maze->cols);

    explore(maze, path, backpack, events, &best, display,
            start_msg, total_treasure, 0);

    if (!best.found) return 0;

    *path = best.path;
    list_free(backpack);
    list_init(backpack);
    for (int i = 0; i < best.backpack_size; i++)
        list_insert(backpack, best.backpack_values[i]);

    renderer_print_solution(path, maze);
    renderer_write_solution(path, maze, backpack);
    return 1;
}

/* ── Public entry point ─────────────────────────────────────────────────── */

int backtrack_run(Maze *maze, LinkedList *backpack, BacktrackMode mode, DisplayMode display) {
    if (display == DISPLAY_INTERACTIVE)
        renderer_set_delay(0);

    Stack path;
    stack_init(&path);
    stack_push(&path, maze->player_pos);
    maze->visited[maze->player_pos] = 1;

    if (mode == BACKTRACK_BEST)
        return run_best(maze, backpack, &path, display);
    return run_first(maze, backpack, &path, display);
}
