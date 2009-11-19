# $Id$
"""
Flow Control
~~~~~~~~~~~~
"""

from hilti.core.instruction import *
from hilti.core.constraints import *

@instruction("jump", op1=label, terminator=True)
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

@instruction("return.result", op1=optional(any), terminator=True)
class ReturnResult(Instruction):
    """
    Returns from the current function with the given value.
    The type of the value must match the function's signature. 
    """
    pass

@instruction("if.else", op1=bool, op2=label, op3=label, terminator=True)
class IfElse(Instruction):
    """
    Transfers control label *op2* if *op1* is true, and to *op3* otherwise. 
    """
    pass

@instruction("call", op1=function, op2=tuple, target=optional(any))
class Call(Instruction):
    """
    Calls *function* using the tuple in *op2* as 
    arguments. The argument types as well as the target's type must 
    match the function's signature.
    """
    pass

@instruction("call.c", op1=function, op2=tuple, target=optional(any))
class CallC(Instruction):
    """
    For internal use only.
    
    Calls the external C *function* using the tuple in *op2* as 
    arguments. The argument types as well as the target's type must 
    match the function's signature.
    """
    pass

@instruction("call.tail.void", op1=function, op2=tuple, op3=label, terminator=True)
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

@instruction("call.tail.result", op1=function, op2=tuple, op3=label, target=optional(any), terminator=True)
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
    
    Todo: Unify this with ``yield``
    """
    pass

@instruction("thread.schedule", op1=integerOfWidth(32), op2=function, op3=tuple, terminator=False)
class ThreadSchedule(Instruction):
    """
    Schedules a function call onto a virtual thread.
    """
    pass

@instruction("yield", op1=optional(integerOfWidth(32)), terminator=True)
class Yield(Instruction):
    """Raises a resumable ~~YieldException. The exception's argument will be
    set to *op1* if given. If the exception is resumed, execution will
    continue with the succeeding block.
    
    Note: While ideally, we'd like to allow any type for the argument, doing
    so void prevent us from statically type-checking the code in the exception
    handler, which is why we're limiting it to an integer. 
    
    Todo: Unify this with ``thread.yield``
    """
    pass

@constraint("YieldException")
def _yieldException(ty, op, i):
    if not isinstance(ty, type.Exception):
        return (False, "argument must be reference to a YieldException")

    if ty.exceptionName() != "Yield_Exception":
        return (False, "argument must be reference to a YieldException")

    return (True, "")
            
@instruction("resume", op1=referenceOf(_yieldException), terminator=True)
class Resume(Instruction):
    """Resumes a previously raised ~~YieldException. The control is transfered
    to the location where it was previously suspended. 
    
    Note: Resuming does not work for exceptions passes in from C because for
    them we can't propagate any *future* exceptions back. 
    
    Todo: Because we still can't catch exceptions in a HILTI program, it's
    actually not possible to get a value for *op1*; therefore this instruction
    is untested.
    """
    pass

@constraint("( (value, destination), ...)")
def _switchTuple(ty, op, i):

    if not isinstance(ty, type.Tuple):
        return (False, "operand 3 must be a tuple")
    
    types = ty.types()

    for j in range(0, len(types)):
        et = types[j]
    
        if not isinstance(et, type.Tuple) or len(et.types()) != 2:
            return (False, "tuple element %d is not a 2-tuple" % (j+1))
        
        ett = et.types()

        if ett[0] != i.op1().type():
            return (False, "all values must be of same type as operand 1 (value %d is not)" % (j+1))
        
        if not isinstance(ett[1], type.Label):
            return (False, "all destination must be labels (destination %d is not)" % (j+1))
    
    return (True, "")

@instruction("switch", op1=valueType, op2=label, op3=_switchTuple, terminator=True)
class Switch(Instruction):
    """Branches to one of several alternatives. *op1* determines which
    alternative to take.  *op3* is a tuple of giving all alternatives as 
    2-tuples *(value, destination)*. *value* must be of the same type as
    *op1*, and *destination* is a block label. If *value* equals *op1*,
    control is transfered to the corresponding block. If multiple alternatives
    match *op1*, one of them is taken but it's undefined which one. If no
    alternative matches, control is transfered to block *op2*.
    """
    
    
