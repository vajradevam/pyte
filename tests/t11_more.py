# Test: More edge cases — comparison chaining, deep nesting, complex expressions
print("=== t11_more ===")

# =============================================
# COMPARISON CHAINING (left-to-right)
# =============================================

# Python does a < b < c as (a < b) and (b < c)
# Our interpreter does (a < b) < c (standard left-to-right)
# Both are useful to test

# --- chain: ints ---
print(1 < 2 < 3)   # (1<2)=True; True<3? True==1 so 1<3=True -> True
print(3 > 2 > 1)   # (3>2)=True; True>1? True==1 so 1>1=False
print(1 < 5 < 2)   # (1<5)=True; True<2? True==1 so 1<2=True
print(5 > 10 > 0)  # (5>10)=False; False>0? False==0 so 0>0=False
print(2 <= 2 < 4)  # (2<=2)=True; True<4? True==1 so 1<4=True
print(1 == 1 == 1) # (1==1)=True; True==1? True==1 so True
print(1 == 2 == 1) # (1==2)=False; False==1? False==0 so 0==1=False

# --- chain: mixed types ---
print(1 == True < 2)   # (1==True)=True; True<2? True==1 so 1<2=True
print(0 == False > -1) # (0==False)=True; True>-1? True==1 so 1>-1=True
print(False < True == 1) # (False<True)=True; True==1 so True
print(None == None == None) # (None==None)=True; True==None? ... badcmp->None

# --- chain: strings ---
print("a" < "b" < "c") # ("a"<"b")=True; True<"c"? ... badcmp->None
print("ab" > "aa" > "a") # ("ab">"aa")=True; True>"a"? ... badcmp->None

# =============================================
# DEEP NESTING
# =============================================

# --- deeply nested if (depth 7) ---
x = 7
if x > 0:
    if x > 1:
        if x > 2:
            if x > 3:
                if x > 4:
                    if x > 5:
                        if x > 6:
                            print("depth7 true")
                        else:
                            print("depth7 else6")
                    else:
                        print("depth7 else5")
                else:
                    print("depth7 else4")
            else:
                print("depth7 else3")
        else:
            print("depth7 else2")
    else:
        print("depth7 else1")
else:
    print("depth7 else0")

# --- deeply nested loop (for inside while inside for) ---
s = 0
for a in range(3):
    i = 0
    while i < 2:
        for b in range(4):
            s = s + a + i + b
        i = i + 1
print(s)
print("deep loop done")

# =============================================
# FUNCTION EDGE CASES
# =============================================

# --- function with many parameters ---
def sum6(a, b, c, d, e, f):
    return a + b + c + d + e + f
print(sum6(1, 2, 3, 4, 5, 6))
print(sum6(10, 20, 30, 40, 50, 60))
print(sum6(-1, 0, 1, -2, 2, -3))

# --- function with 8 parameters ---
def sum8(a, b, c, d, e, f, g, h):
    return a + b + c + d + e + f + g + h
print(sum8(1, 2, 3, 4, 5, 6, 7, 8))
print(sum8(10, 10, 10, 10, 10, 10, 10, 10))

# --- nested function calls (deep chain) ---
def inc(x):
    return x + 1
def dbl(x):
    return x * 2
def dec(x):
    return x - 1
print(inc(dbl(dec(inc(5)))))
print(dbl(inc(dec(dbl(inc(10))))))

# --- function returning complex expression ---
def calc(a, b, c):
    return a * b + c // a - b % c + a
print(calc(10, 5, 3))
print(calc(2, 10, 7))
print(calc(7, 3, 10))

# --- function with local var that shadows param ---
def shadow(a):
    a = 99
    return a
print(shadow(5))
print(shadow(42))

# --- function using variable from outer scope (read-only global) ---
g = 100
def use_global():
    return g + 5
print(use_global())

# --- function returning None explicitly ---
def ret_none():
    return None
print(ret_none())

# --- function with empty body (just return) ---
def empty():
    return 0
print(empty())

# =============================================
# COMPLEX EXPRESSIONS
# =============================================

# --- deep arithmetic trees ---
print(1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10)
print(1 * 2 * 3 * 4 * 5)
print(100 - 10 - 5 - 3 - 2 - 1)
print(100 // 2 // 5 // 2)
print(100 % 7 % 3)
print(1 + 2 * 3 + 4 * 5 + 6 * 7)
print((1 + 2) * (3 + 4) * (5 + 6))
print(((1 + 2) * (3 - 4)) // ((5 % 3) + 1))

# --- unary minus in expressions ---
print(-1 + -2)
print(-(1 + 2))
print(-5 * -3)
print(-10 // -3)
print(-10 % -3)
print(1 - -2)
print(1 + -2 * 3)
print(-(1 + 2) * -(3 + 4))

# --- multiple not ---
print(not not True)
print(not not False)
print(not not not True)
print(not not not False)
print(not not 1)
print(not not 0)
print(not not not 0)
print(not not not "")

# --- not with and/or ---
print(not (True and False))
print(not (True or False))
print(not (1 and 0) or 5)
print(not (0 or "") and "x")
print(not not (1 and 2) or 0)
print(1 and not 0 or not 1 and 0)
print(0 or not 1 and not 0 or 5)

# =============================================
# STRING EDGE CASES
# =============================================

# --- string concatenation chaining ---
s = "a" + "b" + "c"
print(s)
print("x" + "y" + "z")
print("" + "a" + "")
print("a" + "" + "b")
s = "hello"
t = " "
u = "world"
print(s + t + u)
print(s + "," + t + u + "!")

# --- string in/not in with variables ---
s = "hello world"
print("he" in s)
print("wo" in s)
print("xyz" in s)
print("he" not in s)
print("xyz" not in s)

# --- string comparison with identity check ---
s = "hello"
t = "hello"
print(s == t)
print(s != t)
print(s < t)
print(s > t)

# --- empty string comparisons ---
print("" < "a")
print("" > "z")
print("" == "")
print("" != "")
print("" <= "")
print("" >= "")

# --- string subscript edge: one-char string ---
s = "x"
print(s[0])
print(s[-1])

# --- string subscript with len-1 ---
s = "hello"
print(s[len(s) - 1])
print(s[len(s) - 2])

# =============================================
# LIST EDGE CASES
# =============================================

# --- list with all types ---
a = [1, "two", True, None, 3.14, []]
print(a[0])
print(a[1])
print(a[2])
print(a[3])
print(a[4])
print(a[5])
print(len(a))

# --- list subscript with computed index ---
a = [10, 20, 30, 40, 50]
print(a[len(a) - 1])
print(a[len(a) - 2])
i = 2
print(a[i])
print(a[i + 1])
print(a[i - 1])

# --- list in expression with computed index ---
print([1, 2, 3, 4, 5][1 + 2])
print([10, 20, 30, 40][len([1, 2])])
print([[1, 2], [3, 4, 5], [6]][1][len([1])])

# --- list membership with computed value ---
print(1 + 2 in [3, 4, 5])
print(10 - 5 in [5, 10, 15])
print(2 * 3 in [4, 5, 6])

# --- len in expressions ---
print(len([1, 2, 3]) + len("abc"))
print(len([]) + len("") + len([1]))

# --- append with variable ---
a = []
x = 42
a.append(x)
print(a[0])
y = "hello"
a.append(y)
print(a[1])

# --- pop on single element list ---
a = [99]
a.pop()
print(len(a))
a.append(1)
a.append(2)
a.pop()
print(len(a))

# -------------------------------------------------
# LOOP EDGE CASES
# -------------------------------------------------

# --- for with range() no args (empty) ---
for i in range():
    print("never")
print("range empty done")

# --- for variable used after loop ---
for i in range(3):
    print(i)
print(i)
print("var after for done")

# --- for with range and expression arguments ---
for i in range(2 + 3):
    print(i)
print("for expr range done")

# --- while with complex condition ---
a = [1, 2, 3, 4, 5]
i = 0
s = 0
while i < len(a) and a[i] % 2 == 0:
    s = s + a[i]
    i = i + 1
print(s)
print("while complex cond done")

# --- break in nested for ---
for i in range(3):
    for j in range(5):
        if j == 2:
            break
        print(i, j)
print("nest for break done")

# --- continue in nested for ---
for i in range(2):
    for j in range(4):
        if j == 1:
            continue
        print(i, j)
print("nest for continue done")

# =============================================
# PRINT EDGE CASES
# =============================================

# --- print with expressions of all types ---
print(1, "two", True, None, 3.14, [1, 2])
print(1 + 2, "a" + "b", not False, len([1, 2, 3]))

# --- print with no args (blank line) ---
print("before")
print()
print("after")

# --- print single arg of each type ---
print(42)
print(3.14)
print("str")
print(True)
print(False)
print(None)
print([1, 2, 3])

# =============================================
# VARIABLE EDGE CASES
# =============================================

# --- variable reassignment with self-reference ---
x = 1
x = x + x
print(x)
x = x * x
print(x)
x = x + x + x
print(x)

# --- many variables in function ---
def many_vars():
    a = 1
    b = 2
    c = 3
    d = 4
    e = 5
    f = 6
    g = 7
    h = 8
    i = 9
    j = 10
    k = 11
    l = 12
    m = 13
    n = 14
    o = 15
    return a + b + c + d + e + f + g + h + i + j + k + l + m + n + o
print(many_vars())

# --- shadowing local with same name as global ---
val = 100
def shadow_test():
    val = 200
    return val
print(shadow_test())
print(val)

# =============================================
# NONE / BOOL EDGE CASES
# =============================================

# --- None in list comparisons ---
print(None in [None])
print(None in [1, 2])
print(None == None)
print(None != None)

# --- None in function return ---
def returns_none():
    return
print(returns_none())

# --- bool in comparisons (cross-type) ---
print(True == True)
print(False == False)
print(True != False)
print(False != True)

# --- bool comparison with int (prints badcmp, continues) ---
print(True == 1)
print(False == 0)
print(True != 2)
print(1 == True)
print(0 == False)

# =============================================
# INTEGER / FLOAT EDGE CASES
# =============================================

# --- large integer arithmetic ---
print(1000000 * 1000000)
print(1000000000 * 1000)
print(999999999 + 999999999)
print(1000000000000 - 1)
print(1234567890 // 12345)

# --- float comparison ---
print(1.5 == 1.5)
print(1.5 != 2.5)
print(0.0 == 0.0)
print(-0.5 == -0.5)
print(0.1 + 0.2)    # floating point
print(1.0 == 1)
print(1 == 1.0)
print(0.0 == 0)
print(0 == 0.0)

# --- negative float edge ---
print(-0.0)
print(-0.5 // 1)
print(-0.5 % 1)
print(0.5 // -1)
print(0.5 % -1)

# =============================================
# MIXED FEATURES
# =============================================

# --- function calls in loop condition ---
def is_pos(x):
    return x > 0
i = -2
while is_pos(i):
    print("should not print")
    i = i + 1
print("while func cond done")

i = 3
while is_pos(i):
    print(i)
    i = i - 1
print("while func cond 2 done")

# --- for loop with range using function ---
def double_range(n):
    s = 0
    for i in range(n):
        s = s + i * 2
    return s
print(double_range(5))
print(double_range(10))
print(double_range(0))

# --- multiple function calls in one expression ---
def add(x, y):
    return x + y
def mul(x, y):
    return x * y
print(add(5, mul(2, 3)))
print(mul(add(1, 2), add(3, 4)))
print(add(add(1, 2), add(3, 4)))



# =============================================
# OK MARKER -- all non-stopping tests above
# =============================================
print("=== t11_more OK ===")

# =============================================
# ERROR TESTS BELOW (runtime errors that stop execution)
# =============================================
