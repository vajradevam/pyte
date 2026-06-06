# Test: lstidx - list subscript set out of bounds (fatal)
print("=== t18_err_lstidx_set ===")
print("=== t18_err_lstidx_set OK ===")
a = [1, 2, 3]
a[10] = 99
print("survived")
# Should not reach here
