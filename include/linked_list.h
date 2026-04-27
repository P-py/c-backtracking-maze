/**
 * @file linked_list.h
 * @brief Sorted singly linked list used as the player's backpack.
 *
 * Always kept in ascending order so the head holds the lowest-value
 * treasure — the item sacrificed when the player steps on a trap.
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

/**
 * @brief Initialise @p l to the empty state.
 * @param l  List to initialise; must not be NULL.
 */
void list_init(LinkedList *l);

/**
 * @brief Insert @p value in ascending sorted order.
 *
 * Duplicate values are allowed; the new node is placed after all existing
 * nodes with the same value.
 *
 * @param l      Target list.
 * @param value  Coin value to insert (1–100).
 */
void list_insert(LinkedList *l, int value);

/**
 * @brief Remove and return the minimum-value node (the head).
 * @param l  Target list.
 * @return   The removed value, or -1 if the list is empty.
 */
int list_remove_head(LinkedList *l);

/**
 * @brief Remove the first occurrence of @p value from @p l.
 *
 * Used exclusively by BEST-mode undo to reverse a single treasure pickup.
 *
 * @param l      Target list.
 * @param value  Value to remove.
 * @return       1 if the value was found and removed, 0 otherwise.
 */
int list_remove_value(LinkedList *l, int value);

/**
 * @brief Print the backpack contents to stdout.
 * @param l  List to print; may be empty.
 */
void list_print(const LinkedList *l);

/**
 * @brief Free all nodes and reset @p l to the empty state.
 * @param l  List to free.
 */
void list_free(LinkedList *l);

#endif /* LINKED_LIST_H */
