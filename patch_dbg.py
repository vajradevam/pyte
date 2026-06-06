import sys

with open('pyte.c') as f:
    src = f.read()

# Add debug prints before the main switch for key opcodes
# Find "switch (op) {" in vm_exec
target = "        switch (op) {"
idx = src.find(target)
if idx < 0:
    print("not found")
    sys.exit(1)

debug = '''        switch (op) {
        case OP_GLOAD: case OP_GSTORE: case OP_STORE: case OP_LOAD:
        case OP_GET: case OP_SET:
        case OP_INT: case OP_FORITER: case OP_RANGE:
            fprintf(stderr,"  OP_%d pc=%d sp=%ld\\n",op,pc-1,sp-stack);
            break;
        default:
            break;
        }
        switch (op) {'''

src = src[:idx] + debug + src[idx+len(target):]

with open('pyte_dbg.c', 'w') as f:
    f.write(src)

print("patched")
