# 🗺️ C Backtracking Maze

## Overview

This project implements the logic engine of an **archaeological exploration game** written in **C** for the "Data Structures and Algorithms" class @ FACENS. A treasure hunter must navigate an ancient and dangerous maze to find the exit — collecting treasures and avoiding traps along the way. The goal is not just to escape, but to **maximize the total value of treasures collected**.


## Gameplay Rules

### The Maze
- Represented as a **1D array** simulating a matrix of up to **40×40 cells**.
- Layout is **loaded from a `.txt` file**. The first line of the file specifies the maze dimensions (e.g., `10X10`).

| Symbol | Meaning        |
|--------|----------------|
| `#`    | Wall           |
| ` `    | Corridor (path)|
| `P`    | Player (start) |
| `T`    | Treasure       |
| `A`    | Trap           |
| `S`    | Exit           |

### Movement & Search
- The software must implement a **Backtracking** search algorithm to navigate through crossroads and dead ends until the exit is found.

### The Backpack (Mochila)
- Every treasure found is stored in the **backpack**.
- Each treasure has a **randomly generated value between 1 and 100 coins**, pre-assigned at maze load time.
- **Trap rule:** When the player steps on a trap (`A`), the treasure in the **first position** of the backpack is lost.
- **Maximization strategy:** To minimize losses, the backpack must always keep the **lowest-value treasure at the first position** (i.e., it must be kept sorted in ascending order).

### Visual Interface
- The maze is rendered **step by step** in the terminal using **ASCII characters**.
- Each step is displayed with a **small delay**.
- The **current backpack contents** are printed alongside or below the maze at every step.


## Technical Requirements

| Requirement         | Detail                                              |
|---------------------|-----------------------------------------------------|
| Language            | C                                                   |
| Data Structures     | Stacks, Linked Lists, and a Sorting Algorithm       |
| Input               | `.txt` file with maze layout (dimensions inferred)  |
| Output (terminal)   | Step-by-step ASCII visualization + final total value|
| Output (file)       | `.txt` file recording the final solution path       |


## Project Structure

```
/
├── include/                        # All public headers (angle-bracket includes via -Iinclude)
│   ├── defs.h                      # Shared constants (cell symbols, MAX_CELLS)
│   ├── maze.h
│   ├── backtrack.h
│   ├── renderer.h
│   ├── stack.h
│   └── linked_list.h
│
├── src/
│   ├── main.c                      # Entry point — parses args, shows menu, runs solver
│   ├── maze/
│   │   └── maze.c                  # Maze loading, BFS reachability, treasure pre-assignment
│   ├── engine/
│   │   ├── backtrack.c             # DFS search — FIRST and BEST modes
│   │   └── renderer.c              # ASCII rendering and solution file output
│   └── structures/
│       ├── stack.c                 # Fixed-capacity int stack (tracks the current path)
│       └── linked_list.c           # Sorted linked list (the backpack)
│
├── tests/
│   ├── auto/
│   │   ├── test_stack.c            # Automated unit tests for the stack
│   │   ├── test_linked_list.c      # Automated unit tests for the linked list
│   │   └── test_backtrack.c        # Automated tests for the search algorithm
│   └── visual/
│       ├── visual_test_maze.c      # Renders a maze traversal for visual inspection
│       └── visual_test_backpack.c  # Simulates treasure/trap events, prints backpack state
│
├── mazes/
│   ├── maze_10x10.txt              # Named mazes for running the solver
│   ├── maze_20x15.txt
│   ├── maze_30x10.txt
│   ├── maze_40x40.txt
│   └── test/                       # Targeted mazes used by the automated test suite
│
├── docs/
│   ├── architecture.md             # Module map, data ownership, runtime sequence diagrams
│   ├── optimizations.md            # Reachability pre-computation and branch-and-bound analysis
│   ├── c-patterns.md               # Coding conventions used in this project
│   └── makefile-patterns.md        # Makefile structure and build patterns
│
├── scripts/
│   └── generate_mazes.py           # Script to generate random maze .txt files
│
├── output/
│   └── solution.txt                # Final solution path (generated at runtime)
│
└── Makefile
```


## Input File Format

No header line is required. Dimensions are inferred at load time: `cols` = length of the widest line, `rows` = number of non-blank lines. Every cell must be one of the symbols in the table above; short lines are padded with `#`.

**Example (`maze_10x10.txt`):**
```
##########
#P  T    #
# ###### #
# #      #
# # #### #
# # #A   #
# # # ####
# # #    #
#   #   S#
##########
```

## Output

### Terminal
At every step of the traversal, the program prints:
- The current state of the maze (with the player's position marked).
- The contents of the backpack (list of treasure values).

At the end:
```
=== EXIT REACHED ===
Total treasure value: 230 coins
Backpack: [15, 40, 75, 100]
```

### Solution File (`output/solution.txt`)
Records every cell visited on the **correct path** from `P` to `S`, in order.


## Algorithm Design

### Backtracking
1. Start at position `P`.
2. Try moving in each direction (Up, Down, Left, Right).
3. Mark visited cells to avoid loops.
4. If a dead end is reached, **backtrack** using the stack.
5. **First-path mode:** stop as soon as `S` is reached.  
   **Best-path mode:** continue exploring all paths even after reaching `S`; keep the one with the highest total treasure value.

### Backpack Sorting Strategy
- Data structure: **sorted linked list** (ascending order by value).
- On treasure pickup: insert in the correct position to maintain order.
- On trap: remove the **head** of the list (lowest value).
- This guarantees minimal loss when a trap is triggered.

## Error Handling

The program must handle the following cases gracefully:

- Maze file not found → print error message and exit.
- No valid path from `P` to `S` → inform the user that the maze has no solution.
- Trap triggered with an empty backpack → skip item removal and display a warning.
- Maze dimensions exceeding 40×40 → reject the file with an appropriate message.

## Grading Criteria

| Criterion                                          | Weight |
|----------------------------------------------------|--------|
| Correct use of pointers and data structures        | ✓      |
| Efficiency of the maze search algorithm            | ✓      |
| Error handling                                     | ✓      |
| Code quality (indentation, comments, modularity)   | ✓      |
| Step-by-step ASCII visualization                   | ✓      |
| Oral presentation & team knowledge demonstration   | ✓      |

## How to Build & Run

### 1. Build

```bash
make
```

Compiles all sources and produces the `./maze` executable.

### 2. Run

```bash
./maze mazes/maze_10x10.txt    # pass the maze file directly
./maze                          # omit it to be prompted
```

### 3. Choose an execution mode

After the maze loads, an interactive menu is displayed:

```
Execution mode:
  1. Interactive     —  First path
  2. Interactive     —  Best path
  3. Auto (display)  —  First path
  4. Auto (display)  —  Best path
  5. Auto (silent)   —  First path
  6. Auto (silent)   —  Best path
```

| # | Display style | Path mode |
|---|---|---|
| 1 | Step-by-step (press Enter to advance) | First path found |
| 2 | Step-by-step | Highest-value path |
| 3 | Animated (40 ms/step) | First path found |
| 4 | Animated | Highest-value path |
| 5 | Silent (final result only) | First path found |
| 6 | Silent | Highest-value path |

### 4. Read the result

The solution is printed to the terminal and written to `output/solution.txt`.

### Other commands

```bash
make test         # Run all automated unit tests
make test-visual  # Run visual test renderings
make clean        # Remove build artifacts
```

