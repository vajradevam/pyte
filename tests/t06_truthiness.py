# Test: Truthiness in conditionals
print("=== t06_truthiness ===")

# ── int truthiness  ──
if 0:
    print("0 is truthy")
else:
    print("0 is falsy")

if 1:
    print("1 is truthy")

if -1:
    print("-1 is truthy")

if 100:
    print("100 is truthy")

if 0:
    print("0 truthy")
else:
    print("0 falsy ok")

# ── string truthiness ──
if "":
    print("empty string is truthy")
else:
    print("empty string is falsy")

if "hello":
    print("non-empty string is truthy")

if " ":
    print("space is truthy")

if "0":
    print("string '0' is truthy")

# ── list truthiness ──
if []:
    print("empty list is truthy")
else:
    print("empty list is falsy")

if [1]:
    print("non-empty list is truthy")

if [0]:
    print("list [0] is truthy")

# ── None truthiness ──
if None:
    print("None is truthy")
else:
    print("None is falsy")

# ── bool truthiness ──
if True:
    print("True is truthy")

if False:
    print("False is truthy")
else:
    print("False is falsy")

# ── in conditions with truthiness ──
if 1 in [1, 2, 3] and 4 not in [1, 2, 3]:
    print("combined condition")

if not (0 in [1, 2, 3]):
    print("0 not in list is True")

# ── truthiness with not ──
if not 0:
    print("not 0 is True")

if not 1:
    print("not 1 is True")
else:
    print("not 1 is False")

if not "":
    print('not "" is True')

if not "x":
    print('not "x" is True')
else:
    print('not "x" is False')

print("=== t06_truthiness OK ===")
