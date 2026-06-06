# Test: many elif chains
print("=== t14_elif ===")

# 6 elifs + else (last matches)
x = 5
if x == 0:
    print("a")
elif x == 1:
    print("b")
elif x == 2:
    print("c")
elif x == 3:
    print("d")
elif x == 4:
    print("e")
elif x == 5:
    print("f")
elif x == 6:
    print("g")
else:
    print("h")
print("elif1 done")

# 6 elifs + else (no match)
x = 99
if x == 0:
    print("a")
elif x == 1:
    print("b")
elif x == 2:
    print("c")
elif x == 3:
    print("d")
elif x == 4:
    print("e")
elif x == 5:
    print("f")
elif x == 6:
    print("g")
else:
    print("else")
print("elif2 done")

# elif without else (all match)
x = 3
if x == 0:
    print("a")
elif x == 1:
    print("b")
elif x == 2:
    print("c")
elif x == 3:
    print("d")
elif x == 4:
    print("e")
print("elif3 done")

# elif without else (no match)
x = 99
if x == 0:
    print("a")
elif x == 1:
    print("b")
elif x == 2:
    print("c")
elif x == 3:
    print("d")
elif x == 4:
    print("e")
elif x == 5:
    print("f")
elif x == 6:
    print("g")
print("elif4 done")

# first elif matches
x = 0
if x == 0:
    print("first")
elif x == 1:
    print("second")
else:
    print("other")
print("elif5 done")

# simple if/else
x = 1
if x:
    print("true")
else:
    print("false")
print("elif6 done")

print("=== t14_elif OK ===")
