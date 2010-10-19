# $Id$
#
# Platform specific code is outsourced into this file.
#
# Documentation pointers:
#     
#    Darwin ABI   http://developer.apple.com/documentation/DeveloperTools/Conceptual/LowLevelABI/000-Introduction/introduction.html
#    x86_64       http://www.x86-64.org/documentation.html

import platform
import sys

import llvm
import llvm.core 
import llvm.ee

_triple = llvm.core.getHostTriple().split("-")
_arch = _triple[0]
_vendor = _triple[1]
_os = "-".join(_triple[2:])
_target = "%s-%s" % (_arch, _os)
# This is a list of platform for which the code in this file has been adapted.
# If the system we are running on is not in here, we'll likeley have a problem
# and print a warning now and abort later if necessary. We also recognize the
# platforms if they OS parts if followed by a version number.

_knownTargets = [
    "i386-darwin",
    "x86_64-darwin",
    "i386-linux",
    "x86_64-linux"
    ]

for t in _knownTargets:
    if _target.startswith(t):
        break

else:
    print >>sys.stderr, "HILTI warning: running on a non-supported _target '%s', may abort later. Please report this." % _target

def isLittleEndian():
    """Returns True if we are running on a little-endian system.

    Returns: bool - True if it's a little-endian system.
    """

    if _arch == "i386":
        return True

    if _arch == "x86_64":
        return True

    util.error("isLittleEndian() does not support _target %s" % _target, component="system.py")

_structSizeCache = {}

def _targetArch():
    """Return the target architecture. It will be a string of the form
    ``<arch>-<os>``, e.g., ``i386-darwin10``.

    Returns: string - The _target _target.
    """
    return _target

def returnStructByValue(type):
    """Checks whether the host platform's ABI returns a struct type by value.
    If not, we assume that the struct must be passed via a pointer given as
    the (hidden) *first* parameter to a function.

    *type*: llvm.core.Type.struct - The struct for which one wants to know
    whether it is returned by value.

    Returns: llvm.core.Type or None - If a type, the struct is returned by
    value and the integer type specifies the type of the return value that the
    ABI uses for returning such structs. If None, the struct is not returned
    by value.
    """

    try:
        sizeof = _structSizeCache[str(type)]
    except KeyError:
        # This is a bit silly. We want the number of bytes the structure requires
        # for storing. However, there's no direct way of getting that so we build a
        # LLVM function which we then execute JIT ...
        #
        module = llvm.core.Module.new("sizeof")
        # The _target doesn't seem to be initalized correctly by default.
        module._target = ""
        ft = llvm.core.Type.function(llvm.core.Type.int(32), [])
        func = llvm.core.Function.new(module, ft, "sizeof")
        block = func.append_basic_block ("")
        builder = llvm.core.Builder.new(block)
        builder.ret(llvm.core.Constant.sizeof(type))

        ee = llvm.ee.ExecutionEngine.new(module)
        sizeof = ee.run_function(func, []).as_int()

        _structSizeCache[str(type)] = sizeof

    if _target.startswith("i386-linux"):
        # Can't find documentation but looking at clang-cc, it seems to always
        # pass structs via temporary memory objects.
        return None
    
    if _target.startswith("i386-darwin"):
        if sizeof in (1, 2, 4, 8):
            return llvm.core.Type.int(sizeof * 8)
        return None

    if _target.startswith("x86_64-linux") or _target.startswith("x86_64-darwin"):
        if sizeof == 1:
            return llvm.core.Type.int(8)

        if sizeof == 2:
            return llvm.core.Type.int(16)

        if sizeof <= 4:
            return llvm.core.Type.int(32)

        if sizeof <= 8:
            return llvm.core.Type.int(64)

        if sizeof <= 16:
            # FIXME: We special case here: if we have a tuple<double,ptr>, we
            # explictly return the right type. This is just a hack to make
            # this type work. Seems our heuristic isn't sufficient to handle
            # it yet. What a pain ...
            if isinstance(type, llvm.core.StructType) and len(type.elements) == 2:
                if type.elements[0] == llvm.core.Type.double() and \
                   isinstance(type.elements[1], llvm.core.PointerType):
                       return llvm.core.Type.struct([llvm.core.Type.double(), llvm.core.Type.int(64)])
            
            return llvm.core.Type.int(128)

        return None

    util.error("returnsStructByValue() does not support _target %s" % _target, component="system.py")
