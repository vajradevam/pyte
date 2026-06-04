# pyte: Tiny Python interpreter

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
- `for x in list` (only `for x in range(...)`)
- String methods (`.upper()`, etc.) beyond `.pop()` for lists
- Slice notation `[1:3]`
- `while`/`else`, `for`/`else`
- `list.insert()`, `.remove()` (only `.append()` and `.pop()`)
- File I/O, sys.argv

## Size

| Build | Flags | Binary |
|---|---|---|
| `make compile` | `-O2 -s` | 43K |
| `make size` | `-Os -Wl,--gc-sections` | 27K |

## Files

```
pyte.c            interpreter (1599 lines)
Makefile          build targets
run_tests.sh      test runner
tests/            10 test files, all pass
```
