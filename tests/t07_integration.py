# Test: Integration — complex programs combining multiple features
print("=== t07_integration ===")

# ── factorial with while loop (iterative) ──
def factorial(n):
    result = 1
    i = 1
    while i <= n:
        result = result * i
        i = i + 1
    return result
print(factorial(5))
print(factorial(0))

# ── is_even + sum_evens (function calling function) ──
def is_even(x):
    if x % 2 == 0:
        return True
    return False

def sum_evens(limit):
    s = 0
    i = 0
    while i < limit:
        if is_even(i):
            s = s + i
        i = i + 1
    return s
print(sum_evens(10))

# ── Nested call with arithmetic and comparison ──
def max3(a, b, c):
    if a >= b and a >= c:
        return a
    if b >= a and b >= c:
        return b
    return c
print(max3(1, 2, 3))
print(max3(3, 2, 1))
print(max3(1, 3, 2))
print(max3(5, 5, 5))

# ── for loop with list operations ──
def list_sum(lst):
    s = 0
    for i in range(len(lst)):
        s = s + lst[i]
    return s
print(list_sum([1, 2, 3, 4, 5]))
print(list_sum([]))
print(list_sum([10]))

# ── filter function ──
def filter_even(lst):
    result = []
    for i in range(len(lst)):
        val = lst[i]
        if val % 2 == 0:
            result.append(val)
    return result
print(filter_even([1, 2, 3, 4, 5, 6]))
print(filter_even([1, 3, 5]))
print(filter_even([]))

# ── break as "found" flag ──
def has_value(lst, val):
    found = False
    for i in range(len(lst)):
        if lst[i] == val:
            found = True
            break
    return found
print(has_value([1, 2, 3, 4, 5], 3))
print(has_value([1, 2, 3, 4, 5], 99))
print(has_value([], 1))

# ── continue to skip items ──
def sum_positive(lst):
    s = 0
    for i in range(len(lst)):
        if lst[i] <= 0:
            continue
        s = s + lst[i]
    return s
print(sum_positive([1, -2, 3, -4, 5]))
print(sum_positive([-1, -2, -3]))
print(sum_positive([1, 2, 3]))

# ── complex condition function ──
def is_prime(n):
    if n < 2:
        return False
    i = 2
    while i < n:
        if n % i == 0:
            return False
        i = i + 1
    return True
print(is_prime(7))
print(is_prime(4))
print(is_prime(1))
print(is_prime(13))
print(is_prime(2))

# ── multiple recursion calls ──
def ack(m, n):
    if m == 0:
        return n + 1
    if n == 0:
        return ack(m - 1, 1)
    return ack(m - 1, ack(m, n - 1))
print(ack(0, 3))
print(ack(1, 2))
print(ack(2, 1))

# ── mutable list across function calls ──
def append_one(lst):
    lst.append(1)

def append_two(lst):
    lst.append(2)

lst = []
append_one(lst)
append_two(lst)
append_one(lst)
print(lst)

# ── list of expressions ──
print([1 + 2, 3 * 4, 10 - 5, 100 // 10, 7 % 3])

# ── using len and range together ──
arr = [10, 20, 30, 40]
for i in range(len(arr)):
    print(arr[i])

# ── mixed arithmetic, comparison, boolean ──
x = 5
y = 10
z = 15
if x < y and y < z:
    print("x < y < z")
if z > y and y > x:
    print("descending")
if not (x > y):
    print("not (x > y)")

# ── variables in lists ──
a = 1
b = 2
c = 3
lst = [a, b, c]
print(lst[0])
print(lst[1])
print(lst[2])

print("=== t07_integration OK ===")

# ── deeply nested blocks (6 levels) ──
x = 6
if x > 0:
    if x > 1:
        if x > 2:
            if x > 3:
                if x > 4:
                    if x > 5:
                        print("deep 6")
                    else:
                        print("level 5 else")
                else:
                    print("level 4 else")
            else:
                print("level 3 else")
        else:
            print("level 2 else")
    else:
        print("level 1 else")
else:
    print("level 0 else")

# ── deeply nested for inside while inside for inside if ──
if 1:
    for a in range(2):
        i = 0
        while i < 2:
            for b in range(2):
                print(a, i, b)
                if b == 0:
                    print("first")
            i = i + 1
print("deep nest done")

# ── list of strings ──
words = ["hello", "world", "python"]
print(words[0])
print(words[1])
print(words[2])
print(len(words))
words.append("!")
print(words)

# ── string concatenation in functions ──
def greet(name):
    return "Hello, " + name
print(greet("world"))
print(greet("python"))

# ── multiple consecutive breaks (each exits its own loop) ──
i = 0
while i < 5:
    j = 0
    while j < 5:
        print(i, j)
        if j == 0:
            break
        j = j + 1
    if i == 1:
        break
    i = i + 1
print("multi break done")
