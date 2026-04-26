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

int backtrack_run(Maze *maze, LinkedList *backpack);

#endif /* BACKTRACK_H */
