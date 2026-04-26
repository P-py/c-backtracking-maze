#include <stdio.h>
#include <linked_list.h>

int main(void) {
    LinkedList backpack;
    list_init(&backpack);

    printf("=== Backpack simulation ===\n\n");

    printf("Pick up treasure (40 coins):\n");
    list_insert(&backpack, 40);
    list_print(&backpack);

    printf("Pick up treasure (15 coins):\n");
    list_insert(&backpack, 15);
    list_print(&backpack);

    printf("Pick up treasure (75 coins):\n");
    list_insert(&backpack, 75);
    list_print(&backpack);

    printf("Trap triggered — lose lowest:\n");
    int lost = list_remove_head(&backpack);
    printf("  Lost: %d coins\n", lost);
    list_print(&backpack);

    printf("Pick up treasure (60 coins):\n");
    list_insert(&backpack, 60);
    list_print(&backpack);

    printf("Trap triggered — lose lowest:\n");
    lost = list_remove_head(&backpack);
    printf("  Lost: %d coins\n", lost);
    list_print(&backpack);

    printf("Trap triggered on empty backpack:\n");
    list_remove_head(&backpack);
    list_remove_head(&backpack);
    int result = list_remove_head(&backpack);
    printf("  remove_head on empty returned: %d\n", result);
    list_print(&backpack);

    list_free(&backpack);
    return 0;
}
