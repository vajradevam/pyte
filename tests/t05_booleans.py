# Test: Boolean operations — and, or, not, short-circuit, truthiness
print("=== t05_booleans ===")

# ── and / or / not (basic) ──
print(True and True)
print(True and False)
print(False and True)
print(False and False)
print(True or True)
print(True or False)
print(False or True)
print(False or False)
print(not True)
print(not False)
print(not not True)
print(not not False)

# ── with ints (truthy/falsy) ──
print(0 and 5)
print(1 and 5)
print(0 or 5)
print(1 or 5)
print(0 and 1 or 2)
print(1 and 2 or 3)
print(0 or 1 and 2)

# ── with strings ──
print("" or "default")
print("hello" or "default")
print("" and "nope")
print("yes" and "both")

# ── with None ──
print(None or "fallback")
print(None and "nope")
print("x" or None)

# ── with lists ──
print([] or "empty")
print([1] or "empty")
print([] and "nope")
print([1] and "both")

# ── chained (3+ operands) ──
print(0 or 0 or 5)
print(0 or 1 or 5)
print(1 and 2 and 3)
print(0 and 99 and 100)
print(1 or 2 or 3)
print(0 or "" or "last")

# ── not on various types ──
print(not 0)
print(not 1)
print(not "")
print(not "x")
print(not [])
print(not [1])
print(not None)
print(not True)
print(not False)

# ── short-circuit: function calls ──
def called():
    print("called!")
    return True

def not_called():
    print("should not print!")
    return False

# 0 and called() → short-circuits, called() NOT executed
x = 0 and called()
print("after 0 and called")

# 1 and called() → called() IS executed
x = 1 and called()
print("after 1 and called")

# 1 or called() → short-circuits, called() NOT executed
x = 1 or called()
print("after 1 or called")

# 0 or called() → called() IS executed
x = 0 or called()
print("after 0 or called")

# More complex short-circuit chains
x = 0 and called() and called()
print("after 0 and called and called")

x = 1 and 0 and called()
print("after 1 and 0 and called")

x = 0 or 1 or called()
print("after 0 or 1 or called")

x = 0 or 0 or called()
print("after 0 or 0 or called")

print("=== t05_booleans OK ===")
