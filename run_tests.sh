#!/bin/sh
# pyte test runner
# Usage: ./run_tests.sh [test_file_pattern]
#   default: runs all t*.py files

PYR=./pyte
TDIR=tests

if [ ! -x "$PYR" ]; then
    echo "Building pyte..."
    gcc -O2 -Wall -Wextra -o pyte pyte.c 2>&1 || exit 1
fi

if [ ! -x "$PYR" ]; then
    echo "Error: pyte not found and could not be built"
    exit 1
fi

if [ $# -ge 1 ]; then
    # If user gives paths, use as-is; otherwise prefix with tests/
    TESTS=""
    for arg in "$@"; do
        if [ -f "$arg" ]; then
            TESTS="$TESTS $arg"
        elif [ -f "$TDIR/$arg" ]; then
            TESTS="$TESTS $TDIR/$arg"
        else
            echo "  [SKIP] $arg (not found)"
        fi
    done
else
    TESTS="$TDIR/t00_basic.py $TDIR/t01_strings.py $TDIR/t02_control.py $TDIR/t03_functions.py $TDIR/t04_lists.py $TDIR/t05_booleans.py $TDIR/t06_truthiness.py $TDIR/t07_integration.py $TDIR/t08_comments.py $TDIR/t09_forlist.py $TDIR/t10_edge.py $TDIR/t11_string_methods.py"
fi

all_pass=1
pass=0
fail=0
fail_files=""

for t in $TESTS; do
    short=$(basename "$t")
    if [ ! -f "$t" ]; then
        echo "  [SKIP] $short (not found)"
        continue
    fi
    printf "  Running %-20s ... " "$short"
    out=$($PYR "$t" 2>&1)
    result=$?
    first_err=$(echo "$out" | grep -m1 '^Error:')
    last_line=$(echo "$out" | tail -1)
    ok_msg=$(echo "$out" | grep -m1 '=== .* OK ===')
    if [ $result -eq 0 ] && [ -n "$ok_msg" ] && [ -z "$first_err" ]; then
        echo "PASS"
        pass=$((pass + 1))
    else
        echo "FAIL (exit=$result)"
        if [ -n "$first_err" ]; then
            echo "  ERROR: $first_err"
        fi
        echo "--- last output lines ---"
        echo "$out" | tail -10
        echo "---"
        all_pass=0
        fail=$((fail + 1))
        fail_files="$fail_files $t"
    fi
done

echo ""
echo "=========================================="
if [ $all_pass -eq 1 ]; then
    echo "  All $pass test(s) passed!"
    exit 0
else
    echo "  $pass passed, $fail failed"
    echo "  Failed: $fail_files"
    exit 1
fi
