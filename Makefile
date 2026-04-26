CC     = gcc
CFLAGS = -Wall -Wextra -g -Isrc -MMD -MP
BUILD  = build

# ── Main objects ────────────────────────────────────────────────────────────
OBJ = $(BUILD)/main.o       \
      $(BUILD)/maze.o       \
      $(BUILD)/stack.o      \
      $(BUILD)/linked_list.o \
      $(BUILD)/backtrack.o  \
      $(BUILD)/renderer.o

-include $(OBJ:.o=.d)

# ── Top-level targets ────────────────────────────────────────────────────────
.PHONY: all test test-visual clean

all: maze

maze: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# ── Automated tests ──────────────────────────────────────────────────────────
test: $(BUILD)/test_stack $(BUILD)/test_linked_list $(BUILD)/test_backtrack
	@echo "=== test_stack ==="
	./$(BUILD)/test_stack
	@echo "=== test_linked_list ==="
	./$(BUILD)/test_linked_list
	@echo "=== test_backtrack ==="
	./$(BUILD)/test_backtrack

$(BUILD)/test_stack: tests/auto/test_stack.c $(BUILD)/stack.o | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/test_linked_list: tests/auto/test_linked_list.c $(BUILD)/linked_list.o | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/test_backtrack: tests/auto/test_backtrack.c \
                         $(BUILD)/stack.o $(BUILD)/linked_list.o \
                         $(BUILD)/maze.o $(BUILD)/backtrack.o $(BUILD)/renderer.o | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

# ── Visual tests ─────────────────────────────────────────────────────────────
test-visual: $(BUILD)/visual_test_maze $(BUILD)/visual_test_backpack
	./$(BUILD)/visual_test_maze mazes/maze_10x10.txt
	./$(BUILD)/visual_test_backpack

$(BUILD)/visual_test_maze: tests/visual/visual_test_maze.c \
                           $(BUILD)/maze.o $(BUILD)/renderer.o $(BUILD)/linked_list.o | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/visual_test_backpack: tests/visual/visual_test_backpack.c \
                               $(BUILD)/linked_list.o | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

# ── Compile rules ─────────────────────────────────────────────────────────────
$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/maze/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/engine/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/structures/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	rm -rf $(BUILD) maze
