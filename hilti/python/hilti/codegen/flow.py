# $Id$
#
# Code generator for flow control instructions.
"""
Function Signatures
~~~~~~~~~~~~~~~~~~~

All functions receive their call frame as a their first parameter. For a
function called ``Foo``, the type of is defined as::

    struct __frame_Foo {
        __basic_frame bf
        <type_1>      arg_1
        ...
        <type_n>      arg_n
        <type_n+1>    local_1
        ...
        <type_n+m>    local_m
        }

``arg_x`` are the arguments passed to the function, and ``local_x`` are the
function's local variables. The ``__basic_frame`` is a common header shared by
all function frames::

    struct __basic_frame {
        Continuation  cont_normal 
        Continuation  cont_exception
        Exception*    exception
    }

A ``Continuation`` is a snapshot of the current processing state, consisting
of (1) a function to be called to continue processing, and (2) the function
frame to use when calling the function::

    struct Continuation {
        function      *func
        __basic_frame *frame
    }

The ``cont_normal`` specifies where to transfer control if the function
finished normally, and ``cont_exception`` where to go to when an exception is
raised. 

A raised exception is signaled by setting the ``exception`` field to an
``Exception`` object. Before control is transfered to ``cont_exception``, the
``exception`` is copied over to ``cont_exception.frame->exception``.

Except as indicated below, the frame is the only parameter a function receives.

CPS Conversion
~~~~~~~~~~~~~~

Before code generation, all blocks are preprocessed by the ~~canonifier; see
there for a list of transformations. We then turn each block into a separate
function, called ``Func_name`` , where ``Func`` is the name of the function
the block belongs to, and the ``name`` is the name of the block. Each such
block function gets only single parameter ``__frame`` of type
``__frame_<Func>``.

Assuming we're inside a function called ``Foo``, we then convert |terminator|
instructions per the following pseudo-code:

``jump label``
  ::
   
    return Foo_label(__frame)
  
``if.else cond (label_true, label_false)``
  ::
  
    return cond ? Func_label_true(__frame) : Func_label_false(__frame)

``call.tail.void Callee Args`` (function call *without* return value)
  ::
  
        # Allocate new stack frame for called function.
    callee_frame = new __frame_Callee
        # After call, continue with next block.
    callee_frame.bf.cont_normal.func = <sucessor block>
    callee_frame.bf.cont_normal.frame = __frame
    
        # Keep current exception handler.
    callee_frame.bf.cont_exception.func = __frame.bf.cont_exception.func
    callee_frame.bf.cont_exception.frame = __frame.bf.cont_exception.frame
    
        # Clear exception information.
    callee_frame.bf.exception = None
        
        # Initialize function arguments.
    callee_frame.arg_<i> = Args[i] 
  
        # Initialize function' locals.
    callee_frame.local_<i> = <default-value>
  
        # Call function.
    return func(callee_frame)

``return.void`` (return *without* return value)
  ::
  
    return (*__frame.bf.cont_normal.func) (*__frame.bf.cont_normal)

``x = call.tail.result Callee Args`` (function call *with* return value)
  ::
  
    TODO

``return.result result`` (return *with* return value)
  ::
  
    TODO
"""

import llvm
import llvm.core 

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.when(instructions.flow.Call)
def _(self, i):
    assert False

def _makeCall(cg, func, args, llvm_succ):
    frame = cg.llvmCurrentFramePtr()
    builder = cg.builder()
    
    # Allocate new stack frame for called function.
    #   callee_frame = new __frame_func
    callee_frame = cg.llvmAllocFunctionFrame(func)

    # After call, continue with next block.
    #   callee_frame.bf.cont_normal.label = <successor function>
    addr = cg.llvmAddrDefContSuccessor(callee_frame, llvm.core.Type.pointer(llvm_succ.type))
    cg.llvmInit(llvm_succ, addr)
    
    # callee_frame.bf.cont_normal.frame = __frame
    frt = cg.llvmTypeFunctionFrame(cg.currentFunction())
    frt = llvm.core.Type.pointer(llvm.core.Type.pointer(frt))
    addr = cg.llvmAddrDefContFrame(callee_frame, frt)
    cg.llvmInit(frame, addr)

    # Keep current exception handler.
    #   callee_frame.bf.cont_exception.label = __frame.bf.cont_exception.label
    src_succ = cg.llvmAddrExceptContSuccessor(frame)
    dst_succ = cg.llvmAddrExceptContSuccessor(callee_frame)
    current = builder.load(src_succ, "current")
    cg.llvmInit(current, dst_succ)
    
    #   callee_frame.bf.cont_exception.frame = __frame.bf.cont_exception.frame
    src_frame = cg.llvmAddrExceptContFrame(frame)
    dst_frame = cg.llvmAddrExceptContFrame(callee_frame)
    current = builder.load(src_frame, "current")
    cg.llvmInit(current, dst_frame)
 
    # Clear exception information.
    #   callee_frame.bf.exception = None
    addr = cg.llvmAddrException(callee_frame)
    zero = cg.llvmConstNoException()
    cg.llvmInit(zero, addr)
    
    # Initialize function arguments. 
    ids = func.type().args()
    assert len(args) == len(ids)
    
    for i in range(0, len(args)):
	    #   callee_frame.arg_<i> = args[i] 
        val = cg.llvmOp(args[i])
        addr = cg.llvmAddrLocalVar(func, callee_frame, ids[i].name())
        cg.llvmInit(val, addr)

    cg.llvmGenerateTailCallToFunction(func, [callee_frame])
    
@codegen.when(instructions.flow.CallTailVoid)
def _(self, i):
    func = self.lookupFunction(i.op1().value())
    assert func

    args = i.op2().value()
    
    block = self.currentFunction().lookupBlock(i.op3().value().name())
    assert block
    
    llvm_succ = self.llvmGetFunctionForBlock(block)
    _makeCall(self, func, args, llvm_succ)

@codegen.when(instructions.flow.CallTailResult)
def _(self, i):
    func = self.lookupFunction(i.op1().value())
    assert func
    args = i.op2().value()
    block = self.currentFunction().lookupBlock(i.op3().value().name())
    
    # Create a new function which gets the return value 
    # as additional parameter. 
    
    succ_name = self.nameNewFunction("%s_return" % self.nameFunctionForBlock(self.currentBlock()))
    arg_type = self.llvmTypeConvert(func.type().resultType())
    
    llvm_succ = self.llvmCreateFunction(self.currentBlock().function(), succ_name, [("result", arg_type)])
    llvm_succ_block = llvm_succ.append_basic_block(succ_name)
    
    self.pushBuilder(llvm_succ_block)

    # TODO: This is not so nice. The problem is that we are now in a new
    # function so we can't just call storeInTarget(). Find something more
    # elegant. 
    addr = self.llvmAddrLocalVar(self.currentFunction(), llvm_succ.args[0], i.target().id().name())
    self.llvmAssign(llvm_succ.args[1], addr)
    
    self.llvmGenerateTailCallToBlock(block, [llvm_succ.args[0]])
    self.popBuilder()
    
    _makeCall(self, func, args, llvm_succ)
    
@codegen.when(instructions.flow.CallC)
def _(self, i):
    func = self.lookupFunction(i.op1().value())
    assert func
    
    ids = func.type().args()
    tuple = i.op2().value()
    
    result = self.llvmGenerateCCall(func, tuple, [t.type() for t in tuple])
    if i.target():
        self.llvmStoreInTarget(i.target(), result)

def _makeReturn(cg, llvm_result=None, result_type=None):
    fpt = llvm.core.Type.pointer(cg.llvmTypeBasicFunctionPtr([result_type] if result_type else []))
    addr = cg.llvmAddrDefContSuccessor(cg.llvmCurrentFramePtr(), fpt)
    ptr = cg.builder().load(addr, "succ_ptr")
    
    bfptr = llvm.core.Type.pointer(cg.llvmTypeBasicFrame())
    addr = cg.llvmAddrDefContFrame(cg.llvmCurrentFramePtr(), llvm.core.Type.pointer(bfptr))
    frame = cg.builder().load(addr, "frame_ptr")
    
    cg.llvmGenerateTailCallToFunctionPtr(ptr, [frame] + ([llvm_result] if llvm_result else []))
    
@codegen.when(instructions.flow.ReturnVoid)
def _(self, i):
    # return (*__frame.bf.cont_normal.label) (*__frame.bf.cont_normal)
    _makeReturn(self)
        
@codegen.when(instructions.flow.ReturnResult)
def _(self, i):
    # return (*__frame.bf.cont_normal.label) (*__frame.bf.cont_normal, result)
    op1 = self.llvmOp(i.op1())
    _makeReturn(self, op1, i.op1().type())

@codegen.when(instructions.flow.Jump)
def _(self, i):
    b = self.currentFunction().lookupBlock(i.op1().value().name())
    assert b
    self.llvmGenerateTailCallToBlock(b, [self.llvmCurrentFramePtr()])

@codegen.when(instructions.flow.IfElse)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    true = self.currentFunction().lookupBlock(i.op2().value().name())
    false = self.currentFunction().lookupBlock(i.op3().value().name())

    block_true = self.llvmNewBlock("true")
    block_false = self.llvmNewBlock("false")
    
    self.pushBuilder(block_true)
    self.llvmGenerateTailCallToBlock(true, [self.llvmCurrentFramePtr()])
    self.popBuilder()
    
    self.pushBuilder(block_false)
    self.llvmGenerateTailCallToBlock(false, [self.llvmCurrentFramePtr()])
    self.popBuilder()
    
    self.builder().cbranch(op1, block_true, block_false)

# FIXME: _constructFrame is based on _makeCall(), and currently there's a certain amount of code
# duplication going on here. Can _makeCall() be made to utilize _constructFrame()?

def _constructFrame(cg, func, args, contNormFunc, contNormFrame, contExceptFunc, contExceptFrame):
    frame = cg.llvmCurrentFramePtr()
    builder = cg.builder()

    # Allocate new stack frame for called function.
    #   callee_frame = new __frame_func
    callee_frame = cg.llvmAllocFunctionFrame(func)

    # After call, continue with given block.
    addr = cg.llvmAddrDefContSuccessor(callee_frame, llvm.core.Type.pointer(contNormFunc.type))
    cg.llvmInit(contNormFunc, addr)
    
    # Use given frame for normal continuation.
    addr = cg.llvmAddrDefContFrame(callee_frame, llvm.core.Type.pointer(contNormFrame.type))
    cg.llvmInit(contNormFrame, addr)

    # Use given exception handler.
    addr = cg.llvmAddrExceptContSuccessor(callee_frame, llvm.core.Type.pointer(contExceptFunc.type))
    cg.llvmInit(contExceptFunc, addr)
    
    # Use given frame for exception continuation.
    addr = cg.llvmAddrExceptContFrame(callee_frame, llvm.core.Type.pointer(contExceptFrame.type))
    cg.llvmInit(contExceptFrame, addr)

    # Clear exception information.
    addr = cg.llvmAddrException(callee_frame)
    zero = cg.llvmConstNoException()
    cg.llvmInit(zero, addr)
    
    # Initialize function arguments. 
    ids = func.type().args()
    assert len(args) == len(ids)
    
    for i in range(0, len(args)):
        val = cg.llvmOp(args[i])
        addr = cg.llvmAddrLocalVar(func, callee_frame, ids[i].name())
        cg.llvmInit(val, addr)
    
    return callee_frame

def _getStandardFrame(cg):
    # Get the reusable global portions of the standard frame.
    cont_normal = cg.llvmCurrentModule().get_function_named("__hlt_standard_cont_normal")
    cont_normal_frame = cg.llvmCurrentModule().get_global_variable_named("__hlt_standard_frame")
    cont_except = cg.llvmCurrentModule().get_function_named("__hlt_standard_cont_except")

    # Construct the frame for the exception continuation. (This can't be reused.)
    cont_except_frame = cg.llvmMalloc(cg.llvmTypeBasicFrame())
    exception_addr = cg.llvmAddrException(cont_except_frame)
    cg.llvmInit(cg.llvmConstNoException(), exception_addr)

    return (cont_normal, cont_normal_frame, cont_except, cont_except_frame)

@codegen.when(instructions.flow.ThreadYield)
def _(self, i):
    # Get the next block as a function pointer.
    block = self.currentBlock()
    nextBlock = block.next()
    llvmFunc = self.llvmGetFunctionForBlock(nextBlock) 

    # Get the current frame.
    frame = self.llvmCurrentFramePtr()

    # Create a 'void *' type. (in LLVM it's pointer to i8)
    voidPtrType = self.llvmTypeGenericPointer()

    # Get an LLVM handle for the worker scheduler function.
    schedFunc = self.llvmCurrentModule().get_function_named('__hlt_local_schedule_job')

    # Cast the frame to 'void *'.
    castedFrame = self.builder().bitcast(frame, voidPtrType, 'castedFrame')

    # Generate call to the worker scheduler.
    call = self.builder().call(schedFunc, [llvmFunc, castedFrame])
    call.calling_convention = llvm.core.CC_C

    # This is a terminator instructor, so we need to generate a tail call to the next block.
    self.llvmGenerateTailCallToBlock(nextBlock, [frame])

@codegen.when(instructions.flow.ThreadSchedule)
def _(self, i):
    # Get standard information for frame.
    (contNormFunc, contNormFrame, contExceptFunc, contExceptFrame) = _getStandardFrame(self)

    # Get the virtual thread number.
    vthread = self.llvmOp(i.op1())

    # Get the function to schedule.
    func = self.lookupFunction(i.op2().value().name())
    assert func
    llvmFunc = self.llvmGetFunction(func)

    # Get a pointer to the function.
    funcPtrType = self.llvmTypeBasicFunctionPtr()
    castedFunc = self.builder().bitcast(llvmFunc, funcPtrType)

    # Get the function arguments.
    args = i.op3().value()

    # Generate the frame for the function.
    frame = _constructFrame(self, func, args, contNormFunc, contNormFrame, contExceptFunc, contExceptFrame)

    # Create a 'void *' type. (in LLVM it's pointer to i8)
    voidPtrType = self.llvmTypeGenericPointer()

    # Get an LLVM handle for the global scheduler function.
    schedFunc = self.llvmCurrentModule().get_function_named('__hlt_global_schedule_job')

    # Cast the frame to 'void *'.
    castedFrame = self.builder().bitcast(frame, voidPtrType, 'castedFrame')

    # Generate call to the global scheduler.
    call = self.builder().call(schedFunc, [vthread, castedFunc, castedFrame])
    call.calling_convention = llvm.core.CC_C

@codegen.when(instructions.flow.Yield)
def _(self, i):
    next = self.currentBlock().next()
    succ_func = self.llvmGetFunctionForBlock(next) 
    
    # Generate a Yield exception.
    exception_new_yield = codegen.llvmCurrentModule().get_function_named("__hlt_exception_new_yield")
    # FIXME: Add location information.
    arg = self.llvmOp(i.op1()) if i.op1() != None else self.llvmConstInt(0, 32)
    location = self.llvmAddGlobalStringConst(str(i.location()), "yield")
    cont = self.llvmContinuation(succ_func, self.llvmCurrentFramePtr())
    excpt = self.builder().call(exception_new_yield, [cont, arg, location])
    self.llvmRaiseException(excpt)
    
