# Test: String methods — .upper(), .lower(), .strip()
print("=== t11_string_methods ===")

# ── .upper() ──
s = "hello"
s.upper()
print(s)

s = "Hello World"
s.upper()
print(s)

s = "123abc!@#"
s.upper()
print(s)

s = ""
s.upper()
print(s)

# ── .lower() ──
s = "HELLO"
s.lower()
print(s)

s = "Hello World"
s.lower()
print(s)

s = "123ABC!@#"
s.lower()
print(s)

s = ""
s.lower()
print(s)

# ── .strip() ──
s = "  hello  "
s.strip()
print(s)

s = "\t\n  hello  \n\r"
s.strip()
print(s)

s = "hello"
s.strip()
print(s)

s = "  "
s.strip()
print(s)

s = ""
s.strip()
print(s)

s = "  a  b  "
s.strip()
print(s)

# ── chained: strip then upper ──
s = "  hello  "
s.strip()
s.upper()
print(s)

# ── upper then lower ──
s = "Mixed CASE"
s.upper()
s.lower()
print(s)

# ── mixed: actual mutation persists ──
s = "  abc  "
s.strip()
s.upper()
print(s)

print("=== t11_string_methods OK ===")
