# $Id$
"""
Files
~~~~~

XXXX
"""

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

import bytes

@hlt.type("file", 30)
class File(type.HeapType):
    """Type for ``file``.
    
    location: ~~Location - Location information for the type.
    """
    def __init__(self, location=None):
        super(File, self).__init__(location=location)

    ### Overridden from HiltiType.
    
    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_file *"
        return typeinfo
        
    def llvmType(self, cg):
        """A ``file` is passed to C as ``hlt_file*``."""
        return cg.llvmTypeGenericPointer()


@hlt.overload(New, op1=cType(cFile), target=cReferenceOfOp(1))
class New(Operator):
    """Instantiates a new ``file`` instance, which will initially be closed
    and not associated with any actual file.
    """
    def codegen(self, cg):
        top = operand.Type(self.op1().value())
        result = cg.llvmCallC("hlt::file_new", [])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("file.open", op1=cReferenceOf(cFile), op2=cString, op3=cOptional(cIsTuple([cEnum, cEnum, cString])))
class Open(Instruction):
    """Opens a file *op1* for writing. *op2* is the path of the file. If not
    absolute, it is interpreted relative to the current directory. *op3* is
    tuple consisting of (1) the file type, either ~~Hilti::FileType::Text or
    ~~Hilti::FileType::Binary; (2) the file open mode, either
    ~~Hilti::FileMode::Create or ~~Hilti::FileMode::Append; and (3) a string
    giveing the output character set for writing out strings. If *op3* is not
    given, the default is ``(Hilti::FileType::Text, Hilti::FileMode::Create,
    "utf8")``.
    
    Raises ~~IOError if there was a problem opening the file. 
    """
    def codegen(self, cg):
        if self.op3():
            (ty, mode, charset) = self.op3().value().value()
        else:
            mod = cg.currentModule()
            ty = operand.ID(mod.scope().lookup("Hilti::FileType::Text"))
            mode = operand.ID(mod.scope().lookup("Hilti::FileMode::Create"))
            charset = operand.Constant(constant.Constant("utf8", type.String()))
        
        cg.llvmCallC("hlt::file_open", [self.op1(), self.op2(), ty, mode, charset])

@hlt.instruction("file.close", op1=cReferenceOf(cFile))
class Close(Instruction):
    """Closes the file *op1*. Further write operations will not be possible
    (unless reopened.  Other file objects still referencing the same physical
    file will be able to continue writing."""
    def codegen(self, cg):
        cg.llvmCallC("hlt::file_close", [self.op1()])

@hlt.constraint("string | ref<bytes>")
def _string_or_bytes(ty, op, i):
    if isinstance(ty, type.String):
        return (True, "")
    
    if isinstance(ty, type.Reference) and \
       isinstance(ty.refType(), type.Bytes):
        return (True, "")
    
    return (False, "must be string or ref<bytes>")
     
@hlt.instruction("file.write", op1=cReferenceOf(cFile), op2=_string_or_bytes)
class Write(Instruction):
    """Writes *op1* into the file. If *op1* is a string, it will be encoded
    according to the character set given to ~~file.open. If the file was
    opened in text mode, unprintable bytes characters will be suitably escaped
    and all lines will be terminated with newlines. It is guaranteed that a
    single execution of this instruction is atomic in the sense that all
    characters will be written out in one piece even if other threads are
    performing writes to the same file concurrently.  Multiple independent
    write call may however be interleaved with calls from other threads. 
    """
    def codegen(self, cg):
        if isinstance(self.op2().type(), type.String):
            # String version.
            cg.llvmCallC("hlt::file_write_string", [self.op1(), self.op2()])
        else:
            # Bytes version.
            cg.llvmCallC("hlt::file_write_bytes", [self.op1(), self.op2()])

