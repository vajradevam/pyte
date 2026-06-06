# Test: function as value (assign to var, call through var)
print("=== t17_funcval ===")

# Simple function as value
def add(x, y):
    return x + y

op = add
print(op(3, 4))
print(op(10, 20))

# Reassign variable holding function
def mul(x, y):
    return x * y

def sub(x, y):
    return x - y

f = mul
print(f(5, 6))
f = sub
print(f(10, 3))

# Function as argument
def apply(f, x, y):
    return f(x, y)

print(apply(add, 100, 1))
print(apply(mul, 7, 8))

# Identity function (one arg)
def double(x):
    return x * 2

def call_it(f, v):
    return f(v)

print(call_it(double, 21))
print(call_it(double, 0))

# Conditional function selection
def choose(cond):
    if cond:
        return add
    else:
        return sub

g = choose(True)
print(g(10, 3))
g = choose(False)
print(g(10, 3))

# Pass function through multiple levels
def compose(f, g, x):
    return f(g(x))

def inc(x):
    return x + 1

print(compose(double, inc, 5))
print(compose(inc, double, 5))

print("=== t17_funcval OK ===")
