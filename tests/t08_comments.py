# Test: Comments and spacing edge cases
print("=== t08_comments ===")

# ── basic comments ──
# This is a comment
x = 42  # inline comment
print(x)

# ── blank lines ──


# Above are blank lines
print("blank lines ok")

# ── indented comments ──
    # indented comment
print("indented comment ok")

# ── comment after comment ──
# first comment
# second comment
print("multiple comments ok")

# ── comment at end of file ──
print("end comments ok")
# final comment

# ── comment with special chars ──
# This has # inside (should be treated as part of comment)
x = 1
print(x)

# ── comment on same line as code with trailing whitespace ──
y = 2  # comment with trailing space    
print(y)

# ── line starting with spaces then comment ──
    # indented comment before statement
z = 3
print(z)

# ── comment after dedent ──
if True:
    x = 1
# comment after dedent
print(x)

# ── empty program (just prints) ──
print("done")

print("=== t08_comments OK ===")
