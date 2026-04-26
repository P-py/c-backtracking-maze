/**
 * @file stack.h
 * @brief Fixed-capacity integer stack used by the backtracking engine.
 *
 * Stores 1D cell indices representing the current path through the maze.
 * All pop/peek operations must only be called on a non-empty stack —
 * check stack_is_empty() first.
 */

#ifndef STACK_H
#define STACK_H

#include "defs.h"

typedef struct {
    int data[MAX_CELLS]; /**< Storage array; capacity = MAX_CELLS. */
    int top; /**< Index of the next free slot; -1 when empty. */
} Stack;

void stack_init(Stack *s);
void stack_push(Stack *s, int value);
int stack_pop(Stack *s);
int stack_peek(const Stack *s);
int stack_is_empty(const Stack *s);

#endif /* STACK_H */
