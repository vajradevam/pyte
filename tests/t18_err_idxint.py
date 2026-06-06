# Test: idxint - float index on subscript get (fatal)
print("=== t18_err_idxint ===")
print("=== t18_err_idxint OK ===")
a = [1, 2, 3]
x = a[1.5]
print(x)
# Should not reach here
