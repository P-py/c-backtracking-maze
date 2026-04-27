#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <backtrack.h>
#include <stack.h>
#include <renderer.h>
#include <defs.h>

/**
 * @brief Apply the cell event at @p pos and record it for undo.
 *
 * Sets *ev_type to 'T' (treasure), 'A' (trap), or 0 (no event).
 * *ev_val is the coin amount gained/lost, or -1 when a trap fires on an
 * empty backpack (nothing to undo).
 *
 * @param maze     Source maze.
 * @param pos      1D index of the cell being entered.
 * @param backpack Player's backpack; mutated by treasure/trap events.
 * @param ev_type  Output: event character ('T', 'A', or 0).
 * @param ev_val   Output: coin value associated with the event.
 */
static void apply_event(Maze *maze, int pos, LinkedList *backpack,
                        char *ev_type, int *ev_val) {
    char cell = maze_cell(maze, pos);
    *ev_type = 0;
    *ev_val  = 0;

    if (cell == CELL_TREASURE) {
        int v    = (*maze).treasure_values[pos]; /* pre-assigned at maze load */
        *ev_type = 'T';
        *ev_val  = v;
        list_insert(backpack, v);
    } else if (cell == CELL_TRAP) {
        int lost = list_remove_head(backpack); /* always removes the lowest-value item */
        *ev_type = 'A';
        *ev_val  = lost;
        if (lost == -1)
            fprintf(stderr, "Warning: trap with empty backpack\n");
        else
            fprintf(stderr, "Trap! Lost %d coins\n", lost);
    }
}

/**
 * @brief Reverse the effect recorded by apply_event().
 *
 * Trap undo re-inserts the lost coin; treasure undo removes it by value.
 * No-op when ev_type is 0 or ev_val is -1 (trap on empty backpack).
 *
 * @param backpack  Player's backpack; restored to pre-event state.
 * @param ev_type   Event character from apply_event() ('T', 'A', or 0).
 * @param ev_val    Coin value from apply_event(); -1 means nothing to undo.
 */
static void undo_event(LinkedList *backpack, char ev_type, int ev_val) {
    if (ev_type == 'T') {
        list_remove_value(backpack, ev_val);
    } else if (ev_type == 'A' && ev_val != -1) {
        list_insert(backpack, ev_val);
    }
}

/**
 * @brief Return 1 if moving in direction @p d from @p current wraps a row boundary.
 *
 * LEFT (d == 2) from column 0 and RIGHT (d == 3) from the last column would
 * land on a cell in an adjacent row — a legal index but an illegal move.
 *
 * @param current  1D index of the source cell.
 * @param d        Direction: 0 = UP, 1 = DOWN, 2 = LEFT, 3 = RIGHT.
 * @param cols     Maze column count.
 * @return         1 if the move wraps, 0 otherwise.
 */
static int is_wrap(int current, int d, int cols) {
    if (d == 2 && current % cols == 0)        return 1;
    if (d == 3 && current % cols == cols - 1) return 1;
    return 0;
}

/** @brief Block until the user presses Enter. */
static void wait_enter(void) {
    printf("  [Press Enter to continue]");
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * @brief Format a human-readable step description into @p buf.
 *
 * @param buf      Output buffer.
 * @param bufsize  Size of @p buf in bytes.
 * @param prefix   Verb phrase, e.g. "Moved to", "Dead end at".
 * @param pos      1D cell index of the current position.
 * @param cols     Maze column count (used to derive row and column).
 * @param ev_type  Event character from apply_event() ('T', 'A', or 0).
 * @param ev_val   Coin value from apply_event().
 */
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

typedef struct { char type; int value; } CellEvent;

/**
 * @brief Iterative DFS; returns 1 at the first path that reaches the exit.
 *
 * Iterative (not recursive) because we stop immediately on the first exit.
 * Cell events are undone on backtrack so the backpack reflects exactly the
 * cells that appear on the final solution path.
 *
 * @param maze     Source maze; visited[] is mutated.
 * @param backpack Player's backpack; modified by events along the path.
 * @param path     Stack pre-loaded with player_pos.
 * @param display  Rendering mode.
 * @return         1 if the exit was reached, 0 if no solution exists.
 */
static int run_first(Maze *maze, LinkedList *backpack, Stack *path, DisplayMode display) {
    int n_cells = (*maze).rows * (*maze).cols;
    int cols    = (*maze).cols;
    int offsets[4] = {-cols, +cols, -1, +1};

    /* Track what happened when we entered each cell so we can undo it on
     * backtrack, keeping the backpack in sync with the current path. */
    CellEvent *events = calloc(n_cells, sizeof(CellEvent));
    if (!events) {
        fprintf(stderr, "malloc failed\n");
        return 0;
    }

    char step_desc[128];
    build_step_desc(step_desc, sizeof(step_desc),
                    "Starting at", (*maze).player_pos, cols, 0, 0);

    int result = 0;
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
            result = 1;
            break;
        }

        if (display == DISPLAY_INTERACTIVE) wait_enter();

        int found = 0;
        for (int d = 0; d < 4; d++) {
            if (is_wrap(current, d, cols))   continue;
            int next = current + offsets[d];
            if (!maze_is_valid(maze, next))  continue;
            if (!(*maze).reachable[next])    continue; /* skip dead-end cells */

            apply_event(maze, next, backpack, &events[next].type, &events[next].value);
            (*maze).visited[next] = 1;
            stack_push(path, next);
            found = 1;
            build_step_desc(step_desc, sizeof(step_desc),
                            "Moved to", next, cols, events[next].type, events[next].value);
            break; /* commit to the first valid neighbor; backtrack later if needed */
        }

        if (!found) {
            build_step_desc(step_desc, sizeof(step_desc),
                            "Dead end at", current, cols, 0, 0);
            int len = strlen(step_desc);
            snprintf(step_desc + len, sizeof(step_desc) - len, "  —  Backtracking...");
            int popped = stack_pop(path);
            /* Undo the event so the backpack reflects only cells still on the path.
             * visited stays 1 — once explored, a cell is never re-entered. */
            undo_event(backpack, events[popped].type, events[popped].value);
        }
    }

    free(events);
    return result;
}

/*
Solution
*/

/* Heap-allocated snapshot of the best path and backpack found during BACKTRACK_BEST.
 * path_data/backpack_values are NULL when found == 0. */
typedef struct {
    int  *path_data;
    int   path_top;
    int  *backpack_values;
    int   backpack_size;
    int   total_value;
    int   found;
} Solution;

/**
 * @brief Recursive DFS with full event undo; finds the highest-value exit path.
 *
 * Prunes with branch-and-bound: skips any sub-tree whose optimistic upper bound
 * (current_total + remaining_treasure) cannot beat the best solution so far.
 *
 * @param maze               Source maze; visited[] is mutated and restored on backtrack.
 * @param path               Current exploration path stack.
 * @param backpack           Player's backpack; mutated and restored on backtrack.
 * @param events             Per-cell event record used for undo.
 * @param best               Accumulates the highest-value solution found so far.
 * @param display            Rendering mode.
 * @param arrived_msg        Human-readable description of the move that reached current.
 * @param remaining_treasure Sum of pre-assigned values of all uncollected treasures.
 * @param current_total      Coins collected on the current path.
 */
static void explore(Maze *maze, Stack *path, LinkedList *backpack,
                    CellEvent *events, Solution *best, DisplayMode display,
                    const char *arrived_msg,
                    int remaining_treasure, int current_total) {
    /* Branch-and-bound pruning: `current_total + remaining_treasure` is an
     * admissible upper bound — it assumes every uncollected treasure is taken
     * with zero trap losses (the most optimistic possible outcome). Pruning
     * when this can't beat the best found is therefore always safe. */
    if ((*best).found && current_total + remaining_treasure <= (*best).total_value)
        return;

    int current = stack_peek(path);
    int cols    = (*maze).cols;

    if (display != DISPLAY_NONE)
        renderer_draw(maze, current, backpack, path);

    if (display == DISPLAY_INTERACTIVE) {
        printf("  %s\n", arrived_msg);
        wait_enter();
    }

    if (maze_cell(maze, current) == CELL_EXIT) {
        if (!(*best).found || current_total > (*best).total_value) {
            (*best).found       = 1;
            (*best).total_value = current_total;

            /* Deep-copy the path so subsequent exploration doesn't corrupt it. */
            int path_len = (*path).top + 1;
            free((*best).path_data);
            (*best).path_data = malloc(path_len * sizeof(int));
            if (!(*best).path_data) { fprintf(stderr, "malloc failed\n"); exit(1); }
            memcpy((*best).path_data, (*path).data, path_len * sizeof(int));
            (*best).path_top = (*path).top;

            /* Deep-copy the backpack contents. */
            free((*best).backpack_values);
            (*best).backpack_size   = 0;
            (*best).backpack_values = NULL;
            if ((*backpack).size > 0) {
                (*best).backpack_values = malloc((*backpack).size * sizeof(int));
                if (!(*best).backpack_values) { fprintf(stderr, "malloc failed\n"); exit(1); }
            }
            for (Node *n = (*backpack).head; n; n = (*n).next)
                (*best).backpack_values[(*best).backpack_size++] = (*n).value;
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
        if (!(*maze).reachable[next])    continue;

        apply_event(maze, next, backpack, &events[next].type, &events[next].value);
        (*maze).visited[next] = 1;
        stack_push(path, next);

        /* Both counters are passed by value into the child call, so the parent's
         * copies are untouched when the child returns — no explicit numeric undo. */
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

        /* Undo structural state (stack, visited, backpack) so this cell can be
         * entered again from a different parent branch. */
        stack_pop(path);
        (*maze).visited[next] = 0;
        undo_event(backpack, ev_type, ev_val);

        if (display == DISPLAY_INTERACTIVE) {
            char back_msg[128];
            build_step_desc(back_msg, sizeof(back_msg),
                            "Backtracked to", current, cols, 0, 0);
            printf("  %s\n", back_msg);
        }
    }
}

/**
 * @brief Driver for BACKTRACK_BEST: explores all paths and restores the best one.
 *
 * After full exploration, rebuilds @p path and @p backpack from the winning
 * solution snapshot.
 *
 * @param maze     Source maze.
 * @param backpack Rebuilt from the best-solution snapshot after exploration.
 * @param path     Rebuilt to the winning path after exploration.
 * @param display  Rendering mode.
 * @return         1 if any solution exists, 0 otherwise.
 */
static int run_best(Maze *maze, LinkedList *backpack, Stack *path, DisplayMode display) {
    int n_cells = (*maze).rows * (*maze).cols;

    CellEvent *events = calloc(n_cells, sizeof(CellEvent));
    if (!events) {
        fprintf(stderr, "malloc failed\n");
        return 0;
    }

    Solution best;
    memset(&best, 0, sizeof(best));

    /* Sum of all pre-assigned treasure values = the tightest possible upper
     * bound at the start (no treasure collected, no traps fired yet). */
    int total_treasure = 0;
    for (int i = 0; i < n_cells; i++)
        total_treasure += (*maze).treasure_values[i];

    char start_msg[64];
    snprintf(start_msg, sizeof(start_msg),
             "Starting at (%d, %d)",
             (*maze).player_pos / (*maze).cols,
             (*maze).player_pos % (*maze).cols);

    explore(maze, path, backpack, events, &best, display,
            start_msg, total_treasure, 0);

    free(events);

    if (!best.found) {
        free(best.path_data);
        free(best.backpack_values);
        return 0;
    }

    /* Rebuild the path stack from the winning snapshot. */
    stack_free(path);
    stack_init(path);
    for (int i = 0; i <= best.path_top; i++)
        stack_push(path, best.path_data[i]);
    free(best.path_data);

    /* Rebuild the backpack from the winning snapshot. */
    list_free(backpack);
    list_init(backpack);
    for (int i = 0; i < best.backpack_size; i++)
        list_insert(backpack, best.backpack_values[i]);
    free(best.backpack_values);

    renderer_print_solution(path, maze);
    renderer_write_solution(path, maze, backpack);
    return 1;
}

/** @brief Run the maze solver; see backtrack.h for the full contract. */
int backtrack_run(Maze *maze, LinkedList *backpack, BacktrackMode mode, DisplayMode display) {
    /* Interactive mode drives its own pacing via wait_enter(); the renderer
     * delay would add unwanted latency on top of the user's keystrokes. */
    if (display == DISPLAY_INTERACTIVE)
        renderer_set_delay(0);

    Stack path;
    stack_init(&path);
    stack_push(&path, (*maze).player_pos);
    (*maze).visited[(*maze).player_pos] = 1;

    int result;
    if (mode == BACKTRACK_BEST)
        result = run_best(maze, backpack, &path, display);
    else
        result = run_first(maze, backpack, &path, display);

    stack_free(&path);
    return result;
}
