#include <stdio.h>
#include <stack.h>

/**
 * @brief Initialise @p s to the empty state (top = -1).
 *
 * top == -1 is the empty sentinel; all valid 1D cell indices are >= 0,
 * so -1 is unambiguous as the "no element" return value from pop/peek.
 *
 * @param s  Stack to initialise; must not be NULL.
 */
void stack_init(Stack *s) {
    s->top = -1;
}

/** @brief Push @p value onto the stack; prints to stderr on overflow. */
void stack_push(Stack *s, int value) {
    if (s->top >= MAX_CELLS - 1) {
        fprintf(stderr, "stack overflow\n");
        return;
    }
    s->data[++s->top] = value;
}

/** @brief Remove and return the top element; returns -1 on underflow. */
int stack_pop(Stack *s) {
    if (s->top == -1) {
        fprintf(stderr, "stack underflow\n");
        return -1;
    }
    return s->data[s->top--];
}

/** @brief Return the top element without removing it; returns -1 if empty. */
int stack_peek(const Stack *s) {
    if (s->top == -1) {
        fprintf(stderr, "stack peek on empty stack\n");
        return -1;
    }
    return s->data[s->top];
}

/** @brief Return 1 if the stack holds no elements, 0 otherwise. */
int stack_is_empty(const Stack *s) {
    return s->top == -1;
}
