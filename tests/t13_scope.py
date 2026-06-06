# Test: global/func interaction, short-circuit side effects
print("=== t13_scope ===")

# global variable access in function
g = 42
def read_g():
    return g
def write_g():
    g = 99
    return g
print(read_g())
print(write_g())
print(g)

# short-circuit with side effects
def called():
    print("called!")
    return True

x = True or called()
print("short or ok")
x = False and called()
print("short and ok")
x = True and called()
print("eval and ok")
x = False or called()
print("eval or ok")

# for break+continue combined
for i in range(10):
    if i == 0:
        print("start")
        continue
    if i == 3:
        continue
    if i == 7:
        print("end")
        break
    print(i)
print("brk+cont done")

# early return
def has_early(x):
    if x < 0:
        return 0
    return x
print(has_early(-5))
print(has_early(42))

print("=== t13_scope OK ===")
