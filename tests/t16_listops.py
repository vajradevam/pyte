# Test: list operations, negative index, computed subscript
print("=== t16_listops ===")

# negative index assignment
a = [10, 20, 30, 40]
a[-1] = 99
print(a)
a[-2] = a[-1] * 2
print(a)

# list with computed index
a = [1, 2, 3, 4, 5]
print(a[1 + 2])
print(a[4 - 1])
print(a[len(a) - 1])

# computed index assignment
a = [10, 20, 30, 40, 50]
i = 1
j = 3
a[i] = a[j]
a[i + 1] = a[j - 1]
print(a)

# list membership different types
print(1 in [1.0, 2.0])
print(1.0 in [1, 2])
print("a" in ["a", 1, None])
print(True in [True, False])
print(None in [1, None, "x"])
print([] in [1, [], 3])

# len in expressions
print(len("abc") + len([1, 2, 3, 4]))
print(len([]) + len("") + len([1]))
print(len("hello") > 3 and len([1, 2]) < 5)

# string concat with empty strings
print("a" + "" + "b")
print("" + "" + "")
print("x" + "")

# string with backslash
print("test\\")
print("x\\ny")
print("a\\tb")

print("=== t16_listops OK ===")
