/**
 * @file backtrack.h
 * @brief DFS backtracking search through the maze.
 *
 * backtrack_run() drives the full search, calling the renderer at each
 * step and triggering treasure/trap events on the backpack.
 *
 * @return 1 if the exit was reached, 0 if no solution exists.
 */

#ifndef BACKTRACK_H
#define BACKTRACK_H

#include <maze.h>
#include <linked_list.h>

typedef enum {
    BACKTRACK_FIRST = 0, /* stop at the first path that reaches the exit   */
    BACKTRACK_BEST       /* explore all paths; keep the highest-value haul  */
} BacktrackMode;

int backtrack_run(Maze *maze, LinkedList *backpack, BacktrackMode mode);

#endif /* BACKTRACK_H */
