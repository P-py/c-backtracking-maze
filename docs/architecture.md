# Architecture & Flow

Visual overview of how the modules interact and what each one is responsible for.

> For the search optimizations applied to `BACKTRACK_BEST` (reachability pre-computation and branch and bound), see [`optimizations.md`](optimizations.md).

## 1. Module Dependency Map

Who knows about whom at compile time. Arrows mean "depends on / includes".

```mermaid
graph TD
    main["<b>main.c</b><br/>Interactive menu + entry point<br/>Wires everything together"]

    maze["<b>maze/maze.c</b><br/>Loads file → flat grid array<br/>BFS reachability + treasure pre-assignment"]

    backtrack["<b>engine/backtrack.c</b><br/>DFS search — FIRST or BEST mode<br/>Reachability pruning + branch-and-bound"]

    renderer["<b>engine/renderer.c</b><br/>Live trail display + backpack<br/>Prints/writes final solution"]

    stack["<b>structures/stack.c</b><br/>Fixed-array int stack<br/>Tracks current path"]

    linked_list["<b>structures/linked_list.c</b><br/>Sorted linked list<br/>The backpack"]

    main       --> maze
    main       --> backtrack
    main       --> linked_list

    backtrack  --> maze
    backtrack  --> stack
    backtrack  --> linked_list
    backtrack  --> renderer

    renderer   --> maze
    renderer   --> linked_list
    renderer   --> stack
```

**Key rule:** data structures (`stack`, `linked_list`) and `maze` have zero internal dependencies — they are the foundation. `backtrack` is the only module that orchestrates all the others. `main` only sets up and tears down.

## 2. Ownership of Data

Each live object is owned by exactly one place.

```mermaid
graph LR
    main_owns["main()"]

    maze_obj["Maze*  (heap, via maze_load)\n─────────────────\ncells[]          raw grid chars\nvisited[]        runtime DFS state\nreachable[]      BFS pre-computed\ntreasure_values[] pre-assigned coins"]
    backpack_obj["LinkedList\n(stack-allocated in main)"]
    path_stack["Stack\n(stack-allocated in backtrack_run)"]

    main_owns -- "creates / frees" --> maze_obj
    main_owns -- "initialises"      --> backpack_obj

    backtrack_uses["backtrack_run()"]
    backtrack_uses -. "reads reachable[]\nmutates visited[]" .-> maze_obj
    backtrack_uses -. "push / pop"                          .-> path_stack
    backtrack_uses -. "insert / remove"                     .-> backpack_obj

    renderer_uses["renderer_draw()"]
    renderer_uses -. "reads only" .-> maze_obj
    renderer_uses -. "reads only" .-> backpack_obj
    renderer_uses -. "reads only" .-> path_stack
```

## 3. Runtime Sequence

Full flow from launch to program exit.

```mermaid
sequenceDiagram
    participant U  as user
    participant M  as main
    participant Mz as maze
    participant BT as backtrack
    participant St as stack
    participant LL as linked_list
    participant R  as renderer

    U  ->> M  : ./maze [filepath]
    M  ->> U  : prompt for file (if no argv[1])
    U  ->> M  : filepath
    M  ->> U  : show mode menu (1–6)
    U  ->> M  : choice → BacktrackMode + DisplayMode

    M  ->> Mz : maze_load(filepath)
    Note over Mz: parse COLSxROWS<br/>fill cells[], find P and S<br/>BFS from S → reachable[]<br/>rand values → treasure_values[]
    Mz -->> M : Maze* (or NULL → exit)

    M  ->> LL : list_init(&backpack)
    M  ->> BT : backtrack_run(maze, &backpack, mode, display)
    Note over BT: push player_pos, mark visited

    loop Each step of the search
        BT ->> R  : renderer_draw (skipped if DISPLAY_NONE)
        BT ->> Mz : maze_is_valid(neighbor) × 4 directions
        BT ->> Mz : reachable[neighbor]? (skip if 0)

        alt neighbor valid and reachable
            alt cell == 'T'
                BT ->> LL : list_insert(treasure_values[pos])
            else cell == 'A'
                BT ->> LL : list_remove_head()
            end
            BT ->> St : stack_push(neighbor)
            BT ->> Mz : mark neighbor visited
        else all directions exhausted
            BT ->> St : stack_pop()
            Note over BT: BEST: also unmarks visited,<br/>calls undo_event,<br/>checks B&B bound at next call
        end
    end

    alt Exit 'S' found
        BT ->> R  : renderer_print_solution(path, maze)
        BT ->> R  : renderer_write_solution(path, maze, backpack)
        BT -->> M : return 1
        M  ->> U  : print final summary
    else No solution
        BT -->> M : return 0
        M  ->> U  : print "no solution"
    end

    M ->> Mz : maze_free(maze)
    M ->> LL : list_free(&backpack)
```

## 4. Backtracking Modes

### BACKTRACK_FIRST — Iterative DFS

Stops at the first exit reached. Non-reachable neighbors are skipped via the `reachable[]` pre-computation.

```mermaid
flowchart TD
    START([Start]) --> INIT["Push player_pos onto stack\nMark player_pos visited"]
    INIT --> DRAW["renderer_draw (if not DISPLAY_NONE)"]
    DRAW --> INTER{DISPLAY_INTERACTIVE?}
    INTER -->|Yes| DESC["Print step description\nWait for Enter"]
    INTER -->|No| EXIT_CHECK
    DESC --> EXIT_CHECK

    EXIT_CHECK{Current cell == 'S'?}
    EXIT_CHECK -->|Yes| WRITE["renderer_print_solution\nrenderer_write_solution\nReturn 1 — SUCCESS"]
    WRITE --> END_OK([Done])

    EXIT_CHECK -->|No| TRY["Try next direction ↑ ↓ ← →"]
    TRY --> VALID{"valid?\nnot wall, not visited,\nin bounds, no wrap\nreachable[]?"}

    VALID -->|Yes| EVENT{Cell type?}
    EVENT -->|'T' treasure| TREASURE["list_insert(treasure_values[pos])\npush neighbor, mark visited"]
    EVENT -->|'A' trap| TRAP["list_remove_head()\npush neighbor, mark visited"]
    EVENT -->|' ' corridor| MOVE["push neighbor\nmark visited"]

    TREASURE --> DRAW
    TRAP      --> DRAW
    MOVE      --> DRAW

    VALID -->|No — tried all 4| DEAD_END["stack_pop — step back"]
    DEAD_END --> EMPTY{Stack empty?}
    EMPTY -->|Yes| NO_SOL(["Return 0 — NO SOLUTION"])
    EMPTY -->|No| DRAW
```

### BACKTRACK_BEST — Recursive DFS with Undo + Branch and Bound

Explores all paths. Pruned by reachability and by the upper-bound check `current_total + remaining_treasure <= best_total`. On backtrack, reverses cell events using a `CellEvent` array; `current_total` and `remaining_treasure` are passed by value so undo is implicit.

```mermaid
flowchart TD
    START(["explore(pos, remaining, total)"]) --> BNB{"best found AND\ntotal + remaining\n≤ best_total?"}
    BNB -->|Yes| PRUNE(["return — prune sub-tree"])
    BNB -->|No| DRAW["renderer_draw (if not DISPLAY_NONE)"]

    DRAW --> EXIT_CHECK{Current cell == 'S'?}
    EXIT_CHECK -->|Yes| COMPARE["If total > best.total:\nsave path + backpack snapshot"]
    COMPARE --> RETURN([return])

    EXIT_CHECK -->|No| LOOP["For each direction ↑ ↓ ← →"]
    LOOP --> VALID{"valid?\nnot wall, not visited,\nreachable[]?"}

    VALID -->|No| NEXT[Next direction]
    NEXT --> LOOP

    VALID -->|Yes| APPLY["apply_event → ev_type, ev_val\nmark visited, push\nupdate new_remaining, new_total"]
    APPLY --> RECURSE["explore(next, new_remaining, new_total)"]
    RECURSE --> UNDO["pop, unmark visited\nundo_event:\n  T → list_remove_value(ev_val)\n  A → list_insert(ev_val)"]
    UNDO --> NEXT

    LOOP -->|All 4 tried| DONE([return])
```

After all `explore` calls return to `run_best`: restore winning path into caller's stack and rebuild backpack from saved snapshot.

## 5. Data Structure Roles

### Stack — the path memory

```
push(3)  push(4)  push(9)  pop()   peek()
  [3]   [3,4]  [3,4,9] [3,4]    → 4

Stores 1D cell indices. Top = current position.
Pop = backtrack one step.
```

`renderer_draw` uses the full stack contents to mark trail cells (`.`) on the display.

### LinkedList — the backpack

```
insert(40)   insert(15)   insert(75)   remove_head()   insert(60)
  [40]       [15,40]    [15,40,75]       [40,75]       [40,60,75]
              ^sorted                    ^15 lost
                                         (trap hit)
```

Always sorted ascending so the **head is always the cheapest treasure** — the one sacrificed on a trap, minimising total loss.

- `list_insert`: O(n) walk to correct position
- `list_remove_head`: O(1) — always used by traps
- `list_remove_value`: O(n) — used by best-path undo to reverse a treasure pickup

## 6. Maze Memory Layout

The maze is stored as a **flat 1D array** of `char` (row-major order). The `Maze` struct carries four parallel arrays over the same index space:

```
cells[i]           — raw cell char from the file ('P', 'T', 'A', 'S', ' ', '#')
visited[i]         — 0/1, mutated during DFS, reset on backtrack (BEST mode)
reachable[i]       — 0/1, computed once at load by BFS from exit, never mutated
treasure_values[i] — pre-assigned coin value (1–100) for CELL_TREASURE cells, 0 otherwise
```

Index formula for a 4×5 grid (cols = 5):

```
Row 0: cells[0]  cells[1]  cells[2]  cells[3]  cells[4]
Row 1: cells[5]  cells[6]  cells[7]  cells[8]  cells[9]
...

index(row, col) = row * cols + col

Direction offsets:
  UP    = −cols   DOWN  = +cols   LEFT  = −1   RIGHT = +1
```

**Column-wrap guard:** before moving LEFT, check `current % cols == 0`; before RIGHT, check `current % cols == cols − 1`. Without this, the algorithm silently steps from the end of one row to the start of the next.
