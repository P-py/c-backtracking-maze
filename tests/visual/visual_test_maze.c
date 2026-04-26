#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <maze.h>
#include <backtrack.h>
#include <linked_list.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <maze_file> [first|best]\n", argv[0]);
        return 1;
    }

    BacktrackMode mode = BACKTRACK_FIRST;
    if (argc >= 3 && strcmp(argv[2], "best") == 0)
        mode = BACKTRACK_BEST;

    srand((unsigned int)time(NULL));

    Maze *m = maze_load(argv[1]);
    if (!m) return 1;

    printf("Mode : %s\n", mode == BACKTRACK_BEST ? "best-path (exhaustive)" : "first-path");
    printf("Maze : %s  (%d x %d)\n\n", argv[1], m->cols, m->rows);

    LinkedList backpack;
    list_init(&backpack);

    int found = backtrack_run(m, &backpack, mode);

    if (found) {
        int total = 0;
        for (Node *n = backpack.head; n; n = n->next) total += n->value;

        printf("\n=== EXIT REACHED ===\n");
        printf("Total treasure value: %d coins\n", total);
        printf("Backpack: ");
        list_print(&backpack);
    } else {
        printf("\n=== NO SOLUTION ===\n");
        printf("The maze has no path from P to S.\n");
    }

    list_free(&backpack);
    maze_free(m);
    return 0;
}
