# Test: noapp - append on non-list (fatal)
print("=== t18_err_noapp ===")
print("=== t18_err_noapp OK ===")
x = 42
x.append(1)
print("survived")
# Should not reach here
