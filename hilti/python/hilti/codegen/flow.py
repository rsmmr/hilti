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

@codegen.when(instructions.flow.CallTailVoid)
def _(self, i):
    func = self.lookupFunction(i.op1().value())
    assert func
    
    frame = self._llvm.frameptr
    builder = self.builder()
    
    # Allocate new stack frame for called function.
    #   callee_frame = new __frame_func
    callee_frame = self.llvmMalloc(self.llvmTypeFunctionFrame(func), "callee_frame")

    # After call, continue with next block.
    #   callee_frame.bf.cont_normal.label = <successor function>
    fpt = self.llvmTypeFunctionPtr(self._function)
    addr = self.llvmAddrDefContSuccessor(callee_frame, fpt)
    block = self._function.lookupBlock(i.op3().value())
    assert block
    succ = self.llvmGetFunctionForBlock(block)
    self.llvmInit(succ, addr)

    #   callee_frame.bf.cont_normal.frame = __frame
    frt = self.llvmTypeFunctionFrame(self._function)
    frt = llvm.core.Type.pointer(llvm.core.Type.pointer(frt))
    addr = self.llvmAddrDefContFrame(callee_frame, frt)
    self.llvmInit(frame, addr)

    # Keep current exception handler.
    #   callee_frame.bf.cont_exception.label = __frame.bf.cont_exception.label
    src_succ = self.llvmAddrExceptContSuccessor(frame)
    dst_succ = self.llvmAddrExceptContSuccessor(callee_frame)
    current = builder.load(src_succ, "current")
    self.llvmInit(current, dst_succ)
    
    #   callee_frame.bf.cont_exception.frame = __frame.bf.cont_exception.frame
    src_frame = self.llvmAddrExceptContSuccessor(frame)
    dst_frame = self.llvmAddrExceptContSuccessor(callee_frame)
    current = builder.load(src_frame, "current")
    self.llvmInit(current, dst_frame)
    
    # Clear exception information.
    #   callee_frame.bf.exception = None
    addr = self.llvmAddrException(callee_frame)
    zero = self.llvmConstException(0)
    self.llvmInit(zero, addr)
    
    #   callee_frame.bf.exception_data = Null
    addr = self.llvmAddrExceptionData(callee_frame)
    null = llvm.core.Constant.null(self.llvmTypeGenericPointer())
    self.llvmInit(null, addr)
      
    # Initialize function arguments. 
    args = i.op2().value()
    ids = func.type().IDs()
    assert len(args) == len(ids)
    
    for i in range(0, len(args)):
	    #   callee_frame.arg_<i> = args[i] 
        val = self.llvmOp(args[i], "arg%d" % i)
        addr = self.llvmAddrLocalVar(callee_frame, ids[i], "addr-arg%d" % i)
        self.llvmInit(val, addr)
    
    self.llvmGenerateTailCallToFunction(func, [callee_frame])
    
@codegen.when(instructions.flow.ReturnVoid)
def _(self, i):
    # return (*__frame.bf.cont_normal.label) (*__frame.bf.cont_normal)
    fpt = self.llvmTypeBasicFunctionPtr()
    addr = self.llvmAddrDefContSuccessor(self._llvm.frameptr, fpt)
    ptr = self.builder().load(addr, "succ_ptr")
    
    bfptr = llvm.core.Type.pointer(self.llvmTypeBasicFrame())
    addr = self.llvmAddrDefContFrame(self._llvm.frameptr, llvm.core.Type.pointer(bfptr))
    frame = self.builder().load(addr, "frame_ptr")
    
    self.llvmGenerateTailCallToFunctionPtr(ptr, [frame])
        
@codegen.when(instructions.flow.ReturnResult)
def _(self, i):
    # TODO
    pass

@codegen.when(instructions.flow.Jump)
def _(self, i):
    b = self._function.lookupBlock(i.op1().value())
    assert b
    self.llvmGenerateTailCallToBlock(b, [self._llvm.frameptr])

@codegen.when(instructions.flow.CallC)
def _(self, i):
    func = self.lookupFunction(i.op1().value())
    assert func
    
    args = [self.llvmOpToC(op, "arg") for op in i.op2().value()]
    self.llvmGenerateCCall(func, args)
