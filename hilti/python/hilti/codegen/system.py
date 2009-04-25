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

from hilti.core import *

machine = platform.machine().lower()

if len(machine) == 4 and machine[0] == "i" and machine[2:] == "86":
    # Normalize i?86.
    machine = "i386"
    
system = platform.system().lower()

try:
    major_release = platform.release().split(".")[0]
except IndexError:
    print >>sys.stderr, "cannot determine OS major release number from platform.release()"

# Let's hope we don't need to consider the OS major release ...
arch = "%s-%s" % (machine, system)

# This is a list of platform for which the code in this file has been adapted. 
# If the system we are running on is not in here, we'll likeley have a problem
# and print a warning now and abort later if necessary. 

_KnownArchitectures = [ 
    "i386-darwin", 
    "i386-linux", 
    "x86_64-linux"
    ]

if arch not in _KnownArchitectures: 
    print >>sys.stderr, "HILTI warning: running on a non-supported architecture '%s', may abort later. Please report this." % arch

def isLittleEndian():
    """Returns True if we are running on a little-endian system.
    
    Returns: bool - True if it's a little-endian system.
    """
     
    if machine == "i386":
        return True
    
    if machine == "x86_64":
        return True

_structSizeCache = {}    
    
def returnStructByValue(type):
    """Checks whether the host platform's ABI return a struct type by value.
    If not, we assume that the struct must be passed via a pointer given as
    the (hidden) *first* parameter to a function. 
    
    *type*: llvm.core.Type.struct - The struct for which one wants to know
    whether it is returned by value. 
    
    Returns: bool - True if *type* is returned by value.
    """

    try:
        sizeof = _structSizeCache[str(type)]
    except KeyError:
        # This is a bit silly. We want the number of bytes the structure requires
        # for storing. However, there's no direct way of getting that so we build a
        # LLVM function which we then execute JIT ...
        #
        module = llvm.core.Module.new("sizeof")
        ft = llvm.core.Type.function(llvm.core.Type.int(32), [])
        func = llvm.core.Function.new(module, ft, "sizeof")
        block = func.append_basic_block ("")
        builder = llvm.core.Builder.new(block)
        builder.ret(llvm.core.Constant.sizeof(type))
        
        mp = llvm.core.ModuleProvider.new(module)
        ee = llvm.ee.ExecutionEngine.new(mp)
        sizeof = ee.run_function(func, []).as_int()
        
        _structSizeCache[str(type)] = sizeof
        
    if arch == "i386-darwin":
        # Up to 8 bytes are ok.
        return sizeof <= 8
    
    if arch == "i386-linux":
        # Can't find documentation but looking at clang-cc, it seems to always
        # pass struct via temporary memory objects. 
        return False

    if arch == "x86_64-linux":
        # FIXME: Not tested ...
        return sizeof <= 16
    
    util.error("returnsStructByValue() does not support architecture %s" % arch, component="system.py")
    

