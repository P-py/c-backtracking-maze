#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <renderer.h>
#include <defs.h>

static unsigned int step_delay_us = 40000; /* 40 ms default; 0 disables sleep */

/** @brief Set the per-step sleep duration; 0 disables sleeping. */
void renderer_set_delay(unsigned int delay_us) {
    step_delay_us = delay_us;
}

/** @brief Redraw the maze in-place using ANSI escape sequences; see renderer.h. */
void renderer_draw(const Maze *m, int current_pos,
                   const LinkedList *backpack, const Stack *path) {
    /* Build a flat lookup so the inner print loop can check membership in O(1)
     * instead of scanning the stack on every cell. Stack size ≤ MAX_CELLS so
     * this array never overflows. */
    char on_path[MAX_CELLS];
    memset(on_path, 0, sizeof(on_path));
    for (int i = 0; i <= path->top; i++)
        on_path[path->data[i]] = 1;

    /* \033[H = move cursor to top-left; \033[J = erase from cursor to end.
     * Together they redraw in-place without clearing the scrollback buffer. */
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

/** @brief Print the final solution grid to stdout, marking the path with '.'. */
void renderer_print_solution(const Stack *path, const Maze *m) {
    /* Only CELL_CORRIDOR cells get the trail marker. P, T, A, and S keep their
     * original characters so the solution grid shows what was on the path. */
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

/** @brief Write the solution grid and backpack summary to output/solution.txt. */
void renderer_write_solution(const Stack *path, const Maze *m, const LinkedList *backpack) {
    /* mkdir is called every time; if the directory already exists the call fails
     * silently with EEXIST, which is fine — we only need the directory to exist. */
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
