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
    char cells [MAX_CELLS]; /**< Raw cell characters from the input file. */
    char visited[MAX_CELLS]; /**< 0 = unvisited, 1 = visited. */
    int rows;
    int cols;
    int player_pos; /**< 1D index of the 'P' cell. */
    int exit_pos; /**< 1D index of the 'S' cell. */
} Maze;

Maze *maze_load(const char *filepath);
void maze_free(Maze *m);
int maze_index(const Maze *m, int row, int col);
int maze_is_valid(const Maze *m, int pos);
char maze_cell(const Maze *m, int pos);

#endif /* MAZE_H */
