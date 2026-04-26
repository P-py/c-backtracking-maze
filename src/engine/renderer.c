#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <renderer.h>
#include <defs.h>

static unsigned int step_delay_us = 40000; /* 40 ms default */

void renderer_set_delay(unsigned int delay_us) {
    step_delay_us = delay_us;
}

void renderer_draw(const Maze *m, int current_pos,
                   const LinkedList *backpack, const Stack *path) {
    /* Build a quick lookup of which cells are on the current path. */
    char on_path[MAX_CELLS];
    memset(on_path, 0, sizeof(on_path));
    for (int i = 0; i <= path->top; i++)
        on_path[path->data[i]] = 1;

    /* Clear terminal */
    printf("\033[H\033[J");

    for (int r = 0; r < m->rows; r++) {
        for (int c = 0; c < m->cols; c++) {
            int  idx  = r * m->cols + c;
            char cell = m->cells[idx];

            if (idx == current_pos) {
                putchar('@');                   /* player        */
            } else if (on_path[idx] && cell == CELL_CORRIDOR) {
                putchar('.');                   /* trail mark    */
            } else {
                putchar(cell);                  /* original cell */
            }
        }
        putchar('\n');
    }

    printf("\n");
    list_print(backpack);
    fflush(stdout);

    if (step_delay_us > 0)
        usleep(step_delay_us);
}

void renderer_print_solution(const Stack *path, const Maze *m) {
    /* Build display grid: '.' on corridor path cells, originals elsewhere. */
    char display[MAX_CELLS];
    int  total = m->rows * m->cols;
    memcpy(display, m->cells, total);

    for (int i = 0; i <= path->top; i++) {
        int  pos  = path->data[i];
        char cell = m->cells[pos];
        if (cell == CELL_CORRIDOR)
            display[pos] = '.';
    }

    printf("\n=== SOLUTION ===\n");
    for (int r = 0; r < m->rows; r++) {
        for (int c = 0; c < m->cols; c++)
            putchar(display[r * m->cols + c]);
        putchar('\n');
    }
}

void renderer_write_solution(const Stack *path, const Maze *m, const LinkedList *backpack) {
    mkdir("output", 0755);

    FILE *f = fopen("output/solution.txt", "w");
    if (!f) {
        fprintf(stderr, "Error: cannot write output/solution.txt\n");
        return;
    }

    char display[MAX_CELLS];
    memcpy(display, m->cells, m->rows * m->cols);
    for (int i = 0; i <= path->top; i++) {
        int pos = path->data[i];
        if (m->cells[pos] == CELL_CORRIDOR)
            display[pos] = '.';
    }

    fprintf(f, "=== SOLUTION ===\n");
    for (int r = 0; r < m->rows; r++) {
        for (int c = 0; c < m->cols; c++)
            fputc(display[r * m->cols + c], f);
        fputc('\n', f);
    }

    int total = 0;
    for (const Node *n = backpack->head; n; n = n->next) total += n->value;
    fprintf(f, "\nTotal treasure value: %d coins\n", total);
    fprintf(f, "Backpack (%d): [", backpack->size);
    for (const Node *n = backpack->head; n; n = n->next) {
        fprintf(f, "%d", n->value);
        if (n->next) fprintf(f, ", ");
    }
    fprintf(f, "]\n");

    fclose(f);
    printf("Solution written to output/solution.txt\n");
}
