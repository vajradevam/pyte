# Test: List operations
print("=== t04_lists ===")

# ── empty list ──
a = []
print(len(a))

# ── single element ──
a = [5]
print(len(a))
print(a[0])

# ── multiple elements ──
arr = [10, 20, 30]
print(arr[0])
print(arr[1])
print(arr[2])

# ── assign by index ──
arr = [1, 2, 3]
arr[1] = 42
print(arr[1])
arr[0] = 99
arr[2] = 77
print(arr[0])
print(arr[1])
print(arr[2])

# ── sum elements ──
arr = [10, 20, 30]
print(arr[0] + arr[1] + arr[2])

# ── negative indices ──
arr = [10, 20, 30]
print(arr[-1])
print(arr[-2])
print(arr[-3])

# ── compute in expression ──
print([1, 2, 3][1])
print([1+2, 3*4, 10-5][2])
print([10, 20][0] + [30, 40][1])

# ── at the end ──
print([1, 2, 3, 4, 5][4])
print([1, 2, 3, 4, 5][-1])

# ── in / not in (list membership) ──
print(1 in [1, 2, 3])
print(4 in [1, 2, 3])
print(1 not in [1, 2, 3])
print(4 not in [1, 2, 3])
print(0 in [])
print(0 not in [])
print("a" in ["a", "b", "c"])
print("x" in ["a", "b", "c"])
print(True in [True, False])
print(None in [None, 1])
print(42 in [42])

# ── len on lists ──
print(len([]))
print(len([1]))
print(len([1, 2, 3]))
print(len([1, [2, 3], 4]))  # nested list counts as one element

# ── append ──
a = [1, 2, 3]
a.append(4)
print(a)
a.append(5)
print(a)

# ── pop ──
a = [10, 20, 30, 40]
a.pop()
print(a)
a.pop()
print(a)

# ── append then pop ──
a = []
a.append(1)
a.append(2)
a.append(3)
a.pop()
print(a)
a.pop()
print(a)
a.pop()
print(a)

# ── append to pre-populated list ──
a = [10, 20]
a.append(30)
print(a)
a.pop()
print(a)

print("=== t04_lists OK ===")

# ── empty list append/pop cycle ──
a = []
a.append(1)
print(a)
a.pop()
print(a)
a.append(2)
a.append(3)
print(a)
print(len(a))
a.pop()
a.pop()
print(a)
print(len(a))

# ── bool and None in lists ──
b = [True, False, None]
print(b[0])
print(b[1])
print(b[2])
print(len(b))

# ── in on empty list ──
print(1 in [])
print(1 not in [])
