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
    
import sys    
    
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
    fpt = cg.llvmTypeBasicFunctionPtr([result_type] if result_type else [])
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

import sys

# FIXME: In general, the private functions below are duplicate code from existing functions that almost met my
# needs, but not quite. This is bad; the duplicate code needs to be unified.

def _llvmFuncPtrType(cg, args=[]):
    voidt = llvm.core.Type.void()
    args = [cg.llvmTypeConvert(t) for t in args]
    bf = [llvm.core.Type.pointer(cg.llvmTypeBasicFrame())]
    ft = llvm.core.Type.function(voidt, bf + args)
    return llvm.core.Type.pointer(ft)

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

def _getContinuation(cg):
    if hasattr(_getContinuation, "result"):
        return _getContinuation.result

    # Create a variety of HILTI things we don't really need, but that are necessary
    # to use codegen functions.
    # TODO: This isn't robust to multiple modules right now... gotta fix that.
    # FIXME: Actually I believe this whole thing is unnecessary now that Robin has introduced
    # a way to automatically import certain C prototypes into every HILTI module. Should
    # migrate this code to that.

    #hiltiMod = module.Module('__hlt_sched_module')
    hiltiMod = cg.currentModule()
    hiltiFuncType = type.Function([], type.Void)
    hiltiNormFunc = function.Function(hiltiFuncType, '__cont_normal', hiltiMod)
    hiltiExceptFunc = function.Function(hiltiFuncType, '__cont_except', hiltiMod)

    # Create the LLVM functions.
    #llvmModule = llvm.core.Module.new('__hlt_sched_module')
    llvmModule = cg.llvmCurrentModule()
    frameType = cg.llvmTypeFunctionFrame(hiltiNormFunc)
    fpType = llvm.core.Type.function(llvm.core.Type.void(), [llvm.core.Type.pointer(frameType)])
    normFunc = llvmModule.add_function(fpType, '__cont_normal')
    exceptFunc = llvmModule.add_function(fpType, '__cont_except')

    # Create the code for the normal continuation.
    block = normFunc.append_basic_block('entry')
    builder = llvm.core.Builder.new(block)
    builder.ret_void()

    # Create the code for the exception continuation.
    # TODO: Do something with the exception.
    block = exceptFunc.append_basic_block('entry')
    builder = llvm.core.Builder.new(block)
    builder.ret_void()

    # Create the frames.
    normFrame = cg.llvmAllocFunctionFrame(hiltiNormFunc)
    exceptFrame = cg.llvmAllocFunctionFrame(hiltiExceptFunc)

    # Store and return the continuation info.
    _getContinuation.result = (normFunc, normFrame, exceptFunc, exceptFrame)
    return _getContinuation.result

def _getScheduler(cg, name, args):
    # This uses some private vars of CoreGen so I should move this.
    # I only don't use llvmGetCFunction because it (like so many things) insists on HILTI types.
    # There should be a better solution than this code duplication.

    #name = '__hlt_global_schedule'

    try:
        llvmfunc = cg._llvm.functions[name]
    except KeyError:
        #args = [llvm.core.Type.int(32), _llvmFuncPtrType(cg), cg.llvmTypeGenericPointer()]
        ft = llvm.core.Type.function(llvm.core.Type.void(), args)
        llvmfunc = cg._llvm.module.add_function(ft, name)
        llvmfunc.calling_convention = llvm.core.CC_C    # I changed this from llvm.calling_convention; llvmGetCFunction may be wrong?
        cg._llvm.functions[name] = llvmfunc

    return llvmfunc

@codegen.when(instructions.flow.ThreadYield)
def _(self, i):
    # Get the next block as a function pointer.
    block = self.currentBlock()
    nextBlock = block.next()
    llvmFunc = self.llvmGetFunctionForBlock(nextBlock) 

    # Get the current frame.
    frame = self.llvmCurrentFramePtr()

    # Create a 'void *' type. (in LLVM it's pointer to i8)
    voidPtrType = llvm.core.Type.pointer(llvm.core.Type.int(8))

    # Get an LLVM handle for the worker scheduler function.
    #schedFunc = _getWorkerScheduler(self)
    schedFunc = _getScheduler(self, '__hlt_local_schedule_job', [_llvmFuncPtrType(self), self.llvmTypeGenericPointer()])

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
    (contNormFunc, contNormFrame, contExceptFunc, contExceptFrame) = _getContinuation(self)

    # Get the virtual thread number.
    vthread = self.llvmConstInt(i.op1().value(), 32)

    # Get the function to schedule.
    func = self.lookupFunction(i.op2().value().name())
    assert func
    llvmFunc = self.llvmGetFunction(func)

    # Get a pointer to the function.
    #funcPtrType = llvm.core.Type.pointer(func.type())
    funcPtrType = _llvmFuncPtrType(self)

    # Get the function arguments.
    # TODO: Not yet implemented. Only schedule functions that take no arguments for now.
    args = []

    # Generate the frame for the function.
    frame = _constructFrame(self, func, args, contNormFunc, contNormFrame, contExceptFunc, contExceptFrame)

    # Create a 'void *' type. (in LLVM it's pointer to i8)
    voidPtrType = llvm.core.Type.pointer(llvm.core.Type.int(8))

    # Get an LLVM handle for the global scheduler function.
    schedFunc = _getScheduler(self, '__hlt_global_schedule_job', [llvm.core.Type.int(32), _llvmFuncPtrType(self), self.llvmTypeGenericPointer()])

    # Cast the frame to 'void *'.
    castedFrame = self.builder().bitcast(frame, voidPtrType, 'castedFrame')

    # Generate call to the global scheduler.
    call = self.builder().call(schedFunc, [vthread, llvmFunc, castedFrame])
    call.calling_convention = llvm.core.CC_C
