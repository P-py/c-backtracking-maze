# Makefile Patterns

How the project's Makefile is structured and why.

## Variables

```makefile
CC      = gcc
CFLAGS  = -Wall -Wextra -g -Isrc

SRC_DIR   = src
BUILD_DIR = build
```

`-Isrc` lets every file use `#include "maze/maze.h"` instead of relative `../` paths.  
`-g` keeps debug symbols; strip it for a release build.

## Directory Layout (build artifacts)

All `.o` files land in `build/` so the source tree stays clean:
```makefile
OBJ = $(BUILD_DIR)/main.o \
      $(BUILD_DIR)/maze.o \
      $(BUILD_DIR)/stack.o \
      $(BUILD_DIR)/linked_list.o \
      $(BUILD_DIR)/backtrack.o \
      $(BUILD_DIR)/renderer.o
```

## Pattern Rule

Compile any `.c` → `.o` with one rule:
```makefile
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Also needed for nested source paths:
$(BUILD_DIR)/%.o: src/maze/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/engine/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/structures/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
```

`| $(BUILD_DIR)` is an order-only prerequisite — it creates the directory before compiling but doesn't trigger a rebuild if the directory's timestamp changes.

```makefile
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
```

## Targets

```makefile
.PHONY: all test test-visual clean

all: maze          # default target

maze: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

test: build/test_stack build/test_linked_list build/test_backtrack
	./build/test_stack
	./build/test_linked_list
	./build/test_backtrack

test-visual: build/visual_test_maze build/visual_test_backpack
	./build/visual_test_maze mazes/maze_10x10.txt
	./build/visual_test_backpack

clean:
	rm -rf $(BUILD_DIR) maze
```

## Test Executables

Each test binary links only the modules it needs (not `main.o`):
```makefile
STRUCT_OBJ = $(BUILD_DIR)/stack.o $(BUILD_DIR)/linked_list.o

$(BUILD_DIR)/test_stack: tests/auto/test_stack.c $(BUILD_DIR)/stack.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_linked_list: tests/auto/test_linked_list.c $(BUILD_DIR)/linked_list.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_backtrack: tests/auto/test_backtrack.c $(BUILD_DIR)/stack.o \
                              $(BUILD_DIR)/linked_list.o $(BUILD_DIR)/maze.o \
                              $(BUILD_DIR)/backtrack.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@
```

## Automatic Dependency Tracking (optional but recommended)

Avoids stale `.o` files when a header changes:
```makefile
CFLAGS += -MMD -MP
-include $(OBJ:.o=.d)
```

`-MMD` generates a `.d` file next to each `.o` with the header dependencies.  
`-include` silently skips missing `.d` files on the first build.
