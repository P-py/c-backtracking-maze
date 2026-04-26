/**
 * @file linked_list.h
 * @brief Sorted singly linked list used as the player's backpack.
 *
 * Always kept in ascending order so the head holds the lowest-value
 * treasure — the item sacrificed when the player steps on a trap.
 * list_remove_head() must only be called on a non-empty list.
 */

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>

typedef struct Node Node;
struct Node {
    int value; /**< Treasure value (1–100 coins). */
    Node *next;
};

typedef struct {
    Node *head; /**< First node (lowest value), or NULL when empty. */
    int size; /**< Current number of nodes. */
} LinkedList;

void list_init        (LinkedList *l);
void list_insert      (LinkedList *l, int value);
int  list_remove_head (LinkedList *l);
int  list_remove_value(LinkedList *l, int value); /* removes first occurrence; returns 1 if found */
void list_print       (const LinkedList *l);
void list_free        (LinkedList *l);

#endif /* LINKED_LIST_H */
