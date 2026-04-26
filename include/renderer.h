/**
 * @file renderer.h
 * @brief Terminal rendering and solution file output.
 *
 * renderer_draw() clears the terminal and prints the maze state plus
 * backpack contents on every step. renderer_write_solution() writes
 * the final path to output/solution.txt.
 */

#ifndef RENDERER_H
#define RENDERER_H

#include <maze.h>
#include <linked_list.h>
#include <stack.h>

/**
 * @brief Set the per-step pause inserted after each renderer_draw() call.
 * @param delay_us  Sleep duration in microseconds; 0 disables sleeping.
 */
void renderer_set_delay(unsigned int delay_us);

/**
 * @brief Clear the terminal and redraw the current maze state.
 *
 * Uses ANSI escape sequences to repaint in place.  The player is shown as '@',
 * CELL_CORRIDOR cells on @p path are shown as '.', and all other cells keep
 * their original characters.  Sleeps for the configured step delay after drawing.
 *
 * @param m            Maze to render.
 * @param current_pos  1D index of the player's position (rendered as '@').
 * @param backpack     Backpack printed below the grid.
 * @param path         Current exploration path; corridor cells on it get '.'.
 */
void renderer_draw(const Maze *m, int current_pos,
                   const LinkedList *backpack, const Stack *path);

/**
 * @brief Print the final solution grid to stdout.
 *
 * CELL_CORRIDOR cells on @p path are replaced with '.'; all other cells
 * keep their original characters so the grid shows what was on the path.
 *
 * @param path  Winning path stack.
 * @param m     Source maze.
 */
void renderer_print_solution(const Stack *path, const Maze *m);

/**
 * @brief Write the solution grid and backpack summary to output/solution.txt.
 *
 * Creates the output/ directory if it does not already exist.
 *
 * @param path     Winning path stack.
 * @param m        Source maze.
 * @param backpack Final backpack contents to include in the summary.
 */
void renderer_write_solution(const Stack *path, const Maze *m, const LinkedList *backpack);

#endif /* RENDERER_H */
