#include "structures/linked_list.h"

void list_init(LinkedList *l) {
    l->head = NULL;
    l->size = 0;
}

void list_insert(LinkedList *l, int value) {
    (void)l;
    (void)value;
}

int list_remove_head(LinkedList *l) {
    (void)l;
    return -1;
}

void list_print(const LinkedList *l) {
    (void)l;
}

void list_free(LinkedList *l) {
    (void)l;
}
