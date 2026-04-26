#include <stdio.h>
#include <stdlib.h>
#include <linked_list.h>

/** @brief Initialise @p l to the empty state (head = NULL, size = 0). */
void list_init(LinkedList *l) {
    l->head = NULL;
    l->size = 0;
}

/**
 * @brief Insert @p value in ascending sorted order.
 *
 * Maintains ascending sort so the head is always the cheapest item — the one
 * sacrificed when a trap fires, limiting total loss regardless of insert order.
 *
 * @param l      Target list.
 * @param value  Coin value to insert (1–100).
 */
void list_insert(LinkedList *l, int value) {
    Node *node = malloc(sizeof(Node));
    if (!node) {
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }
    node->value = value;
    node->next  = NULL;

    if (!l->head || value <= l->head->value) {
        node->next = l->head;
        l->head    = node;
        l->size++;
        return;
    }

    /* Stop before the first node whose value exceeds the new one. */
    Node *curr = l->head;
    while (curr->next && curr->next->value < value)
        curr = curr->next;

    node->next  = curr->next;
    curr->next  = node;
    l->size++;
}

/**
 * @brief Remove and return the minimum-value node (the head).
 *
 * O(1).  Returns -1 on an empty list, which callers treat as
 * "trap fired with nothing to lose".
 *
 * @param l  Non-empty list.
 * @return   The removed value, or -1 if the list is empty.
 */
int list_remove_head(LinkedList *l) {
    if (!l->head)
        return -1;

    Node *tmp   = l->head;
    int   value = tmp->value;
    l->head     = tmp->next;
    free(tmp);
    l->size--;
    return value;
}

/**
 * @brief Remove the first occurrence of @p value from @p l.
 *
 * O(n).  Used exclusively by BEST-mode undo to reverse a single treasure
 * pickup — removing exactly one node mirrors the one list_insert() that added it.
 *
 * @param l      Target list.
 * @param value  Value to remove.
 * @return       1 if the value was found and removed, 0 otherwise.
 */
int list_remove_value(LinkedList *l, int value) {
    if (!l->head) return 0;

    if (l->head->value == value) {
        list_remove_head(l);
        return 1;
    }

    Node *curr = l->head;
    while (curr->next) {
        if (curr->next->value == value) {
            Node *tmp  = curr->next;
            curr->next = tmp->next;
            free(tmp);
            l->size--;
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

/** @brief Print the backpack contents to stdout. */
void list_print(const LinkedList *l) {
    printf("Backpack (%d): [", l->size);
    for (Node *curr = l->head; curr; curr = curr->next) {
        printf("%d", curr->value);
        if (curr->next) printf(", ");
    }
    printf("]\n");
}

/** @brief Free all nodes and reset @p l to the empty state. */
void list_free(LinkedList *l) {
    Node *curr = l->head;
    while (curr) {
        Node *next = curr->next;
        free(curr);
        curr = next;
    }
    l->head = NULL;
    l->size = 0;
}
