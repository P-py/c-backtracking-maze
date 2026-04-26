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

#include <defs.h>

typedef struct {
    int data[MAX_CELLS]; /**< Storage array; capacity = MAX_CELLS. */
    int top;             /**< Index of the top element; -1 when empty. */
} Stack;

/**
 * @brief Initialise @p s to the empty state.
 * @param s  Stack to initialise; must not be NULL.
 */
void stack_init(Stack *s);

/**
 * @brief Push @p value onto the top of the stack.
 *
 * Prints a diagnostic to stderr and returns without change if the stack is
 * already full (top == MAX_CELLS - 1).
 *
 * @param s      Target stack.
 * @param value  1D cell index to push.
 */
void stack_push(Stack *s, int value);

/**
 * @brief Remove and return the top element.
 * @param s  Non-empty stack.
 * @return   The popped value, or -1 if the stack is empty.
 */
int stack_pop(Stack *s);

/**
 * @brief Return the top element without removing it.
 * @param s  Non-empty stack.
 * @return   The top value, or -1 if the stack is empty.
 */
int stack_peek(const Stack *s);

/**
 * @brief Test whether the stack holds no elements.
 * @param s  Stack to test.
 * @return   1 if empty, 0 otherwise.
 */
int stack_is_empty(const Stack *s);

#endif /* STACK_H */
