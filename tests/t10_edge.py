# Test: Additional edge cases and stress tests
print("=== t10_edge ===")

# ── max int values ──
print(2147483647)
print(-2147483648)
print(2147483647 + 1)
print(-2147483648 - 1)

# ── zero edge cases ──
print(0 // 1)
print(0 % 1)
print(0 * 100)
print(0 + 0)
print(0 - 0)
print(-0)

# ── one edge cases ──
print(1 // 1)
print(1 * 1)
print(1 % 1)
print(1 * 1)

# ── empty programs (just print) ──
print("empty program ok")

# ── single-expression edge cases ──
# print(()) — empty tuple not supported
print((1))
print((1+2))

# ── None in comparisons ──
print(None == None)
print(None != None)
print(None != 0)
print(0 != None)
print(None != "")
print(True != None)
print(None != False)

# ── bool in comparisons ──
print(True == True)
print(True == False)
print(False == False)
print(True != False)
print(True == 1)
print(False == 0)
print(True != 2)
print(False != 1)

# ── str with special chars ──
print("newline\nhere")
print("tab\there")
print("back\\slash")
print("quote\"here")
print("mixed\n\t\\\"chars")

# ── long identifier (max 31 chars due to name buffer) ──
abcdefghijklmnopqrstuvwxyz12345 = 42
print(abcdefghijklmnopqrstuvwxyz12345)

# ── multiple globals ──
g0 = 0
g1 = 1
g2 = 2
g3 = 3
g4 = 4
g5 = 5
g6 = 6
g7 = 7
g8 = 8
g9 = 9
print(g0 + g1 + g2 + g3 + g4 + g5 + g6 + g7 + g8 + g9)

# ── function with no params and no body (empty block) ──
def empty_func():
    x = 1
print(empty_func())

# ── function returning expression with and/or ──
def choose(a, b):
    return a or b
print(choose(0, 5))
print(choose(3, 4))

# ── multiple returns in sequence ──
def multi_ret(x):
    if x == 0:
        return 0
    if x == 1:
        return 1
    return 2
print(multi_ret(0))
print(multi_ret(1))
print(multi_ret(2))

# ── while loop with break at very start ──
i = 0
while i < 100:
    break
    print("should not print")
print("break at start done")

# ── for loop with break at very start ──
for i in range(100):
    break
    print("should not print")
print("for break at start done")

# ── continue at start of loop ──
i = 0
while i < 5:
    i = i + 1
    continue
    print("should not print")
print("continue at start done")

# ── nested if without indent (same level, should work or error?) ──
# This pattern tests that the parser handles same-level if bodies
x = 1
if x > 0:
    print("outer")
    if x > 0:
        print("inner")
    print("outer after inner")
print("nested same level done")

# ── for with range(0, 10) step 1 ──
s = 0
for i in range(0, 10):
    s = s + i
print(s)

# ── function returning list ──
def make_list():
    return [1, 2, 3]
print(make_list())
print(len(make_list()))

# ── list with range ──
lst = [0, 1, 2, 3, 4]
for i in range(len(lst)):
    print(lst[i])

# ── bool in list with in ──
print(True in [True, False])
print(False in [True, False])
print(None in [None, 1])
print(None in [1, 2])
print(0 in [0])
print(0 in [])

# ── string edge: empty in non-empty ──
print("" in "abc")
print("" in "")

# ── function that returns other function's result ──
def double(x):
    return x * 2

def get_double():
    return double(5)
print(get_double())

# ── multiple levels of function calls ──
def id(x):
    return x
def compose(f, g, x):
    return f(g(x))
print(compose(id, id, 42))

print("=== t10_edge OK ===")
