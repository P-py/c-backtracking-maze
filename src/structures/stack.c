#include <stdio.h>
#include <stack.h>

void stack_init(Stack *s) {
    s->top = -1;
}

void stack_push(Stack *s, int value) {
    if (s->top >= MAX_CELLS - 1) {
        fprintf(stderr, "stack overflow\n");
        return;
    }
    s->data[++s->top] = value;
}

int stack_pop(Stack *s) {
    if (s->top == -1) {
        fprintf(stderr, "stack underflow\n");
        return -1;
    }
    return s->data[s->top--];
}

int stack_peek(const Stack *s) {
    if (s->top == -1) {
        fprintf(stderr, "stack peek on empty stack\n");
        return -1;
    }
    return s->data[s->top];
}

int stack_is_empty(const Stack *s) {
    return s->top == -1;
}
