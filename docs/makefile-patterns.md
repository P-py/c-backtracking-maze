# Makefile Patterns

How the project's Makefile is structured and why.

## Variables

```makefile
CC     = gcc
CFLAGS = -Wall -Wextra -g -Isrc -MMD -MP -Iinclude

BUILD  = build
```

`-Iinclude` lets every file use `#include <maze.h>` (angle-bracket style) regardless of where it lives in `src/`.  
`-Isrc` is kept so nested source paths remain resolvable during compilation.  
`-MMD -MP` generate `.d` dependency files that track header changes (see below).  
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

## Automatic Dependency Tracking

Avoids stale `.o` files when a header changes. Already enabled in `CFLAGS` via `-MMD -MP`:

```makefile
-include $(OBJ:.o=.d)
```

`-MMD` generates a `.d` file alongside each `.o` listing all headers the source depends on.  
`-include` (with the leading `-`) silently skips missing `.d` files on the first build.  
After the first build, changing any header automatically triggers a recompile of every affected `.o`.
