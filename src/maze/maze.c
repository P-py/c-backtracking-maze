#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <maze.h>
#include <defs.h>

/**
 * @brief Strip trailing '\r' and '\n' from @p line in-place.
 * @param line  Null-terminated string to modify.
 */
static void strip_newline(char *line) {
    int n = (int)strlen(line);
    while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
        line[--n] = '\0';
}

/** @brief Load a maze from a text file; see maze.h for the full contract. */
Maze *maze_load(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s'\n", filepath);
        return NULL;
    }

    /* --- First pass: buffer all lines to infer dimensions ---
     * cols = widest line (so short trailing lines don't leave garbage cells).
     * rows = non-blank line count. No header line required. */
    char lines[MAX_MAZE_SIZE][256];
    int  rows = 0, cols = 0;

    char buf[256];
    while (rows < MAX_MAZE_SIZE && fgets(buf, sizeof(buf), f)) {
        strip_newline(buf);
        int len = (int)strlen(buf);
        if (len == 0) continue; /* skip blank lines */
        if (len > cols) cols = len;
        memcpy(lines[rows], buf, len + 1);
        rows++;
    }
    fclose(f);

    if (rows == 0 || cols == 0) {
        fprintf(stderr, "Error: empty or blank file '%s'\n", filepath);
        return NULL;
    }
    if (cols > MAX_MAZE_SIZE) {
        fprintf(stderr, "Error: maze width %d exceeds max %d in '%s'\n",
                cols, MAX_MAZE_SIZE, filepath);
        return NULL;
    }

    /* --- Allocate and initialise --- */
    Maze *m = malloc(sizeof(Maze));
    if (!m) {
        fprintf(stderr, "malloc failed\n");
        return NULL;
    }

    /* Default every cell to wall so short lines don't leave uninitialised gaps. */
    memset(m->cells,   CELL_WALL, sizeof(m->cells));
    memset(m->visited, 0,         sizeof(m->visited));
    m->rows       = rows;
    m->cols       = cols;
    m->player_pos = -1;
    m->exit_pos   = -1;

    /* --- Fill grid from buffered lines --- */
    for (int r = 0; r < rows; r++) {
        int len = (int)strlen(lines[r]);
        for (int c = 0; c < cols; c++) {
            char ch  = (c < len) ? lines[r][c] : CELL_WALL;
            int  idx = r * cols + c;

            m->cells[idx] = ch;
            if      (ch == CELL_PLAYER) m->player_pos = idx;
            else if (ch == CELL_EXIT)   m->exit_pos   = idx;
        }
    }

    /* --- Validate required cells --- */

    if (m->player_pos == -1) {
        fprintf(stderr, "Error: no 'P' found in '%s'\n", filepath);
        free(m);
        return NULL;
    }
    if (m->exit_pos == -1) {
        fprintf(stderr, "Error: no 'S' found in '%s'\n", filepath);
        free(m);
        return NULL;
    }

    maze_compute_reachability(m);
    maze_assign_treasures(m);

    return m;
}

/** @brief BFS from the exit to populate reachable[]; see maze.h. */
void maze_compute_reachability(Maze *m) {
    /* BFS backward from the exit — a cell is reachable iff there exists
     * a path of passable cells from it to the exit, ignoring visited state.
     * Running from the exit (not the player) means we don't need player_pos
     * and the result is valid for every possible starting branch of the DFS. */
    memset(m->reachable, 0, sizeof(m->reachable));

    int queue[MAX_CELLS];
    int head = 0, tail = 0;
    int offsets[4] = {-m->cols, +m->cols, -1, +1};

    m->reachable[m->exit_pos] = 1;
    queue[tail++] = m->exit_pos;

    while (head < tail) {
        int pos = queue[head++];
        int col = pos % m->cols;

        for (int d = 0; d < 4; d++) {
            /* Same column-wrap guard as the DFS — BFS and DFS must agree on
             * which moves are legal, or the pruning would block valid paths. */
            if (d == 2 && col == 0)            continue; /* LEFT  wrap */
            if (d == 3 && col == m->cols - 1)  continue; /* RIGHT wrap */

            int next = pos + offsets[d];
            if (next < 0 || next >= m->rows * m->cols) continue;
            if (m->cells[next] == CELL_WALL)            continue;
            if (m->reachable[next])                     continue;

            m->reachable[next] = 1;
            queue[tail++] = next;
        }
    }
}

/** @brief Pre-assign random coin values to all CELL_TREASURE cells; see maze.h. */
void maze_assign_treasures(Maze *m) {
    /* Values are assigned at load time, not on first step, so branch-and-bound
     * can compute the exact total remaining treasure before the DFS starts. */
    memset(m->treasure_values, 0, sizeof(m->treasure_values));
    for (int i = 0; i < m->rows * m->cols; i++) {
        if (m->cells[i] == CELL_TREASURE)
            m->treasure_values[i] = (rand() % 100) + 1;
    }
}

/** @brief Free the heap-allocated Maze returned by maze_load(). */
void maze_free(Maze *m) {
    free(m);
}

/** @brief Convert (row, col) to a flat 1D index: row * m->cols + col. */
int maze_index(const Maze *m, int row, int col) {
    return row * m->cols + col;
}

/** @brief Return 1 if @p pos is in bounds, non-wall, and unvisited. */
int maze_is_valid(const Maze *m, int pos) {
    if (pos < 0 || pos >= m->rows * m->cols) return 0;
    if (m->cells[pos] == CELL_WALL)          return 0;
    if (m->visited[pos])                      return 0;
    return 1;
}

/** @brief Return the raw character stored at cell @p pos. */
char maze_cell(const Maze *m, int pos) {
    return m->cells[pos];
}
