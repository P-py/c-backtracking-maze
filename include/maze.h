/**
 * @file maze.h
 * @brief Maze loading, cell queries, and visited-state tracking.
 *
 * The grid is stored as a flat row-major char array.
 * index(row, col) = row * cols + col.
 * maze_load() returns NULL on any error (file not found, bad dimensions).
 */

#ifndef MAZE_H
#define MAZE_H

#include <stddef.h>
#include "defs.h"

typedef struct {
    char *cells;           /**< Heap-allocated flat array of raw cell characters. */
    char *visited;         /**< Heap-allocated; 0 = unvisited, 1 = visited. */
    char *reachable;       /**< Heap-allocated; 1 if cell can reach the exit (BFS from exit). */
    int  *treasure_values; /**< Heap-allocated; pre-assigned coin value for each CELL_TREASURE. */
    int rows;
    int cols;
    int player_pos; /**< 1D index of the 'P' cell. */
    int exit_pos;   /**< 1D index of the 'S' cell. */
} Maze;

/**
 * @brief Load a maze from a text file and return a heap-allocated Maze.
 *
 * Dimensions are inferred: rows = non-blank line count, cols = widest line.
 * Calls maze_compute_reachability() and maze_assign_treasures() before returning.
 *
 * @param filepath  Path to the .txt maze file.
 * @return          Heap-allocated Maze, or NULL on any error.
 */
Maze *maze_load(const char *filepath);

/**
 * @brief Free a Maze returned by maze_load().
 * @param m  Maze to free; must not be NULL.
 */
void maze_free(Maze *m);

/**
 * @brief Convert (row, col) coordinates to a flat 1D index.
 * @param m    Maze whose column count defines the stride.
 * @param row  Zero-based row.
 * @param col  Zero-based column.
 * @return     row * m->cols + col.
 */
int maze_index(const Maze *m, int row, int col);

/**
 * @brief Return 1 if @p pos is in bounds, non-wall, and unvisited.
 * @param m    Maze to query.
 * @param pos  1D cell index to test.
 * @return     1 if the cell is a valid move target, 0 otherwise.
 */
int maze_is_valid(const Maze *m, int pos);

/**
 * @brief Return the raw character stored at cell @p pos.
 * @param m    Source maze.
 * @param pos  1D cell index; must be in bounds.
 * @return     One of: CELL_WALL, CELL_CORRIDOR, CELL_PLAYER, CELL_TREASURE,
 *             CELL_TRAP, or CELL_EXIT.
 */
char maze_cell(const Maze *m, int pos);

/**
 * @brief BFS from the exit to fill reachable[] for every passable cell.
 *
 * A cell is marked reachable iff a path of non-wall cells connects it to
 * the exit, regardless of visited state.  Called once at load time.
 *
 * @param m  Maze whose reachable[] array will be populated.
 */
void maze_compute_reachability(Maze *m);

/**
 * @brief Pre-assign random coin values (1–100) to all CELL_TREASURE cells.
 *
 * Fixing values at load time lets branch-and-bound compute an exact upper
 * bound on remaining treasure before the DFS begins.
 *
 * @param m  Maze whose treasure_values[] array will be populated.
 */
void maze_assign_treasures(Maze *m);

#endif /* MAZE_H */
