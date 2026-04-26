#include <stdio.h>
#include <linked_list.h>

static int failures = 0;

#define ASSERT(cond, msg) \
    do { \
        if (cond) { printf("  PASS: %s\n", msg); } \
        else      { printf("  FAIL: %s\n", msg); failures++; } \
    } while (0)

/* Walk the list and verify each node's value equals expected[i]. */
static int verify_order(const LinkedList *l, const int *expected, int len) {
    Node *curr = l->head;
    for (int i = 0; i < len; i++) {
        if (!curr || curr->value != expected[i]) return 0;
        curr = curr->next;
    }
    return curr == NULL; /* no extra nodes */
}

static void test_init(void) {
    printf("init:\n");
    LinkedList l;
    list_init(&l);
    ASSERT(l.head == NULL, "head is NULL after init");
    ASSERT(l.size == 0,    "size is 0 after init");
}

static void test_sorted_insert(void) {
    printf("sorted insert:\n");
    LinkedList l;
    list_init(&l);

    list_insert(&l, 40);
    list_insert(&l, 15);
    list_insert(&l, 75);
    list_insert(&l, 5);
    list_insert(&l, 60);

    int expected[] = {5, 15, 40, 60, 75};
    ASSERT(l.size == 5,                          "size == 5 after 5 inserts");
    ASSERT(verify_order(&l, expected, 5),         "order is ascending");
    ASSERT(l.head->value == 5,                   "head holds minimum");

    list_free(&l);
}

static void test_duplicate_insert(void) {
    printf("duplicate values:\n");
    LinkedList l;
    list_init(&l);

    list_insert(&l, 30);
    list_insert(&l, 30);
    list_insert(&l, 10);

    int expected[] = {10, 30, 30};
    ASSERT(l.size == 3,                    "size == 3 with duplicates");
    ASSERT(verify_order(&l, expected, 3),  "duplicates maintain sorted order");

    list_free(&l);
}

static void test_remove_head(void) {
    printf("remove head:\n");
    LinkedList l;
    list_init(&l);

    list_insert(&l, 40);
    list_insert(&l, 15);
    list_insert(&l, 75);

    int removed = list_remove_head(&l);
    ASSERT(removed == 15, "remove_head returns minimum (15)");
    ASSERT(l.size == 2,   "size decremented after remove");

    int expected[] = {40, 75};
    ASSERT(verify_order(&l, expected, 2), "remaining list still sorted");

    list_free(&l);
}

static void test_remove_empty(void) {
    printf("remove from empty:\n");
    LinkedList l;
    list_init(&l);

    int v = list_remove_head(&l);
    ASSERT(v == -1,    "remove_head on empty returns -1");
    ASSERT(l.size == 0, "size stays 0");
}

static void test_remove_then_insert(void) {
    printf("remove then insert sequence:\n");
    LinkedList l;
    list_init(&l);

    list_insert(&l, 50);
    list_insert(&l, 20);
    list_insert(&l, 80);
    /* list: [20, 50, 80] */

    list_remove_head(&l); /* lose 20 — list: [50, 80] */
    list_insert(&l, 10);  /* list: [10, 50, 80] */
    list_insert(&l, 65);  /* list: [10, 50, 65, 80] */

    int expected[] = {10, 50, 65, 80};
    ASSERT(l.size == 4,                    "size == 4");
    ASSERT(verify_order(&l, expected, 4),  "order correct after remove+insert");
    ASSERT(l.head->value == 10,            "new minimum is head");

    list_free(&l);
}

static void test_remove_all(void) {
    printf("remove all items:\n");
    LinkedList l;
    list_init(&l);

    list_insert(&l, 3);
    list_insert(&l, 1);
    list_insert(&l, 2);

    list_remove_head(&l);
    list_remove_head(&l);
    list_remove_head(&l);

    ASSERT(l.size == 0,    "size == 0 after removing all");
    ASSERT(l.head == NULL, "head is NULL after removing all");

    /* Remove from now-empty list. */
    ASSERT(list_remove_head(&l) == -1, "remove on re-emptied list returns -1");
}

int main(void) {
    test_init();
    test_sorted_insert();
    test_duplicate_insert();
    test_remove_head();
    test_remove_empty();
    test_remove_then_insert();
    test_remove_all();

    printf("\n%s\n", failures == 0 ? "All tests passed." : "Some tests FAILED.");
    return failures > 0 ? 1 : 0;
}
