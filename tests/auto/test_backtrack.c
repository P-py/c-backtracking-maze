#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <backtrack.h>
#include <renderer.h>
#include <linked_list.h>

static int failures = 0;

#define ASSERT(cond, msg) \
    do { \
        if (cond) { printf("  PASS: %s\n", msg); } \
        else      { printf("  FAIL: %s\n", msg); failures++; } \
    } while (0)

static Maze *make_maze(const char *grid, int cols, int rows) {
    int n   = rows * cols;
    Maze *m = malloc(sizeof(Maze));
    if (!m) { fprintf(stderr, "make_maze: out of memory (Maze)\n"); abort(); }

    m->cells           = NULL;
    m->visited         = NULL;
    m->reachable       = NULL;
    m->treasure_values = NULL;

    m->cells           = malloc(n);
    m->visited         = calloc(n, 1);
    m->reachable       = calloc(n, 1);
    m->treasure_values = calloc(n, sizeof(int));

    if (!m->cells || !m->visited || !m->reachable || !m->treasure_values) {
        free(m->cells);
        free(m->visited);
        free(m->reachable);
        free(m->treasure_values);
        free(m);
        fprintf(stderr, "make_maze: out of memory (arrays)\n");
        abort();
    }

    m->rows       = rows;
    m->cols       = cols;
    m->player_pos = -1;
    m->exit_pos   = -1;

    memset(m->cells, CELL_WALL, n);
    for (int i = 0; i < n; i++) {
        m->cells[i] = grid[i];
        if (grid[i] == CELL_PLAYER) m->player_pos = i;
        if (grid[i] == CELL_EXIT)   m->exit_pos   = i;
    }

    if (maze_compute_reachability(m) != 0) {
        maze_free(m);
        fprintf(stderr, "make_maze: maze_compute_reachability failed\n");
        abort();
    }
    maze_assign_treasures(m);
    return m;
}

static void test_simple_path(void) {
    printf("first: simple path (P _ S):\n");
    Maze *m = make_maze("#####"
                        "#P S#"
                        "#####", 5, 3);
    LinkedList backpack; list_init(&backpack);
    ASSERT(backtrack_run(m, &backpack, BACKTRACK_FIRST, DISPLAY_NONE) == 1, "returns 1");
    ASSERT(backpack.size == 0, "backpack empty");
    list_free(&backpack);
    maze_free(m);
}

static void test_no_solution(void) {
    printf("first: no solution (P # S):\n");
    Maze *m = make_maze("#####"
                        "#P#S#"
                        "#####", 5, 3);
    LinkedList backpack; list_init(&backpack);
    ASSERT(backtrack_run(m, &backpack, BACKTRACK_FIRST, DISPLAY_NONE) == 0, "returns 0");
    ASSERT(backpack.size == 0, "backpack unchanged");
    list_free(&backpack);
    maze_free(m);
}

static void test_treasure_collected(void) {
    printf("first: treasure collected (P T S):\n");
    Maze *m = make_maze("#####"
                        "#PTS#"
                        "#####", 5, 3);
    LinkedList backpack; list_init(&backpack);
    ASSERT(backtrack_run(m, &backpack, BACKTRACK_FIRST, DISPLAY_NONE) == 1, "returns 1");
    ASSERT(backpack.size == 1, "backpack has 1 treasure");
    ASSERT(backpack.head->value >= 1 && backpack.head->value <= 100,
           "treasure value in [1,100]");
    list_free(&backpack);
    maze_free(m);
}

static void test_trap_removes_lowest(void) {
    printf("first: trap removes lowest (P T T A S):\n");
    Maze *m = make_maze("#######"
                        "#PTTAS#"
                        "#######", 7, 3);
    LinkedList backpack; list_init(&backpack);
    ASSERT(backtrack_run(m, &backpack, BACKTRACK_FIRST, DISPLAY_NONE) == 1, "returns 1");
    ASSERT(backpack.size == 1, "one treasure remains after trap");
    list_free(&backpack);
    maze_free(m);
}

static void test_dead_end_backtrack(void) {
    printf("first: dead end forces backtrack (S unreachable):\n");
    Maze *m = make_maze("#####"
                        "#P  #"
                        "#####"
                        "# S #"
                        "#####", 5, 5);
    LinkedList backpack; list_init(&backpack);
    ASSERT(backtrack_run(m, &backpack, BACKTRACK_FIRST, DISPLAY_NONE) == 0, "returns 0");
    list_free(&backpack);
    maze_free(m);
}

static void test_best_finds_exit(void) {
    printf("best: finds exit:\n");
    Maze *m = make_maze("#####"
                        "#PTS#"
                        "#####", 5, 3);
    LinkedList backpack; list_init(&backpack);
    ASSERT(backtrack_run(m, &backpack, BACKTRACK_BEST, DISPLAY_NONE) == 1, "returns 1");
    ASSERT(backpack.size == 1, "backpack has 1 treasure");
    list_free(&backpack);
    maze_free(m);
}

static void test_best_no_solution(void) {
    printf("best: no solution:\n");
    Maze *m = make_maze("#####"
                        "#P#S#"
                        "#####", 5, 3);
    LinkedList backpack; list_init(&backpack);
    ASSERT(backtrack_run(m, &backpack, BACKTRACK_BEST, DISPLAY_NONE) == 0, "returns 0");
    list_free(&backpack);
    maze_free(m);
}

static void test_first_backtrack_no_ghost_treasure(void) {
    printf("first: backtracked treasure not counted:\n");
    /*
     * The only route to S forces a detour through a dead end containing T:
     *
     *   #######
     *   #P    #
     *   ### ###
     *   # T   #
     *   ### ###
     *   #     #
     *   #    S#
     *   #######
     *
     * DFS must enter T (dead end branch), backtrack, then reach S via a
     * corridor that has no treasure. The final backpack must be empty.
     */
    Maze *m = make_maze("#######"
                        "#P    #"
                        "### ###"
                        "# T   #"
                        "### ###"
                        "#     #"
                        "#    S#"
                        "#######", 7, 8);
    LinkedList backpack; list_init(&backpack);
    int found = backtrack_run(m, &backpack, BACKTRACK_FIRST, DISPLAY_NONE);
    ASSERT(found == 1,         "exit reached");
    ASSERT(backpack.size == 0, "backtracked treasure not in final backpack");
    list_free(&backpack);
    maze_free(m);
}

static void test_best_picks_richer_path(void) {
    printf("best: picks path with more treasure:\n");
    /*
     * Two routes to S:
     *   Top route:    P → (r1,2) → (r1,3) [T] → (r1,4) → (r2,4) → S
     *   Bottom route: P → (r3,1) → (r3,2) → (r3,3) → (r3,4) → (r2,4) → S
     *   (no T on bottom route)
     *
     * #######
     * #P  T #
     * #    S#
     * #     #
     * #######
     */
    Maze *m = make_maze("#######"
                        "#P  T #"
                        "#    S#"
                        "#     #"
                        "#######", 7, 5);
    LinkedList backpack; list_init(&backpack);
    int found = backtrack_run(m, &backpack, BACKTRACK_BEST, DISPLAY_NONE);
    ASSERT(found == 1,         "exit reached");
    ASSERT(backpack.size >= 1, "best path collected treasure");
    list_free(&backpack);
    maze_free(m);
}

int main(void) {
    renderer_set_delay(0);

    test_simple_path();
    test_no_solution();
    test_treasure_collected();
    test_trap_removes_lowest();
    test_dead_end_backtrack();
    test_first_backtrack_no_ghost_treasure();
    test_best_finds_exit();
    test_best_no_solution();
    test_best_picks_richer_path();

    printf("\n%s\n", failures == 0 ? "All tests passed." : "Some tests FAILED.");
    return failures > 0 ? 1 : 0;
}
