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
- Each treasure has a **randomly generated value between 1 and 100 coins**, assigned when the player steps on it.
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
| Input               | `.txt` file with maze layout and dimensions         |
| Output (terminal)   | Step-by-step ASCII visualization + final total value|
| Output (file)       | `.txt` file recording the final solution path       |


## Project Structure

```
/
├── src/
│   ├── main.c                      # Entry point — initializes and runs the game
│   │
│   ├── maze/
│   │   ├── maze.c                  # Maze loading from file and cell logic
│   │   └── maze.h
│   │
│   ├── engine/
│   │   ├── backtrack.c             # Backtracking search algorithm
│   │   ├── backtrack.h
│   │   ├── renderer.c              # ASCII step-by-step rendering and delay
│   │   └── renderer.h
│   │
│   └── structures/
│       ├── stack.c                 # Stack (used by the backtracking algorithm)
│       ├── stack.h
│       ├── linked_list.c           # Sorted linked list (backpack implementation)
│       └── linked_list.h
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
│   ├── maze_10x10.txt
│   ├── maze_20x15.txt
│   ├── maze_30x10.txt
│   └── maze_40x40.txt
│
├── output/
│   └── solution.txt                # Final solution path (generated at runtime)
│
├── Makefile                        # Root — builds the main executable
└── README.md
```


## Input File Format

The first line of the maze file must contain the dimensions in the format `COLSxROWS`. Each subsequent line represents a row of the maze.

**Example (`maze_10x10.txt`):**
```
10x10
##########
#P  T    #
# ###### #
# #      #
# # #### #
# # #A   #
# # # ####
#   #    S
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
5. Continue until `S` is found or all paths are exhausted.

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

```bash
# Build the main executable
make

# Run with a maze file
./maze mazes/maze_10x10.txt

# Run all automated tests
make test

# Run visual tests
make test-visual

# Clean build artifacts
make clean
```

