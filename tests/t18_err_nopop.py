# Test: nopop - pop on non-list (fatal)
print("=== t18_err_nopop ===")
print("=== t18_err_nopop OK ===")
x = 42
x.pop()
print("survived")
# Should not reach here
