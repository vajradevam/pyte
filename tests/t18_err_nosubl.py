# Test: nosubl - subscript set on non-list (fatal)
print("=== t18_err_nosubl ===")
print("=== t18_err_nosubl OK ===")
x = 42
x[0] = 1
print("survived")
# Should not reach here
