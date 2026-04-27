#!/usr/bin/env python3
"""
Stress test for the dynamic maze solver.

Generates extreme-sized mazes well beyond the old 40×40 limit and runs each
through the solver in silent-first-path mode (menu option 5).

Maze types generated:
  - corridor_wide    : 1×N corridor (extreme column count)
  - corridor_tall    : N×1 corridor (extreme row count)
  - generated_NxM    : procedurally carved perfect maze (iterative carver)
"""

import os
import subprocess
import sys
import time
from collections import deque

SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.join(SCRIPT_DIR, "..")
SOLVER      = os.path.join(PROJECT_DIR, "maze")
OUT_DIR     = os.path.join(PROJECT_DIR, "mazes", "stress")


def write_maze(name: str, grid: list[str]) -> str:
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, f"{name}.txt")
    with open(path, "w") as f:
        for row in grid:
            f.write(row + "\n")
    rows = len(grid)
    cols = max(len(r) for r in grid)
    print(f"  wrote {path}  ({cols}x{rows})")
    return path


def corridor_wide(cols: int) -> str:
    """Single-row corridor: P followed by spaces then S, walled top and bottom."""
    inner = "P" + " " * (cols - 4) + "S"
    wall  = "#" * (cols - 2)
    grid  = [
        "#" + wall + "#",
        "#" + inner + "#",
        "#" + wall + "#",
    ]
    return write_maze(f"corridor_wide_{cols}", grid)


def corridor_tall(rows: int) -> str:
    """Single-column corridor: P at top, S at bottom."""
    grid = ["###"]
    grid.append("#P#")
    for _ in range(rows - 4):
        grid.append("# #")
    grid.append("#S#")
    grid.append("###")
    return write_maze(f"corridor_tall_{rows}", grid)


def _iterative_carve(rows: int, cols: int, seed: int) -> list[list[str]]:
    """
    Iterative recursive-backtracking carver (avoids Python's call-stack limit).
    Requires odd rows and cols.
    """
    import random
    rng = random.Random(seed)

    grid = [["#"] * cols for _ in range(rows)]

    # Pick an interior odd starting cell.
    sr = rng.randrange(1, rows - 1, 2)
    sc = rng.randrange(1, cols - 1, 2)
    grid[sr][sc] = " "

    stack = [(sr, sc)]
    while stack:
        r, c = stack[-1]
        dirs = [(0, 2), (0, -2), (2, 0), (-2, 0)]
        rng.shuffle(dirs)
        moved = False
        for dr, dc in dirs:
            nr, nc = r + dr, c + dc
            if 1 <= nr < rows - 1 and 1 <= nc < cols - 1 and grid[nr][nc] == "#":
                grid[r + dr // 2][c + dc // 2] = " "
                grid[nr][nc] = " "
                stack.append((nr, nc))
                moved = True
                break
        if not moved:
            stack.pop()

    return grid


def _place_ps(grid: list[list[str]], rows: int, cols: int, seed: int) -> bool:
    """Place P and S at opposite corners of the open passage, BFS-guaranteed reachable."""
    import random
    rng = random.Random(seed)

    open_cells = [(r, c) for r in range(rows) for c in range(cols) if grid[r][c] == " "]
    if len(open_cells) < 2:
        return False

    rng.shuffle(open_cells)

    # Place P at a random open cell, S at the open cell furthest from P (BFS).
    pr, pc = open_cells[0]
    grid[pr][pc] = "P"

    # BFS to find the cell farthest from P.
    dist   = {(pr, pc): 0}
    q      = deque([(pr, pc)])
    farthest = (pr, pc)
    while q:
        r, c = q.popleft()
        for dr, dc in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
            nr, nc = r + dr, c + dc
            if (nr, nc) not in dist and 0 <= nr < rows and 0 <= nc < cols:
                if grid[nr][nc] != "#":
                    dist[(nr, nc)] = dist[(r, c)] + 1
                    q.append((nr, nc))
                    if dist[(nr, nc)] > dist[farthest]:
                        farthest = (nr, nc)

    sr, sc = farthest
    grid[sr][sc] = "S"
    return True


def generated(cols: int, rows: int, seed: int = 42) -> str:
    """Generate a perfect maze of odd dimensions and place P/S."""
    # Snap to nearest odd values (carver requires odd dimensions).
    if cols % 2 == 0:
        cols += 1
    if rows % 2 == 0:
        rows += 1

    grid = _iterative_carve(rows, cols, seed)
    _place_ps(grid, rows, cols, seed)
    grid_str = ["".join(row) for row in grid]
    return write_maze(f"generated_{cols}x{rows}", grid_str)


def run_solver(maze_path: str, expected_cols: int, expected_rows: int) -> tuple[bool, float, str]:
    """
    Run the solver with option 5 (silent, first path).
    Returns (ok, seconds, output) where ok is False if:
      - the exit was not reached, OR
      - the solver's reported dimensions differ from expected (catches line-reader bugs).
    """
    import re
    t0 = time.perf_counter()
    result = subprocess.run(
        [SOLVER, maze_path],
        input="5\n",
        capture_output=True,
        text=True,
        timeout=60,
    )
    elapsed = time.perf_counter() - t0
    output  = result.stdout + result.stderr

    found = "EXIT REACHED" in result.stdout

    # Verify the dimensions the solver actually loaded match what we wrote.
    # The solver prints: "Maze    : <path>  (<cols> x <rows>)"
    dim_match = re.search(r"\((\d+) x (\d+)\)", result.stdout)
    dims_ok   = True
    dim_note  = ""
    if dim_match:
        actual_cols, actual_rows = int(dim_match.group(1)), int(dim_match.group(2))
        if actual_cols != expected_cols or actual_rows != expected_rows:
            dims_ok  = False
            dim_note = f" [DIM MISMATCH: got {actual_cols}x{actual_rows}, want {expected_cols}x{expected_rows}]"
    else:
        dims_ok  = False
        dim_note = " [no dimension line in output]"

    return found and dims_ok, elapsed, output + dim_note


def separator(title: str) -> None:
    print(f"\n{'─' * 60}")
    print(f"  {title}")
    print(f"{'─' * 60}")


def main() -> None:
    if not os.path.isfile(SOLVER):
        print(f"ERROR: solver not found at {SOLVER}. Run 'make' first.", file=sys.stderr)
        sys.exit(1)

    print("Stress-testing dynamic maze solver (mode: silent first-path)\n")

    # (label, maze_path, expected_cols, expected_rows)
    cases: list[tuple[str, str, int, int]] = []

    # ── Corridor tests ────────────────────────────────────────────────────────
    separator("Wide corridors (stress-test column count)")
    for cols in [100, 500, 1_000, 5_000, 10_000]:
        label = f"corridor_wide  {cols:>7} cols × 3 rows"
        path  = corridor_wide(cols)
        cases.append((label, path, cols, 3))

    separator("Tall corridors (stress-test row count)")
    for rows in [100, 500, 1_000, 5_000, 10_000]:
        label = f"corridor_tall  3 cols × {rows:>7} rows"
        path  = corridor_tall(rows)
        cases.append((label, path, 3, rows))

    # ── Procedurally generated perfect mazes ─────────────────────────────────
    separator("Procedurally generated perfect mazes")
    for cols, rows in [(101, 101), (201, 201), (501, 501), (1001, 1001)]:
        # generated() snaps to odd, so cols/rows stay the same here (already odd).
        label = f"generated      {cols:>5} cols × {rows:>5} rows"
        path  = generated(cols, rows, seed=42)
        cases.append((label, path, cols, rows))

    # ── Run all cases ─────────────────────────────────────────────────────────
    separator("Results")
    print(f"  {'Case':<45} {'Result':<10} {'Time':>8}")
    print(f"  {'─'*45} {'─'*10} {'─'*8}")

    all_ok = True
    for label, path, exp_cols, exp_rows in cases:
        try:
            ok, elapsed, output = run_solver(path, exp_cols, exp_rows)
            status = "FOUND" if ok else "FAIL"
            marker = "" if ok else " ← UNEXPECTED"
            print(f"  {label:<45} {status:<10} {elapsed:>7.2f}s{marker}")
            if not ok:
                all_ok = False
        except subprocess.TimeoutExpired:
            print(f"  {label:<45} {'TIMEOUT':<10} {'>60s':>8}")
            all_ok = False

    print()
    if all_ok:
        print("All stress tests passed.")
    else:
        print("Some stress tests FAILED or timed out.")
        sys.exit(1)


if __name__ == "__main__":
    main()
