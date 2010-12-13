# $Id$
"""
Todo.
"""

import llvm.core

import hilti.type as type

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type(None, 19)
class Label(type.ValueType):
    """Type for block labels.

    Note: This is a value type so that we can pass it to C functions.
    """
    def __init__(self):
        super(Label, self).__init__()

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        """A label is mapped to ``void *``."""
        return cg.llvmTypeGenericPointer()

    ### Overridden from ValueType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "void *"
        return typeinfo

    def llvmDefault(self, cg):
        return llvm.core.Constant.null(cg.llvmType(type.refType()))

    ### Overridden from HiltiType.

    def cPassTypeInfo(self):
        # Always pass type infromation
        return True


