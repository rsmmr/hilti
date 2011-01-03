# $Id$
"""
.. hlt:type:: caddr

   The ``caddr`` data type stores the physical memory address of a HILTI
   object. It is primarily a tool for passing such an address on to an
   external C program. Note that there's no type information associated with a
   *caddr* value and thus care has to be taken when using it to access memory.
"""

import llvm.core

import hilti.function as function

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("caddr", 22, c="void *")
class CAddr(type.ValueType):
    """Type for ``caddr``."""
    def __init__(self, location=None):
        super(CAddr, self).__init__(location=location)

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        return cg.llvmTypeGenericPointer()

    ### Overridden from ValueType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::caddr_to_string";
        return typeinfo

    def llvmDefault(self, cg):
        """A ``caddr`` is initialized to ``null``."""
        return llvm.core.Constant.null(cg.llvmTypeGenericPointer())

@hlt.constraint("function")
def _exportedFunction(ty, op, i):
    if not isinstance(ty, type.Function):
        return (False, "must be a function name")

    if not isinstance(op, operand.ID):
        return (False, "must be a function name")

    if op.value().linkage() != id.Linkage.EXPORTED:
        return (False, "must have export linkage")

    # TODO: Should check that we reference a constant here. But how?

    return (True, "")

@hlt.instruction("caddr.function", op1=_exportedFunction, target=cIsTuple([cCaddr, cCaddr]))
class Function(Instruction):
    """Returns the physical address of a function. The function must be of
    linkage ~~EXPORT, and the instruction returns two separate addresses. For
    functions of calling convention ~~HILTI, the first is that of the compiled
    function itself and the second is that of the corresponding ~~resume
    function. For functions of calling convention ~~C and ~~C_HILTI, only the
    first element of the target tuple is used, and the second is undefined.
    *op1* must be a constant.
    """
    def _codegen(self, cg):

        fid = self.op1().value()
        func = cg.lookupFunction(fid)
        builder = cg.builder()

        if func.callingConvention() == function.CallingConvention.HILTI:
            (hltmain, main, hltresume, resume) = cg.llvmCStubs(func)
            main = builder.bitcast(main, cg.llvmTypeGenericPointer())
            resume = builder.bitcast(resume, cg.llvmTypeGenericPointer())
        elif func.callingConvention() == function.CallingConvention.C_HILTI:
            func = cg.llvmFunction(func)
            main = builder.bitcast(func, cg.llvmTypeGenericPointer())
            resume = llvm.core.Constant.null(cg.llvmTypeGenericPointer())
        else:
            util.internal_error("caddr.Function not supported for non-HILTI/HILTI-C functions yet")

        struct = llvm.core.Constant.struct([main, resume])
        cg.llvmStoreInTarget(self, struct)
