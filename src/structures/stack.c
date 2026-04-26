#include "structures/stack.h"

void stack_init(Stack *s) {
    s->top = -1;
}

void stack_push(Stack *s, int value) {
    (void)s;
    (void)value;
}

int stack_pop(Stack *s) {
    (void)s;
    return 0;
}

int stack_peek(const Stack *s) {
    (void)s;
    return 0;
}

int stack_is_empty(const Stack *s) {
    (void)s;
    return 1;
}
