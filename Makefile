CC      = clang
CFLAGS  = -Wall -Wextra -std=c99 -I src -I test/unity

# Work around MinGW gcc using Windows TEMP dir (which may lack write permission)
export TEMP  := /tmp
export TMP   := /tmp
export TMPDIR := /tmp

SRC_DIR   = src
TEST_DIR  = test
UNITY_DIR = test/unity
BUILD_DIR = build

# Source under test
SRC = $(SRC_DIR)/ansi_print.c $(SRC_DIR)/ansi_tui.c
HDR = $(SRC_DIR)/ansi_print.h $(SRC_DIR)/ansi_tui.h

# Unity framework
UNITY_SRC = $(UNITY_DIR)/unity.c

# Test sources (all test_*.c files in test/)
TEST_SRCS = $(wildcard $(TEST_DIR)/test_*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))

# Flags to disable all optional features (standard colors only)
MINIMAL_FLAGS = -DANSI_PRINT_NO_APP_CFG -DANSI_PRINT_MINIMAL

.PHONY: all ansiprint test test-minimal coverage docs clean

all: ansiprint test docs

# Command-line executable
ansiprint: $(BUILD_DIR)/ansiprint
$(BUILD_DIR)/ansiprint: $(SRC_DIR)/main.c $(SRC) $(HDR) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/main.c $(SRC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build each test runner
$(BUILD_DIR)/test_%: $(TEST_DIR)/test_%.c $(SRC) $(UNITY_SRC) $(HDR) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_$*.c $(SRC) $(UNITY_SRC)

# Run all tests
test: $(TEST_BINS)
	@echo "--- Running tests ---"
	@for t in $(TEST_BINS); do echo ""; echo ">> $$t"; $$t || exit 1; done
	@echo ""
	@echo "--- All tests passed ---"

# Build and run tests with all features disabled
test-minimal: $(BUILD_DIR)/test_cprint_minimal $(BUILD_DIR)/test_tui_minimal
	@echo "--- Running minimal tests ---"
	@for t in $^; do echo ""; echo ">> $$t"; $$t || exit 1; done
	@echo ""
	@echo "--- Minimal tests passed ---"

$(BUILD_DIR)/test_cprint_minimal: $(TEST_DIR)/test_cprint.c $(SRC) $(UNITY_SRC) $(HDR) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(MINIMAL_FLAGS) -o $@ $(TEST_DIR)/test_cprint.c $(SRC) $(UNITY_SRC)

$(BUILD_DIR)/test_tui_minimal: $(TEST_DIR)/test_tui.c $(SRC) $(UNITY_SRC) $(HDR) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(MINIMAL_FLAGS) -o $@ $(TEST_DIR)/test_tui.c $(SRC) $(UNITY_SRC)

# Code coverage (full + minimal builds merged)
COV_DIR = $(BUILD_DIR)/coverage
COV_FULL = $(COV_DIR)/full
COV_MIN  = $(COV_DIR)/minimal

coverage: | $(BUILD_DIR)
	@rm -rf $(COV_DIR)
	@mkdir -p $(COV_FULL) $(COV_MIN)
	@echo "=== Full-feature build ==="
	$(CC) $(CFLAGS) --coverage -c $(SRC) -o $(COV_FULL)/ansiprint.o
	$(CC) $(CFLAGS) --coverage -c $(TEST_DIR)/test_cprint.c -o $(COV_FULL)/test_cprint.o
	$(CC) $(CFLAGS) --coverage -c $(UNITY_SRC) -o $(COV_FULL)/unity.o
	$(CC) --coverage -o $(COV_FULL)/test_cov \
		$(COV_FULL)/ansiprint.o $(COV_FULL)/test_cprint.o $(COV_FULL)/unity.o
	$(COV_FULL)/test_cov
	@echo ""
	@echo "=== Minimal build ==="
	$(CC) $(CFLAGS) $(MINIMAL_FLAGS) --coverage -c $(SRC) -o $(COV_MIN)/ansiprint.o
	$(CC) $(CFLAGS) $(MINIMAL_FLAGS) --coverage -c $(TEST_DIR)/test_cprint.c -o $(COV_MIN)/test_cprint.o
	$(CC) $(CFLAGS) $(MINIMAL_FLAGS) --coverage -c $(UNITY_SRC) -o $(COV_MIN)/unity.o
	$(CC) --coverage -o $(COV_MIN)/test_cov \
		$(COV_MIN)/ansiprint.o $(COV_MIN)/test_cprint.o $(COV_MIN)/unity.o
	$(COV_MIN)/test_cov
	@echo ""
	@echo "--- Generating coverage reports ---"
	gcovr --filter $(SRC_DIR)/ \
		--html-details $(COV_FULL)/index.html --root . $(COV_FULL)
	gcovr --filter $(SRC_DIR)/ \
		--html-details $(COV_MIN)/index.html --root . $(COV_MIN)
	gcovr --filter $(SRC_DIR)/ --merge-mode-functions=merge-use-line-min \
		--html-details $(COV_DIR)/index.html --root . $(COV_FULL) $(COV_MIN)
	@echo ""
	@echo "  Full:    $(COV_FULL)/index.html"
	@echo "  Minimal: $(COV_MIN)/index.html"
	@echo "  Merged:  $(COV_DIR)/index.html"

# Documentation (requires doxygen)
docs: | $(BUILD_DIR)
	doxygen Doxyfile

clean:
	rm -rf $(BUILD_DIR)
