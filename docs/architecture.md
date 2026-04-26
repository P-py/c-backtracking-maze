# Architecture & Flow

Visual overview of how the modules interact and what each one is responsible for.

> For the search optimizations applied to `BACKTRACK_BEST` (reachability pre-computation and branch and bound), see [`optimizations.md`](optimizations.md).

## 1. Module Dependency Map

Who knows about whom at compile time. Arrows mean "depends on / includes".

```mermaid
graph TD
    main["<b>main.c</b><br/>Entry point<br/>Wires everything together"]

    maze["<b>maze/maze.c</b><br/>Loads file → flat grid array<br/>Validates cells, marks visited"]

    backtrack["<b>engine/backtrack.c</b><br/>DFS search — FIRST or BEST mode<br/>Drives all cell events + undo"]

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

    maze_obj["Maze*<br/>(heap, via maze_load)"]
    backpack_obj["LinkedList<br/>(stack-allocated in main)"]
    path_stack["Stack<br/>(stack-allocated in backtrack_run)"]

    main_owns -- "creates / frees" --> maze_obj
    main_owns -- "initialises"      --> backpack_obj

    backtrack_uses["backtrack_run()"]
    backtrack_uses -. "reads & mutates" .-> maze_obj
    backtrack_uses -. "push / pop"      .-> path_stack
    backtrack_uses -. "insert / remove" .-> backpack_obj

    renderer_uses["renderer_draw()"]
    renderer_uses -. "reads only"       .-> maze_obj
    renderer_uses -. "reads only"       .-> backpack_obj
    renderer_uses -. "reads only"       .-> path_stack
```

## 3. Runtime Sequence

What happens from `./maze mazes/maze_10x10.txt first` to program exit.

```mermaid
sequenceDiagram
    participant M  as main
    participant Mz as maze
    participant BT as backtrack
    participant St as stack
    participant LL as linked_list
    participant R  as renderer

    M  ->> Mz : maze_load(argv[1])
    Mz -->> M : Maze* (or NULL → exit)

    M  ->> LL : list_init(&backpack)
    M  ->> BT : backtrack_run(maze, &backpack, mode)

    Note over BT: push player_pos, mark visited

    loop Each step of the search
        BT ->> R  : renderer_draw(maze, current, backpack, path)
        BT ->> Mz : maze_is_valid(neighbor) × 4 directions

        alt neighbor is valid
            alt cell == 'T'
                BT ->> LL : list_insert(rand 1–100)
            else cell == 'A'
                BT ->> LL : list_remove_head()
            end
            BT ->> St : stack_push(neighbor)
            BT ->> Mz : mark neighbor visited
        else all directions exhausted (dead end)
            BT ->> St : stack_pop()
            Note over BT: BEST mode also unmarks visited<br/>and calls undo_event
        end
    end

    alt Exit 'S' found
        BT ->> R  : renderer_print_solution(path, maze)
        BT ->> R  : renderer_write_solution(path, maze)
        BT -->> M : return 1
        M  ->> M  : print final summary
    else Stack empty — no solution
        BT -->> M : return 0
        M  ->> M  : print "no solution"
    end

    M ->> Mz : maze_free(maze)
    M ->> LL : list_free(&backpack)
```

## 4. Backtracking Modes

### BACKTRACK_FIRST — Iterative DFS

```mermaid
flowchart TD
    START([Start]) --> INIT["Push player_pos onto stack\nMark player_pos visited"]
    INIT --> DRAW["renderer_draw — show current state"]
    DRAW --> EXIT_CHECK{Current cell == 'S'?}

    EXIT_CHECK -->|Yes| WRITE["renderer_print_solution\nrenderer_write_solution\nReturn 1 — SUCCESS"]
    WRITE --> END_OK([Done])

    EXIT_CHECK -->|No| TRY["Try next direction ↑ ↓ ← →"]
    TRY --> VALID{Neighbor valid?\nnot wall, not visited,\nin bounds, no wrap}

    VALID -->|Yes| EVENT{Cell type?}
    EVENT -->|'T' treasure| TREASURE["list_insert(rand value)\npush neighbor, mark visited"]
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

### BACKTRACK_BEST — Recursive DFS with Undo

Explores all paths. At each exit, compares total backpack value against the stored best. On backtrack, reverses cell events using a `CellEvent` array.

```mermaid
flowchart TD
    START([explore called]) --> DRAW["renderer_draw"]
    DRAW --> EXIT_CHECK{Current cell == 'S'?}

    EXIT_CHECK -->|Yes| COMPARE["Sum backpack\nIf total > best.total: save path + backpack"]
    COMPARE --> RETURN([return — keep exploring other paths])

    EXIT_CHECK -->|No| LOOP["For each direction ↑ ↓ ← →"]
    LOOP --> VALID{Neighbor valid?}

    VALID -->|No| NEXT[Next direction]
    NEXT --> LOOP

    VALID -->|Yes| APPLY["apply_event → record in events[neighbor]\nmark visited, push"]
    APPLY --> RECURSE["explore(neighbor) — recursive"]
    RECURSE --> UNDO["pop, unmark visited\nundo_event:\n  T → list_remove_value(value)\n  A → list_insert(value)"]
    UNDO --> NEXT

    LOOP -->|All 4 tried| DONE([return])
```

After `explore` returns to `run_best`: restore winning path into caller's stack, rebuild backpack from saved values.

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

The maze is stored as a **flat 1D array** of `char` (row-major order). A 4×5 grid:

```
Row 0: cells[0]  cells[1]  cells[2]  cells[3]  cells[4]
Row 1: cells[5]  cells[6]  cells[7]  cells[8]  cells[9]
Row 2: cells[10] cells[11] cells[12] cells[13] cells[14]
Row 3: cells[15] cells[16] cells[17] cells[18] cells[19]

index(row, col) = row * cols + col

Direction offsets (cols = 5):
  UP    = -5   (index - cols)
  DOWN  = +5   (index + cols)
  LEFT  = -1
  RIGHT = +1
```

**Column-wrap guard:** before moving LEFT, check `current % cols == 0`; before RIGHT, check `current % cols == cols - 1`. Without this, the algorithm would silently step from the end of one row to the start of the next.
