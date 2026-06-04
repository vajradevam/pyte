# Test: for x in list iteration
print("=== t09_forlist ===")

# ── iterate literal list ──
for x in [10, 20, 30]:
    print(x)

# ── iterate list variable ──
a = [1, 2, 3]
for x in a:
    print(x)

# ── empty list ──
for x in []:
    print("should not print")
print("empty done")

# ── single-element list ──
for x in [42]:
    print(x)

# ── list of strings ──
for s in ["hello", "world"]:
    print(s)

# ── list of bools ──
for b in [True, False, True]:
    print(b)

# ── nested: for range + for list ──
for i in range(3):
    for j in ["a", "b"]:
        print(i, j)

# ── nested: for list + for list ──
for x in [1, 2]:
    for y in ["x", "y"]:
        print(x, y)

# ── for list with break ──
for x in [1, 2, 3, 4, 5]:
    if x == 3:
        break
    print(x)
print("break done")

# ── for list with continue ──
for x in [1, 2, 3, 4, 5]:
    if x == 3:
        continue
    print(x)
print("continue done")

# ── for list with break in nested loop ──
for i in range(3):
    for j in [10, 20, 30]:
        if j == 20:
            break
        print(i, j)
print("nested break done")

# ── for list inside function ──
def sum_list(lst):
    s = 0
    for x in lst:
        s = s + x
    return s
print(sum_list([1, 2, 3, 4, 5]))

# ── function returning list, then iterate ──
def make_items():
    return [100, 200, 300]
for v in make_items():
    print(v)

# ── for list with len and range combined ──
items = [10, 20, 30, 40]
for i in range(len(items)):
    print(items[i])

# ── for list with subscript and assignment ──
lst = [0, 0, 0]
for i in range(3):
    lst[i] = i * 10
print(lst)

# ── for list with mutable elements (list of lists) ──
# matrix = [[1, 2], [3, 4]]
# for row in matrix:
#     print(row[0], row[1])

# ── for list then reuse variable ──
for x in [1, 2]:
    print(x)
for x in ["a", "b"]:
    print(x)

print("=== t09_forlist OK ===")
