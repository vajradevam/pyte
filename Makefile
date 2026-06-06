CC = gcc
CFLAGS = -O2 -Wall -Wextra
LDFLAGS = -s

TARGET = pyte
SRC = pyte.c
TESTS_DIR = tests

.PHONY: all compile debug size tiny-nolibc run test test-one clean summary

all: compile size tiny-nolibc

# ── Build (balanced, with libc) ──
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
size: CFLAGS = -Os -s -Wall -Wextra -fomit-frame-pointer \
               -fmerge-all-constants -fno-unwind-tables \
               -fno-asynchronous-unwind-tables -fno-stack-protector \
               -fno-builtin
size: LDFLAGS = -s -Wl,--gc-sections -Wl,-s
size: $(TARGET)-tiny

$(TARGET)-tiny: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	@ls -la $@

# ── Static binary, no libc, aggressively stripped ──
TINY_NOLIBC_FLAGS = -Os -s -Wall -Wextra -fomit-frame-pointer \
                     -fmerge-all-constants -fno-unwind-tables \
                     -fno-asynchronous-unwind-tables -fno-stack-protector \
                     -fno-builtin
TINY_NOLIBC_LDFLAGS = -s -Wl,--gc-sections -Wl,-s -Wl,--build-id=none

tiny-nolibc: $(TARGET)-tiny-nolibc

$(TARGET)-tiny-nolibc: $(SRC)
	@echo "  CC      $@"
	$(CC) -nostdlib -static $(TINY_NOLIBC_FLAGS) $(TINY_NOLIBC_LDFLAGS) \
	      -o $@.tmp $<
	@echo "  STRIP   notes+comment"
	@objcopy --remove-section=.note* --remove-section=.comment \
	         $@.tmp $@.tmp2 2>/dev/null && mv $@.tmp2 $@.tmp
	@echo "  STRIP   section headers"
	@./strip_elf.py $@.tmp
	@mv $@.tmp $@
	@ls -la $@
	@size $@

# ── Quick smoke test ──
run: $(TARGET)
	@if [ -n "$(CODE)" ]; then \
		./$(TARGET) -e "$$CODE"; \
	elif [ -n "$(FILE)" ]; then \
		./$(TARGET) "$$FILE"; \
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
	@./$(TARGET) "$$FILE"

# ── Clean ──
clean:
	rm -f $(TARGET) $(TARGET)-dbg $(TARGET)-tiny $(TARGET)-tiny-nolibc
	rm -f *.tmp
	@echo "clean done"

# ── Info ──
summary:
	@echo "pyte — tiny Python interpreter"
	@echo "  binary:     ~39K (-O2 -s)"
	@echo "  tiny:       ~27K (-Os, gc-sections, with libc)"
	@echo "  nolibc:     ~22K (static, no libc, fully stripped)"
	@echo "  source:     $(SRC) ($$(wc -l < $(SRC)) lines)"
	@echo "  tests:      $$(ls $(TESTS_DIR)/t*.py 2>/dev/null | wc -l) files"
	@echo ""
	@echo "targets:"
	@echo "  make all           — build all 3 targets"
	@echo "  make               — build $(TARGET) (-O2 -s)"
	@echo "  make compile       — build $(TARGET)"
	@echo "  make debug         — build with -O0 -g -DDEBUG"
	@echo "  make size          — $(TARGET)-tiny (-Os, gc-sections)"
	@echo "  make tiny-nolibc   — $(TARGET)-tiny-nolibc (fully stripped, ~22K)"
	@echo "  make run           — quick smoke test"
	@echo "  make test          — full test suite"
	@echo "  make test-one FILE=... — single test"
	@echo "  make clean         — remove binaries"
	@echo "  make summary       — this message"
