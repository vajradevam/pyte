# Test: nested loops, break, continue
print("=== t15_nestloop ===")

# nested for with break
for i in range(3):
    for j in range(5):
        if j == 2:
            break
        print(i, j)
print("nest for break done")

# nested for with continue
for i in range(2):
    for j in range(4):
        if j == 1:
            continue
        print(i, j)
print("nest for cont done")

# while with for inside
i = 0
while i < 3:
    for j in range(2):
        print(i, j)
    i = i + 1
print("while for done")

# nested if inside for inside while
i = 0
while i < 3:
    for j in range(3):
        if i == j:
            print("diag", i)
        elif i < j:
            print("low", i, j)
        else:
            print("high", i, j)
    i = i + 1
print("nest all done")

# list append in loop
src = [10, 20, 30, 40]
dst = []
for i in range(len(src)):
    dst.append(src[i])
print(len(dst))
print(dst[0])
print(dst[3])
print("copy done")

print("=== t15_nestloop OK ===")
