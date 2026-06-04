# Test: String operations
print("=== t01_strings ===")

# ── len ──
print(len(""))
print(len("a"))
print(len("hello"))
print(len("  spaces  "))

# ── subscript (positive) ──
print("hello"[0])
print("hello"[1])
print("hello"[4])
print("abc"[2])

# ── subscript (negative) ──
print("hello"[-1])
print("hello"[-2])
print("hello"[-5])
print("abc"[-1])
print("abc"[-3])

# ── subscript via variable ──
s = "world"
print(s[0])
print(s[1])
print(s[-1])
print(s[-2])

# ── string comparison ──
print("abc" == "abc")
print("abc" == "xyz")
print("abc" != "xyz")
print("abc" < "xyz")
print("abc" > "ab")
print("" == "")
print("" != "x")
print("a" < "b")
print("z" > "a")
print("hello" == "hello")
print("Hello" != "hello")

# ── in / not in (substring) ──
print("he" in "hello")
print("x" in "hello")
print("he" not in "hello")
print("x" not in "hello")
print("" in "abc")
print("abc" in "abc")
print("" in "")
print("abc" in "")

# ── string concatenation (via +) ──
# This is a planned future feature; skip for now
# print("a" + "b")
# print("hello " + "world")

print("=== t01_strings OK ===")

# ── string concatenation (+) ──
print("a" + "b")
print("hello " + "world")
print("" + "x")
print("x" + "")
s = "abc"
t = "def"
print(s + t)
