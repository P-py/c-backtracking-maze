#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <maze.h>
#include <backtrack.h>
#include <linked_list.h>

static void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static int read_filepath(char *buf, int bufsize) {
    printf("Maze file path: ");
    fflush(stdout);
    if (!fgets(buf, bufsize, stdin)) return 0;
    buf[strcspn(buf, "\n")] = '\0';
    return buf[0] != '\0';
}

static void print_menu(void) {
    printf("\nExecution mode:\n");
    printf("  1. Interactive     —  First path\n");
    printf("  2. Interactive     —  Best path\n");
    printf("  3. Auto (display)  —  First path\n");
    printf("  4. Auto (display)  —  Best path\n");
    printf("  5. Auto (silent)   —  First path\n");
    printf("  6. Auto (silent)   —  Best path\n\n");
    printf("Select [1-6]: ");
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    printf("=== MAZE SOLVER ===\n\n");

    char filepath[256];

    if (argc >= 2) {
        strncpy(filepath, argv[1], sizeof(filepath) - 1);
        filepath[sizeof(filepath) - 1] = '\0';
    } else {
        if (!read_filepath(filepath, sizeof(filepath))) {
            fprintf(stderr, "Error: no maze file provided.\n");
            return 1;
        }
    }

    print_menu();

    int choice = 0;
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > 6) {
        fprintf(stderr, "Invalid choice.\n");
        return 1;
    }
    flush_stdin();

    BacktrackMode mode;
    DisplayMode   display;

    switch (choice) {
        case 1: mode = BACKTRACK_FIRST; display = DISPLAY_INTERACTIVE; break;
        case 2: mode = BACKTRACK_BEST;  display = DISPLAY_INTERACTIVE; break;
        case 3: mode = BACKTRACK_FIRST; display = DISPLAY_AUTO;        break;
        case 4: mode = BACKTRACK_BEST;  display = DISPLAY_AUTO;        break;
        case 5: mode = BACKTRACK_FIRST; display = DISPLAY_NONE;        break;
        case 6: mode = BACKTRACK_BEST;  display = DISPLAY_NONE;        break;
        default: return 1;
    }

    srand((unsigned int)time(NULL));

    Maze *m = maze_load(filepath);
    if (!m) return 1;

    const char *mode_str    = (mode    == BACKTRACK_BEST)  ? "Best path"       : "First path";
    const char *display_str = (display == DISPLAY_AUTO)    ? "Auto (display)"  :
                              (display == DISPLAY_INTERACTIVE) ? "Interactive"  : "Auto (silent)";

    printf("\nMode    : %s\n", mode_str);
    printf("Display : %s\n", display_str);
    printf("Maze    : %s  (%d x %d)\n\n", filepath, m->cols, m->rows);

    LinkedList backpack;
    list_init(&backpack);

    int found = backtrack_run(m, &backpack, mode, display);

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
