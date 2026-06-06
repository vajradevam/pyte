# Test: Edge cases for every feature (non-error tests first)
print("=== t09_edges ===")

# =============================================
# ARITHMETIC EDGE CASES
# =============================================

# --- division by zero (prints error, returns None, continues) ---
print(1 // 0)
print(0 // 0)
print(1 % 0)
print(0 % 0)

# --- float division by zero ---
print(1.0 // 0.0)
print(1.0 // 0)

# --- division rounding (Python floor division) ---
print(10 // 3)
print(-10 // 3)
print(10 // -3)
print(-10 // -3)

# --- modulo with negatives ---
print(10 % 3)
print(-10 % 3)
print(10 % -3)
print(-10 % -3)

# --- mixed float/int division ---
print(10.0 // 3)
print(10 // 3.0)
print(-10.0 // 3)
print(10 // -3.0)

# --- mixed float/int modulo ---
print(10.0 % 3)
print(10 % 3.0)

# --- float arithmetic edge cases ---
print(0.5 + 0.5)
print(1.5 - 0.5)
print(2.0 * 0.5)
print(10.0 // 2.5)
print(10.0 % 2.5)

# --- unary minus edge ---
print(-0)
print(-0.0)
print(-(1 + 2))
print(-(3.5 * 2))
print(-(-(-5)))

# --- complex expressions ---
print(1 + 2 * 3 - 4 // 2 + 5 % 3)
print((1 + 2) * (3 - 4) // (2 + 5 % 3))
print(2 * 3 + 4 * 5 - 6 // 2 + 7 % 3 - 8 + 9 * 10 // 5)

# --- large integers ---
print(999999999)
print(-999999999)
print(1000000 * 1000000)
print(1000000000 // 500000000)
print(1 + 999999999)

# --- zero arithmetic ---
print(0 + 0)
print(0 - 0)
print(0 * 0)
print(0 // 1)
print(0 % 1)
print(0 * 1000000)
print(1000000 * 0)
print(0 // 1000000)

# --- one arithmetic ---
print(1 + 1)
print(1 - 1)
print(1 * 1)
print(1 // 1)
print(1 % 1)
print(1 * 0)
print(0 * 1)
print(1 // 2)
print(1 // -2)

# =============================================
# COMPARISON EDGE CASES
# =============================================

# --- bool-int comparisons ---
print(1 < True)
print(1 == True)
print(0 == False)
print(1 != True)
print(True < 2)
print(True > 0)
print(False < 1)
print(True >= 0)
print(True <= 1)

# --- None comparisons ---
print(None == None)
print(None != None)
print(None == 0)
print(None != 0)
print(None == "")
print(None != "")
print(None == False)
print(None != True)
print(None == [])
print(None != [])
print(0 == None)
print("" != None)
print(False == None)

# --- bool-bool comparisons ---
print(True == True)
print(True == False)
print(False == False)
print(True != False)
print(True < False)
print(False < True)
print(True > False)
print(True >= True)
print(False <= True)
print(True <= True)

# --- string comparisons ---
print("" == "")
print("" != "")
print("" < "a")
print("a" < "aa")
print("ab" < "abc")
print("" > "")
print("" >= "")
print("" <= "")

# --- float-int comparisons ---
print(True == 1)
print(True != 2)
print(False == 0)
print(False != -1)
print(1.0 == 1)
print(1.0 != 2)
print(0.0 == 0)
print(0.5 == 0.5)
print(0.5 != 0)

# =============================================
# STRING EDGE CASES
# =============================================

# --- empty string ---
print(len(""))
print("" == "")
print("" != "x")
print("" < "x")
print("x" > "")
print("" in "abc")
print("" in "")
print("" not in "abc")
print("" not in "")

# --- string subscript (valid indices only) ---
s = "hello"
print(s[0])
print(s[4])
print(s[-1])
print(s[-5])

# --- string subscript with computed index ---
s = "abcde"
i = 2
print(s[i])
print(s[i + 1])
print(s[-i])

# --- string comparison edge cases ---
print("a" == "a")
print("a" != "A")
print("hello" < "hellp")
print("hello" > "hell")
print("0123456789" == "0123456789")
print(" " == " ")

# --- string concatenation ---
print("a" + "b")
print("" + "x")
print("x" + "")
print("hello " + "world")
s = "abc"
t = "def"
print(s + t)
print(s + "" + t)
print("" + "")

# --- string in/not in edge ---
print("a" in "a")
print("a" in "ba")
print("abc" in "abc")
print("abc" in "abcabc")
print("a" in "")
print("" in "a")
print("x" not in "")
print("" not in "a")

# --- string with escaped characters ---
print("hello\nworld")
print("tab\there")
print("back\\slash")
print("quote\"here")
print("single'quote")
print("newline\nand\ttab")

# --- long string ---
print("abcdefghijklmnopqrstuvwxyz")

# =============================================
# LIST EDGE CASES
# =============================================

# --- empty list ---
a = []
print(len(a))
print(1 in [])
print(1 not in [])
print([] == [])
print([] != [])

# --- single element list ---
a = [42]
print(a[0])
print(a[-1])
print(len(a))
print(42 in a)
print(99 not in a)

# --- list subscript (valid indices only) ---
a = [10, 20, 30]
print(a[0])
print(a[2])
print(a[-1])
print(a[-3])

# --- subscript assignment ---
a = [1, 2, 3]
a[0] = 99
print(a[0])
a[-1] = 77
print(a[-1])
a[1] = a[0] - a[-1]
print(a[1])

# --- negative index assignment ---
a = [10, 20, 30, 40]
a[-1] = 99
print(a[-1])
a[-2] = a[-2] * 2
print(a[-2])

# --- append as statement ---
a = []
a.append(1)
a.append(2)
a.append(3)
print(len(a))
print(a[0])
print(a[1])
print(a[2])

# --- pop as statement (no return value) ---
a = [10, 20, 30]
a.pop()
print(len(a))
print(a[0])
print(a[1])
a.pop()
print(len(a))
print(a[0])

# --- append/pop cycle ---
a = []
a.append(1)
a.pop()
a.append(2)
a.append(3)
a.pop()
a.append(4)
print(len(a))
print(a[0])

# --- list with bool and None ---
a = [True, False, None, 0, 1]
print(a[0])
print(a[1])
print(a[2])
print(len(a))
print(True in a)
print(False in a)
print(None in a)
print(42 not in a)

# --- list membership types ---
print(1 in [1, 2, 3])
print(0 in [1, 2, 3])
print("a" in ["a", "b"])
print(True in [True, False])
print(None in [None, 1])
print(1.0 in [1.0, 2.0])
print(1 in [1.0, 2.0])
print(1.0 in [1, 2])

# --- nested list ---
a = [1, [2, 3], 4]
print(a[0])
print(a[1][0])
print(a[1][1])
print(len(a))

# --- list in expression ---
print([1, 2, 3][1])
print([10, 20][0] + [30, 40][1])
print([1 + 2, 3 * 4, 10 - 5][2])
print([[1, 2], [3, 4]][0][1])

# --- len on various lists ---
print(len([]))
print(len([1]))
print(len([1, 2, 3, 4, 5]))
print(len([[1, 2], [3, 4, 5]]))

# =============================================
# BOOLEAN / LOGIC EDGE CASES
# =============================================

# --- and/or truth table ---
print(True and True)
print(True and False)
print(False and True)
print(False and False)
print(True or True)
print(True or False)
print(False or True)
print(False or False)

# --- and/or with truthy/falsy ---
print(0 and 5)
print(1 and 5)
print(0 or 5)
print(1 or 5)
print(0 and 0)
print(0 or 0)
print(1 and 0)
print(1 or 0)

# --- short-circuit: and ---
print(0 and 1 // 0)  # short-circuits div0
print(0 and 1 % 0)   # short-circuits mod0

# --- short-circuit: or ---
print(1 or 1 // 0)   # short-circuits div0
print(1 or 1 % 0)

# --- not on all types ---
print(not True)
print(not False)
print(not 0)
print(not 1)
print(not -1)
print(not "")
print(not "x")
print(not [])
print(not [1])
print(not None)
print(not 0.0)
print(not 0.5)

# --- chained and/or ---
print(0 or 0 or 5)
print(0 or 1 or 5)
print(1 and 2 and 3)
print(0 and 1 and 2)
print(1 and 0 and 2)
print(0 or "" or "last")
print(1 or 2 and 3)
print(0 and 1 or 2)
print(1 and 0 or 3)

# --- not with and/or ---
print(not True and False)
print(not False or True)
print(not (True and False))
print(not (True or False))

# =============================================
# FUNCTION EDGE CASES
# =============================================

# --- implicit None return ---
def no_ret():
    x = 1
print(no_ret())

# --- early return ---
def early(x):
    if x < 0:
        return 0
    if x > 100:
        return 100
    return x
print(early(-5))
print(early(50))
print(early(200))

# --- return in if/else branches ---
def abs_val(x):
    if x >= 0:
        return x
    return -x
print(abs_val(5))
print(abs_val(0))
print(abs_val(-3))

# --- function returning list ---
def make_list():
    return [1, 2, 3]
print(make_list()[0])
print(len(make_list()))

# --- function returning string ---
def greet(name):
    return "Hello, " + name
print(greet("world"))

# --- function modifying list in place ---
def add_one(lst):
    lst.append(1)
a = []
add_one(a)
add_one(a)
print(len(a))
print(a[0])

# --- function with many params ---
def sum4(a, b, c, d):
    return a + b + c + d
print(sum4(1, 2, 3, 4))
print(sum4(10, 20, 30, 40))
print(sum4(-1, 0, 1, 2))

# --- function calling another ---
def square(x):
    return x * x
def sum_sq(a, b):
    return square(a) + square(b)
print(sum_sq(3, 4))
print(sum_sq(0, 5))

# --- recursion ---
def fact(n):
    if n <= 1:
        return 1
    return n * fact(n-1)
print(fact(0))
print(fact(1))
print(fact(5))
print(fact(7))

# --- mutual recursion ---
def even(n):
    if n == 0:
        return True
    return odd(n-1)
def odd(n):
    if n == 0:
        return False
    return even(n-1)
print(even(0))
print(even(4))
print(odd(1))
print(odd(5))

# --- local shadowing global ---
x = 42
def f():
    x = 10
    return x
print(f())
print(x)

# --- conditional def ---
if 1:
    def cond_func():
        return "true branch"
else:
    def cond_func():
        return "false branch"
print(cond_func())

if 0:
    def cond_func2():
        return "true branch 2"
else:
    def cond_func2():
        return "false branch 2"
print(cond_func2())

# =============================================
# LOOP EDGE CASES
# =============================================

# --- for range(0) ---
for i in range(0):
    print("never")
print("range0 done")

# --- for range(0, 0) ---
for i in range(0, 0):
    print("never")
print("range00 done")

# --- for range(5, 2) (start > stop) ---
for i in range(5, 2):
    print("never")
print("range down done")

# --- for range(1) ---
for i in range(1):
    print(i)
print("range1 done")

# --- for range(2, 5) ---
for i in range(2, 5):
    print(i)
print("range2to5 done")

# --- for with break first ---
for i in range(100):
    break
    print("never")
print("for break first done")

# --- for with continue first ---
for i in range(5):
    continue
    print("never")
print("for cont first done")

# --- for with break mid ---
for i in range(10):
    if i == 4:
        break
    if i == 2:
        print("mid")
print("for break mid done")

# --- for with continue mid ---
for i in range(5):
    if i == 2:
        continue
    print(i)
print("for cont mid done")

# --- while with zero iterations ---
while 0:
    print("never")
print("while0 done")

# --- while finite ---
i = 0
while i < 3:
    print(i)
    i = i + 1
print("while finite done")

# --- while break first ---
i = 0
while i < 100:
    break
    i = i + 1
print("while break first done")

# --- while continue first ---
i = 0
while i < 5:
    i = i + 1
    continue
    print("never")
print("while cont first done")

# --- while break mid ---
i = 0
while i < 100:
    if i == 3:
        break
    print(i)
    i = i + 1
print("while break mid done")

# --- nested loops ---
i = 0
while i < 3:
    for j in range(2):
        print(i, j)
    i = i + 1
print("nested done")

# --- nested break (inner only) ---
i = 0
while i < 3:
    j = 0
    while j < 5:
        if j == 1:
            break
        j = j + 1
    i = i + 1
print("nested break inner done")

# --- nested break both ---
i = 0
while i < 5:
    j = 0
    while j < 5:
        if j == 1:
            break
        j = j + 1
    if i == 2:
        break
    i = i + 1
print("nested break both done")

# --- for with variable range ---
n = 5
for i in range(n):
    print(i)
print("for var range done")

# --- for with expression range ---
for i in range(2 + 3):
    print(i)
print("for expr range done")

# =============================================
# VARIABLE EDGE CASES
# =============================================

# --- local variable reuse ---
def f():
    x = 1
    x = x + 1
    x = x * 2
    return x
print(f())

# --- many local variables ---
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
    return a + b + c + d + e + f + g + h + i + j
print(many_vars())

# --- many global variables ---
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

# --- cross-type reassignment ---
x = 42
print(x)
x = "hello"
print(x)
x = [1, 2, 3]
print(x[1])
x = True
print(x)
x = None
print(x)
x = 3.14
print(x)

# --- long identifier ---
a_very_long_identifier_name_test = 99
print(a_very_long_identifier_name_test)
another_long_one_xyz = a_very_long_identifier_name_test + 1
print(another_long_one_xyz)

# =============================================
# CONTROL FLOW EDGE CASES
# =============================================

# --- if without else (true) ---
x = 5
if x > 0:
    print("if true")
print("if no else done")

# --- if without else (false) ---
if 0:
    print("never")
print("if false no else done")

# --- elif without else (first true) ---
x = 5
if x > 10:
    print(">10")
elif x > 2:
    print(">2")
print("elif chain no else done")

# --- elif without else (all false) ---
x = 0
if x > 10:
    print(">10")
elif x > 5:
    print(">5")
elif x > 2:
    print(">2")
print("elif all false done")

# --- deeply nested if (depth 4) ---
x = 5
if x > 0:
    if x > 1:
        if x > 2:
            if x > 3:
                print("deep4 true")
            else:
                print("deep4 else3")
        else:
            print("deep4 else2")
    else:
        print("deep4 else1")
else:
    print("deep4 else0")

# --- if in while ---
i = 0
s = 0
while i < 5:
    if i % 2 == 0:
        s = s + i
    i = i + 1
print(s)
print("if in while done")

# --- for with if inside ---
for i in range(5):
    if i == 0:
        print("first")
    elif i == 4:
        print("last")
    else:
        print(i)
print("for with if done")

# =============================================
# PRINT EDGE CASES
# =============================================

# --- print with multiple args ---
print(1, 2, 3)
print("a", "b", "c")
print(1, "two", 3.0, True, None)
print()
print("single")
print(True, False, None, 0, "", [])

# --- print with expressions ---
print(1 + 2, 3 * 4, 10 - 5)
print([1, 2][0], "hello"[-1], len([1, 2, 3]))

# =============================================
# RANGE EDGE CASES
# =============================================

# --- range with negative ---
for i in range(-1):
    print("neg range")
print("neg range done")

# --- range with expression ---
n = 3
for i in range(n):
    print(i)
print("range expr done")

# --- range with 1 ---
for i in range(1):
    print(i)
print("range1 done")

# --- range with large start/stop ---
for i in range(10, 15):
    print(i)
print("range10to15 done")

# =============================================
# MIXED / COMPLEX EDGE CASES
# =============================================

# --- string comparison lengths ---
print("" < "a")
print("a" < "aa")
print("abc" == "abc")
print("abcd" > "abc")
print("abc" < "abd")

# --- list comparison ---
print([] == [])
print([] != [1])
print([1, 2] == [1, 2])
print([1, 2] != [1, 3])
print([1, 2, 3] == [1, 2, 3])
print([1, 2, 3] != [1, 2])

# --- None ops ---
print(None == None)
print(None != None)
print(None == 0)
print(0 == None)
print(None == "")
print(None == False)
print(True != None)
print(None == [])



# --- complex list operations ---
a = [1, 2, 3]
a[0] = a[1] + a[2]
print(a[0])
a[1] = a[0] * 2
print(a[1])

# --- for with range(len) ---
a = [10, 20, 30, 40]
for i in range(len(a)):
    print(a[i])
print("for range len done")

# --- build list with loop ---
a = []
for i in range(5):
    a.append(i)
print(len(a))
print(a[0])
print(a[4])
print("build list done")

# --- function using for ---
def sum_range(n):
    s = 0
    for i in range(n):
        s = s + i
    return s
print(sum_range(0))
print(sum_range(1))
print(sum_range(5))
print(sum_range(10))

# --- function using while ---
def count_to(n):
    i = 0
    while i < n:
        i = i + 1
    return i
print(count_to(0))
print(count_to(5))
print(count_to(100))

# --- function with nested loops ---
def mul_table(n):
    s = 0
    i = 0
    while i < n:
        for j in range(n):
            s = s + i * j
        i = i + 1
    return s
print(mul_table(3))
print(mul_table(5))

# =============================================
# OK MARKER -- all non-error tests above this line
# =============================================
print("=== t09_edges OK ===")

# =============================================
# ERROR TESTS BELOW (runtime errors that stop execution)
# These are tested one by one; if any stops, later tests
# won't run. Each error section is isolated.
# =============================================
