#include <stdio.h>
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
        int v    = maze->treasure_values[pos]; /* pre-assigned at maze load */
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

/**
 * @brief Iterative DFS; returns 1 at the first path that reaches the exit.
 *
 * Iterative (not recursive) because we stop immediately on the first exit —
 * no call-stack undo or backtracking state management is needed.
 *
 * @param maze     Source maze; visited[] is mutated.
 * @param backpack Player's backpack; modified by events along the path.
 * @param path     Stack pre-loaded with player_pos.
 * @param display  Rendering mode.
 * @return         1 if the exit was reached, 0 if no solution exists.
 */
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
            if (!maze->reachable[next])      continue; /* skip dead-end cells */

            char ev_type; int ev_val;
            apply_event(maze, next, backpack, &ev_type, &ev_val);
            maze->visited[next] = 1;
            stack_push(path, next);
            found = 1;
            build_step_desc(step_desc, sizeof(step_desc),
                            "Moved to", next, cols, ev_type, ev_val);
            break; /* commit to the first valid neighbor; backtrack later if needed */
        }

        if (!found) {
            build_step_desc(step_desc, sizeof(step_desc),
                            "Dead end at", current, cols, 0, 0);
            int len = strlen(step_desc);
            snprintf(step_desc + len, sizeof(step_desc) - len, "  —  Backtracking...");
            stack_pop(path);
            /* Note: visited flag is NOT cleared on FIRST backtrack. Once a cell
             * is visited in this mode it is never revisited — avoids cycles and
             * is safe because we only need any one path, not the optimal one. */
        }
    }
    return 0;
}

typedef struct {
    Stack path;
    int   backpack_values[MAX_CELLS]; /* snapshot of winning backpack contents */
    int   backpack_size;
    int   total_value;
    int   found;
} Solution;

typedef struct { char type; int value; } CellEvent;

/**
 * @brief Recursive DFS with full event undo; finds the highest-value exit path.
 *
 * Prunes with branch-and-bound: skips any sub-tree whose optimistic upper bound
 * (current_total + remaining_treasure) cannot beat the best solution so far.
 *
 * @param maze               Source maze; visited[] is mutated and restored on backtrack.
 * @param path               Current exploration path stack.
 * @param backpack           Player's backpack; mutated and restored on backtrack.
 * @param events             Per-cell event record (size MAX_CELLS) used for undo.
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
            best->path          = *path; /* copy the full stack struct (fixed array) */
            best->backpack_size = 0;
            /* Snapshot the backpack because the live list will be mutated as
             * the recursion continues exploring other branches. */
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
        if (!maze->reachable[next])      continue;

        apply_event(maze, next, backpack, &events[next].type, &events[next].value);
        maze->visited[next] = 1;
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
    CellEvent events[MAX_CELLS];
    memset(events, 0, sizeof(events));

    Solution best;
    memset(&best, 0, sizeof(best));

    /* Sum of all pre-assigned treasure values = the tightest possible upper
     * bound at the start (no treasure collected, no traps fired yet). */
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

    /* Restore the winning path and rebuild the backpack from the snapshot.
     * The live stack/backpack are in an arbitrary state after full exploration. */
    *path = best.path;
    list_free(backpack);
    list_init(backpack);
    for (int i = 0; i < best.backpack_size; i++)
        list_insert(backpack, best.backpack_values[i]);

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
    stack_push(&path, maze->player_pos);
    maze->visited[maze->player_pos] = 1;

    if (mode == BACKTRACK_BEST)
        return run_best(maze, backpack, &path, display);
    return run_first(maze, backpack, &path, display);
}
