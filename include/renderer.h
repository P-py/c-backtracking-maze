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

void renderer_set_delay      (unsigned int delay_us);
void renderer_draw           (const Maze *m, int current_pos,
                              const LinkedList *backpack, const Stack *path);
void renderer_print_solution (const Stack *path, const Maze *m);
void renderer_write_solution (const Stack *path, const Maze *m, const LinkedList *backpack);

#endif /* RENDERER_H */
