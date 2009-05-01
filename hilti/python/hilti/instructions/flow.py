# $Id$
"""
Flow Control
~~~~~~~~~~~~
"""

from hilti.core.type import *
from hilti.core.instruction import *

@instruction("jump", op1=Label, terminator=True)
class Jump(Instruction):
    """
    Jumps unconditionally to label *op2*.
    """
    pass

@instruction("return.void", terminator=True)
class ReturnVoid(Instruction):
    """
    Returns from the current function without returning any value.
    The function's signature must not specifiy a return value. 
    """
    pass

@instruction("return.result", op1=Optional(Any), terminator=True)
class ReturnResult(Instruction):
    """
    Returns from the current function with the given value.
    The type of the value must match the function's signature. 
    """
    pass

@instruction("if.else", op1=Bool, op2=Label, op3=Label, terminator=True)
class IfElse(Instruction):
    """
    Transfers control label *op2* if *op1* is true, and to *op3* otherwise. 
    """
    pass

@instruction("call", op1=Function, op2=Tuple, target=Optional(Any))
class Call(Instruction):
    """
    Calls *function* using the tuple in *op2* as 
    arguments. The argument types as well as the target's type must 
    match the function's signature.
    """
    pass

@instruction("call.c", op1=Function, op2=Tuple, target=Optional(Any))
class CallC(Instruction):
    """
    For internal use only.
    
    Calls the external C *function* using the tuple in *op2* as 
    arguments. The argument types as well as the target's type must 
    match the function's signature.
    """
    pass

@instruction("call.tail.void", op1=Function, op2=Tuple, op3=Label, terminator=True)
class CallTailVoid(Instruction):
    """
    For internal use only.
    
    Calls *function* using the tuple in *op2* as arguments. The
    argument types must match the function's signature, and the
    function must not return a value. Different than *Call()*,
    *CallTailVoid* must be the *last instruction* of a *Block*.
    After the called function returns, control is passed to 
    block *op3*.
    
    This instruction is for internal use only. During the code
    generation, all *Call* instructions are converted into either
    *CallTailVoid* or *CallTailResult* instructions. 
    """
    pass

@instruction("call.tail.result", op1=Function, op2=Tuple, op3=Label, target=Optional(Any), terminator=True)
class CallTailResult(Instruction):
    """
    
    Like *Call()*, calls *function* using the tuple in *op2* as 
    arguments. The argument types must match the function's
    signature. The function must return a result value and it's type
    must likewise match the function's signature. Different than
    *Call()*, *CallTailVoid* must be the *last instruction* of a
    *Block*.  After the called function returns, control is passed
    to block *op3*.

    
    This instruction is for internal use only. During the code
    generation, all *Call* instructions are converted into either
    *CallTailVoid* or *CallTailResult* instructions. 
    """
    pass

@instruction("thread.yield", terminator=True)
class ThreadYield(Instruction):
    """
    Schedules the next block on the same worker thread and
    returns to the scheduler to allow another virtual thread
    to run.
    """
    pass

@instruction("thread.schedule", op1=Integer, op2=Function, op3=Tuple, terminator=False)
class ThreadSchedule(Instruction):
    """
    Schedules a function call onto a virtual thread.
    """
    pass
