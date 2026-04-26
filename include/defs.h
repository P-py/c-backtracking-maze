/**
 * @file defs.h
 * @brief Shared constants used across all modules.
 */

#ifndef DEFS_H
#define DEFS_H

#define MAX_MAZE_SIZE 40                              /**< Maximum grid dimension (rows or columns). */
#define MAX_CELLS     (MAX_MAZE_SIZE * MAX_MAZE_SIZE) /**< Maximum flat 1D array size. */

#define CELL_WALL     '#'  /**< Impassable wall. */
#define CELL_CORRIDOR ' '  /**< Open passable corridor. */
#define CELL_PLAYER   'P'  /**< Player starting position. */
#define CELL_TREASURE 'T'  /**< Treasure cell; grants coins on entry. */
#define CELL_TRAP     'A'  /**< Trap cell; removes the lowest-value backpack item. */
#define CELL_EXIT     'S'  /**< Maze exit (goal). */

#endif /* DEFS_H */
