# Test: lstidx - list subscript get out of bounds (fatal)
print("=== t18_err_lstidx_get ===")
print("=== t18_err_lstidx_get OK ===")
a = [1, 2, 3]
x = a[10]
print(x)
# Should not reach here
