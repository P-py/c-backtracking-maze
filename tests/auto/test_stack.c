#include <stdio.h>
#include <stack.h>

static int failures = 0;

#define ASSERT(cond, msg) \
    do { \
        if (cond) { printf("  PASS: %s\n", msg); } \
        else      { printf("  FAIL: %s\n", msg); failures++; } \
    } while (0)

static void test_init(void) {
    printf("init:\n");
    Stack s;
    stack_init(&s);
    ASSERT(stack_is_empty(&s), "is_empty after init");
}

static void test_push_peek_pop(void) {
    printf("push / peek / pop:\n");
    Stack s;
    stack_init(&s);

    stack_push(&s, 42);
    ASSERT(!stack_is_empty(&s),   "not empty after push");
    ASSERT(stack_peek(&s) == 42,  "peek returns pushed value");
    ASSERT(!stack_is_empty(&s),   "peek does not remove item");

    int v = stack_pop(&s);
    ASSERT(v == 42,               "pop returns correct value");
    ASSERT(stack_is_empty(&s),    "is_empty after pop");
    stack_free(&s);
}

static void test_lifo_order(void) {
    printf("LIFO order:\n");
    Stack s;
    stack_init(&s);

    stack_push(&s, 1);
    stack_push(&s, 2);
    stack_push(&s, 3);

    ASSERT(stack_pop(&s) == 3, "pop 1st = 3");
    ASSERT(stack_pop(&s) == 2, "pop 2nd = 2");
    ASSERT(stack_pop(&s) == 1, "pop 3rd = 1");
    ASSERT(stack_is_empty(&s), "empty after 3 pops");
    stack_free(&s);
}

static void test_underflow_guard(void) {
    printf("underflow guard:\n");
    Stack s;
    stack_init(&s);

    int v = stack_pop(&s);
    ASSERT(v == -1, "pop on empty returns -1");

    int p = stack_peek(&s);
    ASSERT(p == -1, "peek on empty returns -1");
}

static void test_many_pushes(void) {
    printf("multiple pushes and pops:\n");
    Stack s;
    stack_init(&s);

    for (int i = 0; i < 100; i++)
        stack_push(&s, i);

    int ok = 1;
    for (int i = 99; i >= 0; i--)
        if (stack_pop(&s) != i) { ok = 0; break; }

    ASSERT(ok,                 "100 items pop in LIFO order");
    ASSERT(stack_is_empty(&s), "empty after 100 pops");
    stack_free(&s);
}

int main(void) {
    test_init();
    test_push_peek_pop();
    test_lifo_order();
    test_underflow_guard();
    test_many_pushes();

    printf("\n%s\n", failures == 0 ? "All tests passed." : "Some tests FAILED.");
    return failures > 0 ? 1 : 0;
}
