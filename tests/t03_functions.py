# Test: Functions — def, return, recursion, nested calls, higher-order
print("=== t03_functions ===")

# ── no arguments ──
def greet():
    print("Hello!")
greet()
greet()

def noop():
    x = 1
print(noop())  # implicit return None

# ── single argument ──
def double(x):
    return x * 2
print(double(21))
print(double(0))
print(double(-3))

# ── multiple arguments ──
def add(a, b):
    return a + b
print(add(3, 4))
print(add(10, 20))
print(add(-5, 5))

def mul3(a, b, c):
    return a * b * c
print(mul3(2, 3, 4))

def sum4(a, b, c, d):
    return a + b + c + d
print(sum4(1, 2, 3, 4))

def avg(a, b, c, d):
    return (a + b + c + d) // 4
print(avg(1, 2, 3, 4))
print(avg(10, 20, 30, 40))

# ── early return / multiple returns ──
def first_positive(a, b, c):
    if a > 0:
        return a
    if b > 0:
        return b
    return c
print(first_positive(-1, 5, 10))
print(first_positive(-1, -2, 10))
print(first_positive(3, -1, 10))

def abs_val(x):
    if x >= 0:
        return x
    return -x
print(abs_val(5))
print(abs_val(0))
print(abs_val(-3))

# ── nested function calls in expression ──
def inc(x):
    return x + 1
def mul2(x):
    return x * 2
def sub3(x):
    return x - 3
print(inc(mul2(sub3(10))))
print(mul2(inc(sub3(10))))
print(sub3(inc(mul2(10))))

# ── function calling another function ──
def square(x):
    return x * x
def sum_of_squares(a, b):
    return square(a) + square(b)
print(sum_of_squares(3, 4))

# ── higher-order: function passed as argument ──
def apply(f, x):
    return f(x)
def triple(x):
    return x * 3
def quadruple(x):
    return x * 4
print(apply(triple, 5))
print(apply(quadruple, 5))
print(apply(double, 10))  # double from earlier

# ── multiple functions with same logic ──
def inc2(x):
    return x + 1
def dec2(x):
    return x - 1
print(inc2(5))
print(dec2(5))
print(inc2(dec2(10)))
print(dec2(inc2(10)))

# ── factorial (recursive) ──
def fact(n):
    if n <= 1:
        return 1
    return n * fact(n-1)
print(fact(0))
print(fact(1))
print(fact(2))
print(fact(3))
print(fact(4))
print(fact(5))
print(fact(6))
print(fact(7))

# ── fibonacci (recursive, dual calls) ──
def fib(n):
    if n < 2:
        return n
    return fib(n-1) + fib(n-2)
print(fib(0))
print(fib(1))
print(fib(2))
print(fib(3))
print(fib(4))
print(fib(5))
print(fib(6))
print(fib(7))
print(fib(8))
print(fib(9))
print(fib(10))

# ── deep recursion (50 levels) ──
def rec(n):
    if n <= 0:
        return 0
    return rec(n-1)
print(rec(50))

# ── function defined inside if (conditional def) ──
x = 5
if x > 0:
    def pos_func():
        print("positive path")
    pos_func()
else:
    def neg_func():
        print("negative path")
    neg_func()
print("conditional def ok")

# ── function with for loop inside ──
def sum_to(n):
    s = 0
    for i in range(n+1):
        s = s + i
    return s
print(sum_to(10))

print("=== t03_functions OK ===")
