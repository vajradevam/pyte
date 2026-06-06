# pyte TODO

## High priority
- [x] **`for x in lst`** — extend FORITER to iterate lists (not just range)
- [x] **Fix `OP_SET` stack bug** — was pushing value back, corrupting stack in loops
- [x] **Fix nested for-break** — break now pops iterator before exiting
- [x] **String methods** — `.upper()`, `.lower()`, `.strip()`
- [x] **REPL** — interactive mode with multi-line block support
- [x] **Improve REPL** — expression result printing, multi-line handling, error recovery
- [x] **`sys.argv`** — command-line args as `argv` list
- [ ] **REPL command history

## Code quality
- [ ] **Split into multiple files** — `lexer.c`, `compile.c`, `vm.c`, `main.c`, `pyte.h`
- [ ] **Replace fprintf/stderr errors with clean error handler**
- [ ] **Remove debug strings** (token names, etc.) to shrink binary
- [ ] **Use raw syscalls** instead of `printf`/`fprintf` for smaller binary

## Size optimization (Bellard-style)
- [ ] **Inline lexer** — replace token stream with character-at-a-time parsing
- [ ] **Tighter opcode encoding** — pack immediates, shorter jumps
- [ ] **Eliminate `setjmp`/`longjmp`** for error handling
- [ ] **Static ELF layout** — hand-tuned sections, no relocations
- [ ] **BSS-only allocations** — move from fixed buffers to zero-initialized BSS

## Features
- [ ] **List `.insert()`, `.remove()`**
- [ ] **`sys.argv`** — pass command-line args as list
- [ ] **Range with step** — `range(start, stop, step)`
- [ ] **Multiple assignment** — `x, y = 1, 2`
- [ ] **String multiplication** — `"x" * 3`

## Tests
- [ ] **`for x in lst` tests**
- [ ] **String method tests**
- [ ] **REPL tests**
- [ ] **Error case tests** (pop empty, div by zero, etc.)
