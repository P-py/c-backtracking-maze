/**
 * @file stack.h
 * @brief Dynamic integer stack used by the backtracking engine.
 *
 * Stores 1D cell indices representing the current path through the maze.
 * Capacity grows automatically via realloc; no fixed upper limit.
 */

#ifndef STACK_H
#define STACK_H

#include <defs.h>

typedef struct {
    int *data;     /**< Heap-allocated storage; NULL when capacity is 0. */
    int  top;      /**< Index of the top element; -1 when empty. */
    int  capacity; /**< Number of slots currently allocated. */
} Stack;

/**
 * @brief Initialise @p s to the empty state (no allocation yet).
 * @param s  Stack to initialise; must not be NULL.
 */
void stack_init(Stack *s);

/**
 * @brief Release the heap storage owned by @p s and reset to empty state.
 * @param s  Stack to free; must not be NULL.
 */
void stack_free(Stack *s);

/**
 * @brief Push @p value onto the top of the stack.
 *
 * Grows the internal array via realloc if needed.
 * Calls exit(1) on allocation failure.
 *
 * @param s      Target stack.
 * @param value  1D cell index to push.
 */
void stack_push(Stack *s, int value);

/**
 * @brief Remove and return the top element.
 * @param s  Target stack.
 * @return   The popped value, or -1 if the stack is empty.
 */
int stack_pop(Stack *s);

/**
 * @brief Return the top element without removing it.
 * @param s  Target stack.
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
