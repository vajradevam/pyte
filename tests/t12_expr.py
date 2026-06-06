# Test: print as expression, range as value
print("=== t12_expr ===")

# print() as expression returns None
x = print(42)
print(x)
y = print(1, 2, 3)
print(y)
z = print()
print(z)

# range as value
r = range(5)
print(r)
r2 = range(2, 6)
print(r2)

# function implicit None return
def noop():
    x = 1
print(noop())

# string concat chaining
print("a" + "b" + "c" + "d")
s = "x" + "y" + "z"
print(s)

# list pop in loop
a = [1, 2, 3, 4, 5]
while len(a) > 0:
    a.pop()
    print(len(a))
print("pop loop done")

# continue in for
for i in range(5):
    if i == 2:
        continue
    print(i)
print("cont for done")

print("=== t12_expr OK ===")
