/**
 * @file defs.h
 * @brief Shared constants used across all modules.
 */

#ifndef DEFS_H
#define DEFS_H

#define CELL_WALL     '#'  /**< Impassable wall. */
#define CELL_CORRIDOR ' '  /**< Open passable corridor. */
#define CELL_PLAYER   'P'  /**< Player starting position. */
#define CELL_TREASURE 'T'  /**< Treasure cell; grants coins on entry. */
#define CELL_TRAP     'A'  /**< Trap cell; removes the lowest-value backpack item. */
#define CELL_EXIT     'S'  /**< Maze exit (goal). */

#endif /* DEFS_H */
