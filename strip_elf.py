#!/usr/bin/env python3
"""Strip ELF: remove notes, comments, zero section headers, truncate."""
import struct, sys, os

path = sys.argv[1]
with open(path, 'rb') as f:
    data = bytearray(f.read())

e_shoff = struct.unpack_from('<Q', data, 0x28)[0]
struct.pack_into('<Q', data, 0x28, 0)
struct.pack_into('<H', data, 0x3C, 0)  # e_shnum
struct.pack_into('<H', data, 0x3E, 0)  # e_shstrndx

if e_shoff > 0 and e_shoff < len(data):
    data = data[:e_shoff]

with open(path, 'wb') as f:
    f.write(data)
os.chmod(path, 0o755)
