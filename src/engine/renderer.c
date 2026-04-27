#include <stdio.h>
#include <stdlib.h>
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

/* Reused across frames; grown when the maze gets larger, never shrunk. */
static char *on_path_buf = NULL;
static int   on_path_cap = 0;

/** @brief Redraw the maze in-place using ANSI escape sequences; see renderer.h. */
void renderer_draw(const Maze *m, int current_pos,
                   const LinkedList *backpack, const Stack *path) {
    int n = (*m).rows * (*m).cols;

    /* Grow the lookup buffer only when the maze is larger than any seen before.
     * The new region is zeroed here; previously used slots are zeroed after each
     * frame (see below), so the buffer is always clean on entry. */
    if (n > on_path_cap) {
        char *tmp = realloc(on_path_buf, n);
        if (!tmp) {
            fprintf(stderr, "malloc failed\n");
            return;
        }
        memset(tmp + on_path_cap, 0, n - on_path_cap);
        on_path_buf = tmp;
        on_path_cap = n;
    }

    /* Mark path cells — O(path length). */
    for (int i = 0; i <= (*path).top; i++)
        on_path_buf[(*path).data[i]] = 1;

    /* \033[H = move cursor to top-left; \033[J = erase from cursor to end.
     * Together they redraw in-place without clearing the scrollback buffer. */
    printf("\033[H\033[J");

    for (int r = 0; r < (*m).rows; r++) {
        for (int c = 0; c < (*m).cols; c++) {
            int  idx  = r * (*m).cols + c;
            char cell = (*m).cells[idx];

            if (idx == current_pos) {
                putchar('@');                   /* player        */
            } else if (on_path_buf[idx] && cell == CELL_CORRIDOR) {
                putchar('.');                   /* trail mark    */
            } else {
                putchar(cell);                  /* original cell */
            }
        }
        putchar('\n');
    }

    /* Clear only the marked slots — O(path length), not O(n). */
    for (int i = 0; i <= (*path).top; i++)
        on_path_buf[(*path).data[i]] = 0;

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
    int   total   = (*m).rows * (*m).cols;
    char *display = malloc(total);
    if (!display) {
        fprintf(stderr, "malloc failed\n");
        return;
    }
    memcpy(display, (*m).cells, total);

    for (int i = 0; i <= (*path).top; i++) {
        int  pos  = (*path).data[i];
        char cell = (*m).cells[pos];
        if (cell == CELL_CORRIDOR)
            display[pos] = '.';
    }

    printf("\n=== SOLUTION ===\n");
    for (int r = 0; r < (*m).rows; r++) {
        for (int c = 0; c < (*m).cols; c++)
            putchar(display[r * (*m).cols + c]);
        putchar('\n');
    }

    free(display);
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

    int   n       = (*m).rows * (*m).cols;
    char *display = malloc(n);
    if (!display) {
        fprintf(stderr, "malloc failed\n");
        fclose(f);
        return;
    }
    memcpy(display, (*m).cells, n);

    for (int i = 0; i <= (*path).top; i++) {
        int pos = (*path).data[i];
        if ((*m).cells[pos] == CELL_CORRIDOR)
            display[pos] = '.';
    }

    fprintf(f, "=== SOLUTION ===\n");
    for (int r = 0; r < (*m).rows; r++) {
        for (int c = 0; c < (*m).cols; c++)
            fputc(display[r * (*m).cols + c], f);
        fputc('\n', f);
    }
    free(display);

    int total = 0;
    for (const Node *n = (*backpack).head; n; n = (*n).next) total += (*n).value;
    fprintf(f, "\nTotal treasure value: %d coins\n", total);
    fprintf(f, "Backpack (%d): [", (*backpack).size);
    for (const Node *n = (*backpack).head; n; n = (*n).next) {
        fprintf(f, "%d", (*n).value);
        if ((*n).next) fprintf(f, ", ");
    }
    fprintf(f, "]\n");

    fclose(f);
    printf("Solution written to output/solution.txt\n");
}
