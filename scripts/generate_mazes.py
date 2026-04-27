#!/usr/bin/env python3
"""
Maze generator for the backtracking maze solver.

Generates .txt maze files covering every possible outcome the solver can encounter:
  1.  straight_path       — minimal P→S, no events
  2.  single_treasure     — one T on the only route
  3.  single_trap         — one A on the only route (backpack starts empty)
  4.  treasure_then_trap  — T then A (trap removes the collected treasure)
  5.  two_treasures_trap  — T T A (trap removes lowest; one treasure survives)
  6.  dead_end            — a dead-end branch + the real route (forces backtrack)
  7.  no_solution         — P and S in disconnected components
  8.  two_routes_treasure — two paths to S, one with more treasure (BEST mode)
  9.  multi_treasure_trap — several T and A on a longer corridor
  10. generated_medium    — 15x10 procedurally generated maze with random T/A
  11. generated_large     — 25x15 procedurally generated maze with random T/A

File format (matches the solver):
  Line 1: COLSxROWS
  Lines 2+: grid rows using  # (wall)  (space) (corridor)  P T A S
"""

import os
import random
import sys
from collections import deque

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "mazes", "test")


def write_maze(name: str, grid: list[str], cols: int, rows: int) -> None:
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    path = os.path.join(OUTPUT_DIR, f"{name}.txt")
    with open(path, "w") as f:
        for row in grid:
            f.write(row + "\n")
    print(f"  wrote {path}  ({cols}x{rows})")


def maze_straight_path():
    """Minimal 5×3 maze: P→→S, no events."""
    grid = [
        "#####",
        "#P S#",
        "#####",
    ]
    write_maze("01_straight_path", grid, 5, 3)


def maze_single_treasure():
    """One treasure on the only route: P T S."""
    grid = [
        "#####",
        "#PTS#",
        "#####",
    ]
    write_maze("02_single_treasure", grid, 5, 3)


def maze_single_trap():
    """Trap hit with empty backpack: P A S.
    The solver warns 'trap with empty backpack' and continues."""
    grid = [
        "#####",
        "#PAS#",
        "#####",
    ]
    write_maze("03_single_trap", grid, 5, 3)


def maze_treasure_then_trap():
    """Collect one treasure, hit a trap — backpack ends empty."""
    grid = [
        "######",
        "#PTAS#",
        "######",
    ]
    write_maze("04_treasure_then_trap", grid, 6, 3)


def maze_two_treasures_trap():
    """Two treasures, one trap: trap removes lowest; one treasure survives."""
    grid = [
        "#######",
        "#PTTAS#",
        "#######",
    ]
    write_maze("05_two_treasures_trap", grid, 7, 3)


def maze_dead_end():
    """
    Dead-end branch forces the solver to backtrack.

    Direction order is UP→DOWN→LEFT→RIGHT.  The column going UP from P is a
    dead-end (both UP cells are valid + reachable but have no forward exit).
    After exhausting that branch the solver backtracks to P and goes RIGHT to S.

    #####
    # # #
    # # #
    #P S#
    #####

    DFS trace (FIRST mode):
      P(3,1) → UP(2,1) → UP(1,1) → dead end, backtrack
      → (2,1) dead end, backtrack
      → P(3,1) → RIGHT(3,2) → RIGHT(3,3)=S  ✓
    """
    grid = [
        "#####",
        "# # #",
        "# # #",
        "#P S#",
        "#####",
    ]
    write_maze("06_dead_end", grid, 5, 5)


def maze_no_solution():
    """P and S separated by a wall — no path exists."""
    grid = [
        "#######",
        "#P #  #",
        "#  #S #",
        "#######",
    ]
    write_maze("07_no_solution", grid, 7, 4)


def maze_two_routes_treasure():
    """
    Two distinct routes to S; the top route has a treasure, the bottom does not.
    BEST mode must pick the top route; FIRST mode may take either.

    #######
    #P T  #
    #### S#
    #     #
    #######
    Top:    P→(1,2)→(1,3)[T]→(1,4)→(1,5)→(2,5)=S
    Bottom: P→(3,1)→(3,2)→(3,3)→(3,4)→(3,5)→(2,5)=S
    """
    grid = [
        "#######",
        "#P T  #",
        "#### S#",
        "#     #",
        "#######",
    ]
    write_maze("08_two_routes_treasure", grid, 7, 5)


def maze_multi_treasure_trap():
    """Longer corridor with three treasures and two traps interspersed."""
    grid = [
        "############",
        "#P T A T T A S#",
        "############",
    ]
    # Rebuild with correct width
    inner = "P T A T T A S"  # 13 chars
    row   = "#" + inner + "#"
    cols  = len(row)
    grid  = [
        "#" * cols,
        row,
        "#" * cols,
    ]
    write_maze("09_multi_treasure_trap", grid, cols, 3)

def _carve(grid: list[list[str]], r: int, c: int, rows: int, cols: int,
           rng: random.Random) -> None:
    """Recursive-backtracking maze carver (produces perfect mazes)."""
    directions = [(0, 2), (0, -2), (2, 0), (-2, 0)]
    rng.shuffle(directions)
    for dr, dc in directions:
        nr, nc = r + dr, c + dc
        if 1 <= nr < rows - 1 and 1 <= nc < cols - 1 and grid[nr][nc] == "#":
            grid[r + dr // 2][c + dc // 2] = " "
            grid[nr][nc] = " "
            _carve(grid, nr, nc, rows, cols, rng)


def _reachable_cells(grid: list[list[str]], rows: int, cols: int,
                     sr: int, sc: int) -> set[tuple[int, int]]:
    """BFS from (sr,sc) through non-wall cells."""
    visited = set()
    q = deque([(sr, sc)])
    visited.add((sr, sc))
    while q:
        r, c = q.popleft()
        for dr, dc in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
            nr, nc = r + dr, c + dc
            if 0 <= nr < rows and 0 <= nc < cols and (nr, nc) not in visited:
                if grid[nr][nc] != "#":
                    visited.add((nr, nc))
                    q.append((nr, nc))
    return visited


def _place_symbols(grid: list[list[str]], rows: int, cols: int,
                   n_treasures: int, n_traps: int,
                   rng: random.Random) -> bool:
    """
    Place P, S, T×n, A×n on open corridor cells.
    P and S are placed first, at least 30% of the grid apart.
    T and A are placed only on cells reachable from both P and S.
    Returns False if placement fails.
    """
    open_cells = [(r, c) for r in range(rows) for c in range(cols)
                  if grid[r][c] == " "]
    if len(open_cells) < 2 + n_treasures + n_traps:
        return False

    rng.shuffle(open_cells)

    # Place P and S far apart
    min_dist = (rows + cols) // 3
    player_pos = exit_pos = None
    for i, (pr, pc) in enumerate(open_cells):
        for er, ec in open_cells[i + 1:]:
            if abs(pr - er) + abs(pc - ec) >= min_dist:
                player_pos = (pr, pc)
                exit_pos   = (er, ec)
                break
        if player_pos:
            break
    if not player_pos:
        player_pos, exit_pos = open_cells[0], open_cells[-1]

    grid[player_pos[0]][player_pos[1]] = "P"
    grid[exit_pos[0]][exit_pos[1]]     = "S"

    # Only place T/A on cells reachable from both P and S
    from_p = _reachable_cells(grid, rows, cols, *player_pos)
    from_s = _reachable_cells(grid, rows, cols, *exit_pos)
    on_path = [cell for cell in (from_p & from_s)
               if grid[cell[0]][cell[1]] == " "]
    rng.shuffle(on_path)

    needed = n_treasures + n_traps
    if len(on_path) < needed:
        return False

    for i, (r, c) in enumerate(on_path[:needed]):
        grid[r][c] = "T" if i < n_treasures else "A"

    return True


def generate_maze(name: str, cols: int, rows: int,
                  n_treasures: int, n_traps: int, seed: int) -> None:
    """
    Generate a perfect maze (no loops) using recursive backtracking,
    then place P, S, T, A symbols.
    cols and rows must both be odd for the carver to work correctly.
    """
    assert cols % 2 == 1 and rows % 2 == 1, "cols and rows must be odd"
    rng = random.Random(seed)

    for attempt in range(20):
        grid: list[list[str]] = [["#"] * cols for _ in range(rows)]

        # Pick a random interior odd cell as carving start
        sr = rng.randrange(1, rows, 2)
        sc = rng.randrange(1, cols, 2)
        grid[sr][sc] = " "
        _carve(grid, sr, sc, rows, cols, rng)

        if _place_symbols(grid, rows, cols, n_treasures, n_traps, rng):
            write_maze(name, ["".join(row) for row in grid], cols, rows)
            return
        rng.seed(seed + attempt + 1)

    print(f"  WARNING: could not place all symbols for {name} after 20 attempts", file=sys.stderr)

# Varying sizes/densities cycled across the random slots.
_RANDOM_CONFIGS = [
    dict(cols=11, rows=11, n_treasures=2, n_traps=1),   # small
    dict(cols=15, rows=11, n_treasures=3, n_traps=2),   # medium-sparse
    dict(cols=21, rows=15, n_treasures=5, n_traps=2),   # medium-dense
    dict(cols=25, rows=19, n_treasures=6, n_traps=3),   # large
    dict(cols=31, rows=21, n_treasures=8, n_traps=4),   # extra-large
]

_N_RANDOM = 3   # how many random mazes to generate per run


def _generate_random_batch(n: int = _N_RANDOM) -> None:
    master_seed = random.randrange(2**32)
    rng = random.Random(master_seed)
    print(f"\n  [random batch seed={master_seed}]")
    for i in range(n):
        cfg  = _RANDOM_CONFIGS[i % len(_RANDOM_CONFIGS)]
        seed = rng.randrange(2**32)
        name = f"{13 + i:02d}_random_{cfg['cols']}x{cfg['rows']}"
        generate_maze(name, seed=seed, **cfg)

def main():
    print(f"Generating mazes → {os.path.abspath(OUTPUT_DIR)}/\n")

    # Hand-crafted scenarios
    maze_straight_path()
    maze_single_treasure()
    maze_single_trap()
    maze_treasure_then_trap()
    maze_two_treasures_trap()
    maze_dead_end()
    maze_no_solution()
    maze_two_routes_treasure()
    maze_multi_treasure_trap()

    # Procedurally generated mazes — fixed seeds, always the same output
    generate_maze("10_generated_medium", cols=15, rows=11,
                  n_treasures=2, n_traps=1, seed=42)
    generate_maze("11_generated_large",  cols=25, rows=15,
                  n_treasures=4, n_traps=2, seed=137)
    generate_maze("12_generated_dense",  cols=21, rows=21,
                  n_treasures=5, n_traps=3, seed=999)

    # Random mazes — different every run
    _generate_random_batch()

    print("\nDone. Run with:")
    print("  ./maze mazes/test/01_straight_path.txt")
    print("  ./maze mazes/test/08_two_routes_treasure.txt   # use BEST mode to verify")


if __name__ == "__main__":
    main()
