#include <stdio.h>
#include <stdlib.h>
#include <linked_list.h>

void list_init(LinkedList *l) {
    l->head = NULL;
    l->size = 0;
}

void list_insert(LinkedList *l, int value) {
    Node *node = malloc(sizeof(Node));
    if (!node) {
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }
    node->value = value;
    node->next  = NULL;

    /* Insert at head if list is empty or value is <= current minimum. */
    if (!l->head || value <= l->head->value) {
        node->next = l->head;
        l->head    = node;
        l->size++;
        return;
    }

    /* Walk until the next node's value exceeds the new value. */
    Node *curr = l->head;
    while (curr->next && curr->next->value < value)
        curr = curr->next;

    node->next  = curr->next;
    curr->next  = node;
    l->size++;
}

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

int list_remove_value(LinkedList *l, int value) {
    if (!l->head) return 0;

    /* Check head */
    if (l->head->value == value) {
        list_remove_head(l);
        return 1;
    }

    /* Search rest of list */
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

void list_print(const LinkedList *l) {
    printf("Backpack (%d): [", l->size);
    for (Node *curr = l->head; curr; curr = curr->next) {
        printf("%d", curr->value);
        if (curr->next) printf(", ");
    }
    printf("]\n");
}

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
