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

#include "maze/maze.h"
#include "structures/linked_list.h"
#include "structures/stack.h"

void renderer_draw (const Maze *m, int current_pos, const LinkedList *backpack);
void renderer_write_solution (const Stack *path, const Maze *m);

#endif /* RENDERER_H */
