# Tiny Python interpreter

A minimal Python bytecode interpreter in ~1600 lines of C. Single pass compiler, small VM, no malloc per operation, no external deps.

## Quick start

```
make         # build
make test    # run all tests
./pyte file.py
./pyte -e "print(1+2)"
```

## Supported Python subset

| Feature | Details |
|---|---|
| **Types** | `int`, `float`, `str`, `bool`, `None` |
| **Lists** | literal, subscript `lst[i]`, assign `lst[i]=x`, negative indices, `.append()`, `.pop()`, `len()`, `in`/`not in` |
| **Strings** | subscript, len, comparison, `in`/`not in` (substring), concatenation `+` |
| **Arithmetic** | `+`, `-`, `*`, `//`, `%`, unary `-`, mixed int/float |
| **Comparisons** | `==`, `!=`, `<`, `>`, `<=`, `>=` across all types including cross-type (bool↔int, None↔any) |
| **Variables** | global and local scope, reassignment |
| **Control** | `if`/`elif`/`else`, `while`, `for x in range(...)` |
| **Loops** | `break`, `continue`, nested loops |
| **Functions** | `def`/`return`, multiple params, recursion, higher-order (pass/return functions), conditional def |
| **Logic** | `and`, `or`, `not` (short-circuit) |
| **Comments** | `#` line comments |
| **Print** | `print(...)` with multiple arguments |

## Not supported (intentionally)

- Classes, exceptions, generators, modules, imports
- String methods (`.upper()`, etc.) beyond `.pop()` for lists
- Slice notation `[1:3]`
- `while`/`else`, `for`/`else`
- `list.insert()`, `.remove()` (only `.append()` and `.pop()`)
- File I/O, sys.argv

## Size

| Target | Flags | Size |
|---|---|---|
| `make` | `-O2 -s` | 39K |
| `make size` | `-Os -Wl,--gc-sections` | 27K |
| `make tiny-nolibc` | `-nostdlib -static -Os` + post-processing | **22K** |

The `tiny-nolibc` target produces a fully static binary with zero external dependencies. The complete build pipeline is:

1. **Compile** with `-nostdlib -static -Os -Wl,--gc-sections -Wl,--build-id=none -fno-unwind-tables` to remove all standard library references and unused sections at link time.
2. **Strip notes and comment** via `objcopy --remove-section=.note* --remove-section=.comment` to delete metadata sections that the loader does not require.
3. **Remove section headers** by zeroing `e_shoff` in the ELF header and truncating the file, since the kernel only needs the program headers to load the binary.

### How the size is achieved

All standard library functions have been replaced with minimal inline equivalents or eliminated entirely:

- **No libc at all.** System calls (`read`, `write`, `open`, `close`, `lseek`, `exit`) are performed via inline assembly (`syscall` instruction) with register constraints. No PLT entries, no dynamic linking overhead.
- **No `malloc`/`free`.** Bytecode, source text, and function objects use static pools (`bc_buf[128K]`, `src_buf[256K]`, `FuncObj[64]`). No heap allocation.
- **No `stdio`.** File I/O uses raw POSIX `open`/`read`/`close`. Output uses bare `write(1,...)` and `write(2,...)`.
- **No `string.h`.** String operations (`strcmp`, `strcpy`, `memcmp`, `memcpy`, `memset`, `strlen`) are implemented as tiny inline loops (3-5 lines each). No library calls.
- **No `ctype.h`.** Character class checks (`isalpha`, `isalnum`) are inline range checks.
- **No `setjmp`/`longjmp`.** Error propagation uses a global flag + `goto` pattern, saving a few hundred bytes and removing two libc symbols.
- **No `snprintf`.** Float-to-string conversion is a custom `ftoa` that produces the shortest decimal representation.
- **Minimal error mnemonics.** All error messages are single-word codes (2-5 characters), stored compactly in `.rodata` (~500 bytes total for all strings).
- **Entry point.** A `__attribute__((naked, weak))` `_start` trampoline reads `argc`/`argv` from the stack and calls `main()`, then invokes `SYS_exit`. The weak linkage lets libc-linked builds use the standard CRT `_start`.

## Files

```
pyte.c            interpreter (1599 lines)
Makefile          build targets
strip_elf.py      ELF section stripper
run_tests.sh      test runner
tests/            10 test files, all pass
```
