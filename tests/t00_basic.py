# Test: Basic literals, arithmetic, comparisons, variables
print("=== t00_basic ===")

# ── int literals ──
print(0)
print(42)
print(-7)
print(1000000)
print(-999999)

# ── float literals ──
print(3.14)
print(-2.5)
print(0.0)

# ── string literals ──
print("hello world")
print('single quotes')
print("")
print('')
print("spaces and .!? symbols")

# ── bool / None ──
print(True)
print(False)
print(None)

# ── arithmetic ──
print(1 + 2)
print(10 - 3)
print(5 * 6)
print(42 // 7)
print(10 % 3)
print(2 + 3 * 4)
print((2 + 3) * 4)
print(-5)
print(0 - 3)
print(2 * 3 + 4 // 2 - 1)
print(2 + 3 - 4 + 5 - 6)
print(0 + 0)
print(1 - 2 + 3 - 4 + 5 - 6 + 7)
print(2 * 3 * 4)
print(100 // 10 // 2)
print(0 // 5)
print(10 // 1)
print(7 % 2)
print(100 % 7)
print(0 - 0)
print(-(-5))

# ── mixed float/int arithmetic ──
print(1 + 2.0)
print(3.0 + 4)
print(10.0 - 3.5)
print(1.5 + 2.5)
print(3.0 * 4.5)
print(-2.5)
print(10.0 // 3)
print(10 // 3.0)

# ── comparisons ──
print(5 < 10)
print(5 > 10)
print(5 == 5)
print(5 != 6)
print(5 <= 5)
print(5 >= 5)
print(0 == 0)
print(0 != 1)
print(10 < 20)
print(100 > 99)
print(-5 < 0)
print(0 >= -1)

# ── cross-type comparison (bool vs int, None vs others) ──
print(True == 1)
print(True != 2)
print(False == 0)
print(False != 1)
print(None == None)
print(None != 0)
print(0 != None)
print(True == True)
print(False == False)
print(True != False)

# ── variables ──
x = 42
print(x)
y = x + 8
print(y)
z = y * 2
print(z)
x = 100
print(x)
print(x + y + z)

a = 0
b = 1
c = 2
print(a)
print(b)
print(c)
print(a + b + c)

# reassign many times
v = 1
v = v + 1
v = v + 1
v = v + 1
print(v)

# ── negative float division ──
print(-10.0 // 3)
print(10.0 // -3)
print(-10.0 // -3)

print("=== t00_basic OK ===")
