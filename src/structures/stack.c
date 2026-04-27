#include <stdio.h>  /* fprintf, stderr — used by stack_push on fatal alloc failure */
#include <stdlib.h>
#include <stdint.h>
#include <stack.h>

/**
 * @brief Initialise @p s to the empty state.
 *
 * top == -1 is the empty sentinel; all valid 1D cell indices are >= 0,
 * so -1 is unambiguous as the "no element" return value from pop/peek.
 * No memory is allocated until the first push.
 *
 * @param s  Stack to initialise; must not be NULL.
 */
void stack_init(Stack *s) {
    (*s).data     = NULL;
    (*s).top      = -1;
    (*s).capacity = 0;
}

/** @brief Release the heap storage owned by @p s and reset to empty state. */
void stack_free(Stack *s) {
    free((*s).data);
    (*s).data     = NULL;
    (*s).top      = -1;
    (*s).capacity = 0;
}

/** @brief Push @p value onto the stack; grows the array via realloc as needed. */
void stack_push(Stack *s, int value) {
    if ((size_t)((*s).top + 1) == (*s).capacity) {
        if ((*s).capacity > SIZE_MAX / (2 * sizeof(int))) {
            fprintf(stderr, "stack: out of memory\n");
            exit(1);
        }
        size_t new_cap   = (*s).capacity ? (*s).capacity * 2 : 16;
        int   *new_data  = realloc((*s).data, new_cap * sizeof(int));
        if (!new_data) {
            fprintf(stderr, "stack: out of memory\n");
            exit(1);
        }
        (*s).data     = new_data;
        (*s).capacity = new_cap;
    }
    (*s).data[++(*s).top] = value;
}

/** @brief Remove and return the top element; returns -1 on empty. */
int stack_pop(Stack *s) {
    if ((*s).top == -1) return -1;
    return (*s).data[(*s).top--];
}

/** @brief Return the top element without removing it; returns -1 if empty. */
int stack_peek(const Stack *s) {
    if ((*s).top == -1) return -1;
    return (*s).data[(*s).top];
}

/** @brief Return 1 if the stack holds no elements, 0 otherwise. */
int stack_is_empty(const Stack *s) {
    return (*s).top == -1;
}
