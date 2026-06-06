CC = gcc
CFLAGS = -O2 -Wall -Wextra
LDFLAGS = -s

TARGET = pyte
SRC = pyte.c
TESTS_DIR = tests
TEST_FILES = $(wildcard $(TESTS_DIR)/t*.py)

.PHONY: all compile debug size tiny-nolibc run test test-one clean summary

all: compile

# ── Build (balanced) ──
compile: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# ── Debug build ──
debug: CFLAGS = -O0 -g -Wall -Wextra -DDEBUG
debug: LDFLAGS =
debug: $(TARGET)-dbg

$(TARGET)-dbg: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# ── Extreme size optimization (with libc) ──
size: CFLAGS = -Os -s -Wall -Wextra -fomit-frame-pointer -fmerge-all-constants -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-stack-protector -fno-builtin
size: LDFLAGS = -s -Wl,--gc-sections -Wl,-s
size: $(TARGET)-tiny

$(TARGET)-tiny: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	@echo ""
	@echo "--- size report ---"
	@ls -la $@
	@echo "--- sections ---"
	@size $@
	@which sstrip 2>/dev/null && sstrip $@ 2>/dev/null && echo "--- after sstrip ---" && ls -la $@ || true

# ── Static binary, no libc at all ──
tiny-nolibc: CFLAGS = -Os -s -Wall -Wextra -fomit-frame-pointer -fmerge-all-constants -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-stack-protector -fno-builtin
tiny-nolibc: LDFLAGS = -s -Wl,--gc-sections -Wl,-s
tiny-nolibc: $(TARGET)-tiny-nolibc

$(TARGET)-tiny-nolibc: $(SRC)
	$(CC) -nostdlib -static $(CFLAGS) $(LDFLAGS) -o $@ $<
	@echo ""
	@echo "--- size report ---"
	@ls -la $@
	@size $@

# ── Quick smoke test ──
run: $(TARGET)
	@if [ -n "$(CODE)" ]; then \
		./$(TARGET) -e "$(CODE)"; \
	elif [ -n "$(FILE)" ]; then \
		./$(TARGET) "$(FILE)"; \
	elif [ -f $(TESTS_DIR)/t00_basic.py ]; then \
		echo "--- running $(TESTS_DIR)/t00_basic.py ---"; \
		./$(TARGET) $(TESTS_DIR)/t00_basic.py; \
	else \
		echo "Error: no test file found"; exit 1; \
	fi

# ── Run the full test suite ──
test: $(TARGET)
	@./run_tests.sh

# ── Run a single test file ──
test-one: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make test-one FILE=tests/tNN_xxx.py"; exit 1; \
	fi
	@./$(TARGET) "$(FILE)"

# ── Clean ──
clean:
	rm -f $(TARGET) $(TARGET)-dbg $(TARGET)-tiny $(TARGET)_asan
	@echo "clean done"

# ── Info ──
summary:
	@echo "pyte — tiny Python interpreter"
	@echo "  binary:     ~39K stripped (-O2 -s)"
	@echo "  tiny:       ~27K stripped (-Os, gc-sections)"
	@echo "  nolibc:     ~26K static, no libc"
	@echo "  source:     $(SRC) ($$(wc -l < $(SRC)) lines)"
	@echo "  tests:      $$(ls $(TESTS_DIR)/t*.py 2>/dev/null | wc -l) files"
	@echo ""
	@echo "targets:"
	@echo "  make               — build (default, -O2 -s)"
	@echo "  make compile       — build"
	@echo "  make debug         — build with -O0 -g -DDEBUG"
	@echo "  make size          — extreme size optimization (-Os, gc-sections)"
	@echo "  make tiny-nolibc   — static binary, no libc (~26K)"
	@echo "  make run           — quick smoke test"
	@echo "  make test          — full test suite"
	@echo "  make test-one FILE=... — single test"
	@echo "  make clean         — remove binaries"
	@echo "  make summary       — this message"
