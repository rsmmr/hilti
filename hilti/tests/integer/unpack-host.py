# $Id$
#
# @TEST-EXEC:  python %INPUT >tmp.hlt
# @TEST-EXEC:  hilti-build tmp.hlt -o a.out 
# @TEST-EXEC:  ./a.out >output 2>&1
# @TEST-EXEC:  test-diff output
#
# Dynamically generate a script for testing with host byte order.

import sys
import struct

# '=i' is standardized to refer to an int32.
packed = repr(struct.pack("=i", 0x01020304))[1:-1]

print """
module Main

import hilti

void run() {
    local ref<bytes> b
    local iterator<bytes> p1
    local iterator<bytes> p2
    local iterator<bytes> p3
    local tuple<int32, iterator<bytes>> t32
    local int32 i32
    local int32 diff
    local string out
    
    b = b\"%s\"
    p1 = begin b
    p2 = end b
    t32 = unpack (p1,p2) Hilti::Packed::Int32
    i32 = tuple.index t32 0
    p3 = tuple.index t32 1
    diff = bytes.diff p1 p3
    out = call Hilti::fmt ("hex=0x%%x dec=%%d diff=%%d", (i32, i32, diff))
    call Hilti::print(out)
    
}
""" % packed
