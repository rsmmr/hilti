# $Id$
#
# Control flow operations. 

from instruction import *
from type import *

@instruction("jump", op1=Label)
class Jump(Instruction):
    """
    Jumps unconditionally to label *op2*.
    """
    pass

@instruction("return", op1=Any)
class Return(Instruction):
    """
    Returns from the current function with the given value. 
    The type of the value must match the function's signature. 
    """
    pass

@instruction("jump.if", op1=Bool, op2=Label)
class If(Instruction):
    """
    Jumps to label *op2* if *op1* is true.
    """
    pass

@instruction("call", op1=Function, op2=AnyTuple, target=Any)
class Call(Instruction):
    """
    Like *Call()*, calls *function* using the tuple in *op2* as 
    arguments. The argument types as well as the target's type must 
    match the function's signature.
    """
    pass

@instruction("call.tail", op1=Function, op2=AnyTuple, target=Any)
class TailCall(Instruction):
    """
    Like *Call()*, calls *function* using the tuple in *op2* as 
    arguments. The argument types as well as the target's type must 
    match the function's signature. Different than *Call()*, *TailCall* 
    must be the *last instruction* of a *Block*.
    
    This instruction is primarily for internal use. The *CPS* pass converts 
    all normal *Call* instructions into *TailCalls* by splitting functions 
    as necessary.
    """
    pass
    
