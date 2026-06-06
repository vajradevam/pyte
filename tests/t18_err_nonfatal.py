# Test: non-fatal runtime error paths (error prints, returns None, execution continues)
# These errors set vm_err_flag and return v_none() via add_val/sub_val/mul_val/div_val/mod_val/cmp_val/neg_val
print("=== t18_err_nonfatal ===")

# --- bad+ : unsupported addition types ---
x = None + 1
print(x)
print("after bad+")

# --- bad- : unsupported subtraction ---
x = None - 1
print(x)
print("after bad- 1")
x = "abc" - "b"
print(x)
print("after bad- 2")

# --- bad* : unsupported multiplication ---
x = "abc" * 3
print(x)
print("after bad* 1")
x = [1, 2] * 3
print(x)
print("after bad* 2")

# --- bad// : the lexer converts // to T_SLASH; div_val handles it ---

# --- div0 : division by zero ---
x = 10 // 0
print(x)
print("after div0")

# --- mod0 : modulo by zero ---
x = 10 % 0
print(x)
print("after mod0")

# --- badcmp : cross-type comparisons ---
x = "abc" == 42
print(x)
print("after badcmp 1")
x = "abc" < 42
print(x)
print("after badcmp 2")
x = True < "abc"
print(x)
print("after badcmp 3")
x = None == [1, 2]
print(x)
print("after badcmp 4")

# --- badneg : negation on non-numeric ---
x = -"abc"
print(x)
print("after badneg 1")
x = -None
print(x)
print("after badneg 2")

# --- chain multiple non-fatal errors ---
x = "abc" * 3 + None - 1
print(x)
print("after chain")

print("=== t18_err_nonfatal OK ===")
