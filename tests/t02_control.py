# Test: Control flow — if/elif/else, while, for, break, continue
print("=== t02_control ===")

# ── simple if (true) ──
x = 10
if x > 5:
    print("big")

# ── simple if with else (false path) ──
x = -5
if x > 0:
    print("positive")
else:
    print("negative")

# ── if with else (true path) ──
x = 5
if x > 0:
    print("positive")
else:
    print("negative")

# ── if/elif/else (elif taken) ──
x = 10
if x > 20:
    print(">20")
elif x > 5:
    print(">5")
else:
    print("<=5")

# ── if/elif/else (else taken) ──
x = 0
if x > 20:
    print(">20")
elif x > 5:
    print(">5")
elif x > 0:
    print(">0")
else:
    print("<=0")

# ── if/elif (no else) ──
x = 3
if x > 10:
    print(">10")
elif x > 5:
    print(">5")
elif x > 2:
    print(">2")
elif x > 0:
    print(">0")
print("elif chain done")

# ── if/elif (no else, all false) ──
x = 0
if x > 10:
    print(">10")
elif x > 5:
    print(">5")
elif x > 2:
    print(">2")
elif x > 0:
    print(">0")
print("all elifs false")

# ── deeply nested if/else (depth 3) ──
x = 5
if x > 0:
    if x > 2:
        if x > 4:
            print("deep: x>4")
        else:
            print("deep: 2<x<=4")
    else:
        print("deep: 0<x<=2")
else:
    print("deep: x<=0")

# ── deeply nested (all paths) ──
x = 1
if x > 0:
    if x > 2:
        print("nested A")
    else:
        print("nested B")
else:
    print("nested C")

x = -1
if x > 0:
    if x > 2:
        print("nested D")
    else:
        print("nested E")
else:
    print("nested F")

# ── while loop (basic) ──
i = 0
s = 0
while i < 10:
    s = s + i
    i = i + 1
print(s)

# ── while loop (zero iterations) ──
while 0:
    print("should not print")
print("while false done")

# ── while with break condition (set flag) ──
n = 0
while n < 100:
    if n * n > 50:
        print(n)
        n = 200
    n = n + 1

# ── while with complex condition ──
i = 1
while i < 10 and i % 3 != 0:
    i = i + 1
print(i)

# ── while with inner if printing even numbers ──
i = 0
while i < 5:
    i = i + 1
    if i % 2 == 0:
        print(i)

# ── while with break (keyword) ──
i = 0
while i < 10:
    if i == 3:
        break
    print(i)
    i = i + 1

# ── while with continue (keyword) ──
i = 0
while i < 5:
    i = i + 1
    if i == 3:
        continue
    print(i)
print("while continue done")

# ── for loop with range(stop) ──
for i in range(5):
    print(i)

# ── for loop with range(start, stop) ──
for i in range(3, 7):
    print(i)

# ── for loop (empty range) ──
for i in range(0):
    print("should not print")
print("for empty done")

# ── for loop (range 0, 0) ──
for i in range(0, 0):
    print("should not print")
print("for zero range done")

# ── for loop with break ──
for i in range(5):
    if i == 2:
        break
    print(i)

# ── for loop with continue ──
for i in range(5):
    if i == 2:
        continue
    print(i)

# ── nested loop (while inside for) ──
i = 0
while i < 3:
    for k in range(2):
        print(i * 2 + k)
    i = i + 1

# ── nested loop (while inside while) ──
i = 0
while i < 3:
    j = 0
    while j < 3:
        print(i * 3 + j)
        j = j + 1
    i = i + 1

# ── nested break (inner only, outer continues) ──
i = 0
while i < 5:
    j = 0
    while j < 5:
        print(i, j)
        if j == 1:
            break
        j = j + 1
    if i == 2:
        break
    i = i + 1
print("nested break done")

# ── for with range in function ──
def sum_range(n):
    s = 0
    for i in range(n):
        s = s + i
    return s
print(sum_range(5))
print(sum_range(10))

print("=== t02_control OK ===")

# ── for loop nested inside if (true branch) ──
if 1:
    for i in range(3):
        print(i)
print("for in if done")

# ── for loop nested inside if (false branch) ──
if 0:
    for i in range(3):
        print("should not print")
print("for in false if done")

# ── while with continue that skips increment — careful pattern ──
i = 0
while i < 5:
    i = i + 1
    if i == 2:
        continue
    if i == 4:
        continue
    print(i)
print("continue skip done")

# ── for loop with same variable name ──
for x in range(2):
    print(x)
for x in range(2, 4):
    print(x)
print("reuse var done")
