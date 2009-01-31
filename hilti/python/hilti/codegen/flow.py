# $Id$
#
# Code generator for flow control instructions.
"""
Flow Control
------------


* Before code generation, all blocks are preprocessed according to
  the following constraints:

  - Each instruction block ends with a *terminator* instruction,
    which is one of ''jump'', ''return'', ''if.else'', or
    ''call.tail.void`, or ''call.tail.result`.
  
  - A terminator instruction must not occur inside a block (i.e.,
    somewhere else than as the last instruction of a block).
  
  - There must not be any ''call'' instruction. 
  
  - Each block has name unique within the function it belongs to.  

* We turn each block into a separate function, called
  ''<Func>__<name>'' , where ''Func'' is the name of the function
  the block belongs, and the ''name'' is the name of the block. Each
  such block function gets only single parameter of type
  ''__frame_Func''.

Flow Instruction Conversion
~~~~~~~~~~~~~~~~~~~~~~~~~~~
  
* We convert terminator instructions as per the following pseudo-code:

''jump label''
  ::
  return label(__frame)
  
''if.else cond (label_true, label_false)''
  ::
  TODO
  return cond ? lable_true(__frame) : lable_false(__frame)

''x = call.tail.result func args'' (function call /with/ return value)
  ::
  TODO

''call.tail.void func args'' (function call /without/ return value)
  ::
      # Allocate new stack frame for called function.
  callee_frame = new __frame_func
      # After call, continue with next block.
  callee_frame.bf.cont_normal.label = <sucessor block>
  callee_frame.bf.cont_normal.frame = __frame
  
      # Keep current exception handler.
  callee_frame.bf.cont_exception.label = __frame.bf.cont_exception.label
  callee_frame.bf.cont_exception.frame = __frame.bf.cont_exception.frame
  
      # Clear exception information.
  callee_frame.bf.exception = None
  callee_frame.bf.exception_data = Null
      
      # Initialize function arguments.
  callee_frame.arg_<i> = args[i] 

      # Call function.
  return func(callee_frame)
  
''return.void'' (return /without/ return value)
  ::
  return (*__frame.bf.cont_normal.label) (*__frame.bf.cont_normal)

''return.result result'' (return /with/ return value)
  ::
  TODO
"""

"""
Flow Control
------------

Data Structures
~~~~~~~~~~~~~~~

* A ''__continuation'' is a snapshot of the current processing state,
  consisting of (a) a function to be called to continue
  processing, and (b) the frame to use when calling the function.::

    struct Continuation {
        ref<label>         func
        ref<__basic_frame> frame
    }

* A ''__basic_frame'' is the common part of all function frame::

    struct __basic_frame {
        ref<__continuation> cont_normal 
        ref<__continuation> cont_exception
        int                 current_exception
        ref<any>            current_exception_data
    }
    
* Each function ''Foo'' has a function-specific frame containg all
  parameters and local variables::

    struct ''__frame_Foo'' {
        __basic_frame  bf
        <type_1>       arg_1
        ...
        <type_n>       arg_n
        <type_n+1>     local_1
        ...
        <type_n+m>     local_m
        

Calling Conventions
~~~~~~~~~~~~~~~~~~~

* All functions take a mandatory first parameter
  ''ref<__frame__Func> __frame''. Functions which work on the result
  of another function call (see below), take a second parameter
  ''<result-type> __return_value''.

* Before code generation, all blocks are preprocessed according to
  the following constraints:

  - Each instruction block ends with a *terminator* instruction,
    which is one of ''jump'', ''return'', ''if.else'', or
    ''call.tail.void`, or ''call.tail.result`.
  
  - A terminator instruction must not occur inside a block (i.e.,
    somewhere else than as the last instruction of a block).
  
  - There must not be any ''call'' instruction. 
  
  - Each block has name unique within the function it belongs to.  

* We turn each block into a separate function, called
  ''<Func>__<name>'' , where ''Func'' is the name of the function
  the block belongs, and the ''name'' is the name of the block. Each
  such block function gets only single parameter of type
  ''__frame_Func''.

Flow Instruction Conversion
~~~~~~~~~~~~~~~~~~~~~~~~~~~
  
* We convert terminator instructions as per the following pseudo-code:

''jump label''
  ::
  return label(__frame)
  
''if.else cond (label_true, label_false)''
  ::
  TODO
  return cond ? lable_true(__frame) : lable_false(__frame)

''x = call.tail.result func args'' (function call /with/ return value)
  ::
  TODO

''call.tail.void func args'' (function call /without/ return value)
  ::
      # Allocate new stack frame for called function.
  callee_frame = new __frame_func
      # After call, continue with next block.
  callee_frame.bf.cont_normal.label = <sucessor block>
  callee_frame.bf.cont_normal.frame = __frame
  
      # Keep current exception handler.
  callee_frame.bf.cont_exception.label = __frame.bf.cont_exception.label
  callee_frame.bf.cont_exception.frame = __frame.bf.cont_exception.frame
  
      # Clear exception information.
  callee_frame.bf.exception = None
  callee_frame.bf.exception_data = Null
      
      # Initialize function arguments.
  callee_frame.arg_<i> = args[i] 

      # Call function.
  return func(callee_frame)
  
''return.void'' (return /without/ return value)
  ::
  return (*__frame.bf.cont_normal.label) (*__frame.bf.cont_normal)

''return.result result'' (return /with/ return value)
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
    frame = cg._llvm.frameptr
    builder = cg.builder()
    
    # Allocate new stack frame for called function.
    #   callee_frame = new __frame_func
    callee_frame = cg.llvmMalloc(cg.llvmTypeFunctionFrame(func), "callee_frame")

    # After call, continue with next block.
    #   callee_frame.bf.cont_normal.label = <successor function>
    addr = cg.llvmAddrDefContSuccessor(callee_frame, llvm.core.Type.pointer(llvm_succ.type))
    cg.llvmInit(llvm_succ, addr)
    
    #   callee_frame.bf.cont_normal.frame = __frame
    frt = cg.llvmTypeFunctionFrame(cg._function)
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
    src_frame = cg.llvmAddrExceptContSuccessor(frame)
    dst_frame = cg.llvmAddrExceptContSuccessor(callee_frame)
    current = builder.load(src_frame, "current")
    cg.llvmInit(current, dst_frame)
    
    # Clear exception information.
    #   callee_frame.bf.exception = None
    addr = cg.llvmAddrException(callee_frame)
    zero = cg.llvmConstNoException()
    cg.llvmInit(zero, addr)
    
    #   callee_frame.bf.exception_data = Null
    addr = cg.llvmAddrExceptionData(callee_frame)
    null = llvm.core.Constant.null(cg.llvmTypeGenericPointer())
    cg.llvmInit(null, addr)
      
    # Initialize function arguments. 
    ids = func.type().IDs()
    assert len(args) == len(ids)
    
    for i in range(0, len(args)):
	    #   callee_frame.arg_<i> = args[i] 
        val = cg.llvmOp(args[i], "arg%d" % i, cast_to_type=ids[i].type())
        addr = cg.llvmAddrLocalVar(func, callee_frame, ids[i], "addr-arg%d" % i)
        cg.llvmInit(val, addr)

    cg.llvmGenerateTailCallToFunction(func, [callee_frame])
    
@codegen.when(instructions.flow.CallTailVoid)
def _(self, i):
    func = self.lookupFunction(i.op1().value())
    assert func

    args = i.op2().value()
    
    block = self._function.lookupBlock(i.op3().value())
    assert block
    
    llvm_succ = self.llvmGetFunctionForBlock(block)
    _makeCall(self, func, args, llvm_succ)

@codegen.when(instructions.flow.CallTailResult)
def _(self, i):
    func = self.lookupFunction(i.op1().value())
    assert func
    args = i.op2().value()
    block = self._function.lookupBlock(i.op3().value())
    
    # Create a new function which gets the return value 
    # as additional parameter. 
    self.ret_func_counter += 1
    succ_name = "__%s_return_%d" % (self.nameFunctionForBlock(self._block), self.ret_func_counter)
    arg_type = self.llvmTypeConvert(func.type().resultType())
    
    succ_ft = type.FunctionType(generic=True) # doesn't matter. 
    succ = function.Function(succ_name, succ_ft, None, parentfunc=func)
    llvm_succ = self.llvmCreateFunctionForBlock(succ_name, self._block, [("result", arg_type)])
    llvm_succ_block = llvm_succ.append_basic_block(succ_name)
    
    self.pushBuilder(llvm_succ_block)

    # TODO: This is not so nice. The problem is that we are now in a new
    # function so we can't just call storeInTarget(). Find something more
    # elegant. 
    addr = self.llvmAddrLocalVar(self._function, llvm_succ.args[0], id.ID(i.target().id().name(), None), "result_addr")
    self.llvmAssign(llvm_succ.args[1], addr)
    
    self.llvmGenerateTailCallToBlock(block, [llvm_succ.args[0]])
    self.popBuilder()
    
    _makeCall(self, func, args, llvm_succ)
    
@codegen.when(instructions.flow.CallC)
def _(self, i):
    func = self.lookupFunction(i.op1().value())
    assert func
    
    ids = func.type().IDs()
    tuple = i.op2().value()
    
    args = []
    for i in range(len(tuple)):
        args += [self.llvmOpToC(tuple[i], "arg", cast_to_type=ids[i].type())]
    
    self.llvmGenerateCCall(func, args)

def _makeReturn(cg, llvm_result=None, result_type=None):
    fpt = cg.llvmTypeBasicFunctionPtr([result_type] if result_type else [])
    addr = cg.llvmAddrDefContSuccessor(cg._llvm.frameptr, fpt)
    ptr = cg.builder().load(addr, "succ_ptr")
    
    bfptr = llvm.core.Type.pointer(cg.llvmTypeBasicFrame())
    addr = cg.llvmAddrDefContFrame(cg._llvm.frameptr, llvm.core.Type.pointer(bfptr))
    frame = cg.builder().load(addr, "frame_ptr")
    
    cg.llvmGenerateTailCallToFunctionPtr(ptr, [frame] + ([llvm_result] if llvm_result else []))
    
@codegen.when(instructions.flow.ReturnVoid)
def _(self, i):
    # return (*__frame.bf.cont_normal.label) (*__frame.bf.cont_normal)
    _makeReturn(self)
        
@codegen.when(instructions.flow.ReturnResult)
def _(self, i):
    # return (*__frame.bf.cont_normal.label) (*__frame.bf.cont_normal, result)
    op1 = self.llvmOp(i.op1(), "op1")
    _makeReturn(self, op1, i.op1().type())

@codegen.when(instructions.flow.Jump)
def _(self, i):
    b = self._function.lookupBlock(i.op1().value())
    assert b
    self.llvmGenerateTailCallToBlock(b, [self._llvm.frameptr])

@codegen.when(instructions.flow.IfElse)
def _(self, i):
    op1 = self.llvmOp(i.op1(), "cond")
    true = self._function.lookupBlock(i.op2().value())
    false = self._function.lookupBlock(i.op3().value())

    block_true = self.llvmNewBlock("true")
    block_false = self.llvmNewBlock("false")
    
    self.pushBuilder(block_true)
    self.llvmGenerateTailCallToBlock(true, [self._llvm.frameptr])
    self.popBuilder()
    
    self.pushBuilder(block_false)
    self.llvmGenerateTailCallToBlock(false, [self._llvm.frameptr])
    self.popBuilder()
    
    self.builder().cbranch(op1, block_true, block_false)
