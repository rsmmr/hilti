# $Id$
#
# Code generator for Bytes instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

import integer

_doc_c_conversion = """A ``bytes`` object is mapped to ``hlt::bytes *``. The
type is defined in |hilti_intern.h|."""

def _llvmIteratorType(cg):
    # struct hlt_bytes_pos
    return llvm.core.Type.struct([cg.llvmTypeGenericPointer()] * 2)

def _llvmBytesType():
    return codegen.llvmTypeGenericPointer()

@codegen.typeInfo(type.Bytes)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "hlt::bytes *"
    typeinfo.to_string = "hlt::bytes_to_string"
    return typeinfo

@codegen.typeInfo(type.IteratorBytes)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "hlt::bytes_pos"
    return typeinfo

@codegen.llvmType(type.Bytes)
def _(type, refine_to):
    return _llvmBytesType()

@codegen.llvmType(type.IteratorBytes)
def _(type, refine_to):
    return _llvmIteratorType(codegen)

def llvmEnd():
    """Returns an ``llvm.core.Value`` that marks the end of a (any) bytes
    object.
    
    Note: The value returned must correspond to what ``bytes.c`` expects as
    the end-of-bytes marker, obviously."""
    
    return llvm.core.Constant.struct([llvm.core.Constant.null(codegen.llvmTypeGenericPointer())] * 2)

@codegen.llvmDefaultValue(type.IteratorBytes)
def _(type):
    return llvmEnd()

@codegen.llvmCtorExpr(type.Bytes)
def _(op, refine_to):
    # We create two globals:
    #
    # (1) one for storing the raw characters themselves.
    # (2) one for storing the bytes objects (which will point to (1))
    #
    # Note that (2) needs to be dynamically intialized via an LLVM ctor. 
    
    size = len(op.value())
    array = [llvm.core.Constant.int(llvm.core.Type.int(8), ord(c)) for c in op.value()]
    array = llvm.core.Constant.array(llvm.core.Type.int(8), array)
    
    data = codegen.llvmAddGlobalConst(array, "bytes-data")
    bytes = codegen.llvmAddGlobalVar(llvm.core.Constant.null(_llvmBytesType()), "bytes")

    def callback():
        builder = codegen.builder()
        bytes_new_from_data = codegen.llvmCurrentModule().get_function_named("__hlt_bytes_new_from_data")
        exception = codegen.llvmAlloca(codegen.llvmTypeExceptionPtr())
        exception = codegen.builder().bitcast(exception, llvm.core.Type.pointer(codegen.llvmTypeGenericPointer()))
        datac = builder.bitcast(data, codegen.llvmTypeGenericPointer())
        newobj = builder.call(bytes_new_from_data, [datac, codegen.llvmConstInt(size, 32), exception])
        codegen.llvmAssign(newobj, bytes)

    codegen.llvmAddGlobalCtor(callback)
    
    return bytes
    
@codegen.operator(instructions.bytes.New)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::bytes_new", [], [])
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.operator(instructions.bytes.IterIncr)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::bytes_pos_incr", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.operator(instructions.bytes.IterDeref)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::bytes_pos_deref", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.operator(instructions.bytes.IterEqual)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::bytes_pos_eq", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.operator(instructions.bytes.Equal)
def _(self, i):
    cmp = self.llvmGenerateCCallByName("hlt::bytes_cmp", [i.op1(), i.op2()], [i.op2().type(), i.op2().type()])
    zero = codegen.llvmConstInt(0, 8)
    result = codegen.builder().icmp(llvm.core.IPRED_EQ, cmp, zero)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Length)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::bytes_len", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Empty)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::bytes_empty", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Append)
def _(self, i):
    self.llvmGenerateCCallByName("hlt::bytes_append", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])

@codegen.when(instructions.bytes.Sub)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::bytes_sub", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Offset)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::bytes_offset", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Begin)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::bytes_begin", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.End)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::bytes_end", [])
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.when(instructions.bytes.Diff)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::bytes_pos_diff", [i.op1(), i.op2()], [i.op2().type(), i.op2().type()])
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.when(instructions.bytes.Cmp)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::bytes_cmp", [i.op1(), i.op2()], [i.op2().type(), i.op2().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.unpack(type.Bytes)
def _(t, begin, end, fmt, arg):
    """Bytes unpacking extract a subrange from a bytes object per the
    following formats:
    
    .. literalinclude:: /libhilti/hilti.hlt
       :start-after: %doc-packed-bytes-start
       :end-before:  %doc-packed-bytes-end
       
    The ``SKIP_*`` variants do always return an empty bytes object but advance
    the unpacking iterator in the same way as the non-skip version. This can
    be used if the data itself is not uninteresting but it's end position is
    needed to then unpack any subsequent data. 
    """
    
    bytes = codegen.llvmAlloca(codegen.llvmTypeConvert(t))
    iter = codegen.llvmAlloca(codegen.llvmTypeConvert(type.IteratorBytes(type.Bytes())))
    exception = codegen.llvmAddrException(codegen.llvmCurrentFramePtr())
    
    extract_one = codegen.llvmCurrentModule().get_function_named("__hlt_bytes_extract_one")

    def unpackFixed(n, skip, begin):
        def _unpackFixed(case):
            # We build a loop here even in the constant case, hoping that LLVM
            # is able to do some smart loop unrolling ...
            # FIXME: Check that that is indeed the case ...
            block_head = codegen.llvmNewBlock("loop-head")
            block_body = codegen.llvmNewBlock("loop-body")
            block_exit = codegen.llvmNewBlock("loop-exit")

            width = n.type.width
            zero = codegen.llvmConstInt(0, width)
            one = codegen.llvmConstInt(1, width)

            # Copy the start iterator.
            codegen.builder().store(begin, iter)
            iter_casted = codegen.builder().bitcast(iter, codegen.llvmTypeGenericPointer())
            
            # Make sure it's not already zero. 
            builder = codegen.builder()
            done = builder.icmp(llvm.core.IPRED_ULE, n, zero)
            builder.cbranch(done, block_exit, block_head)

            codegen.pushBuilder(block_head)
            j = codegen.llvmAlloca(n.type)
            codegen.llvmInit(n, j)
            codegen.builder().branch(block_body)
            codegen.popBuilder()
            
            # Loop body.
            builder = codegen.pushBuilder(block_body)
            
            byte = builder.call(extract_one, [iter_casted, end, exception])
            cur = builder.sub(builder.load(j), one)
            builder.store(cur, j)
            done = builder.icmp(llvm.core.IPRED_ULE, cur, zero)
            builder.cbranch(done, block_exit, block_body)
            codegen.popBuilder()
            
            # Loop exit.
            builder = codegen.pushBuilder(block_exit)
            
            if not skip:
                val = codegen.llvmGenerateCCallByName("hlt::bytes_sub", [begin, builder.load(iter)], [type.IteratorBytes(type.Bytes())] * 2, llvm_args=True)
            else:
                val = llvm.core.Constant.null(codegen.llvmTypeGenericPointer())
                
            codegen.llvmInit(val, bytes)
                
            # Leave builder on stack.
            
        return _unpackFixed

    def unpackRunLength(arg, skip):
        def _unpackRunLength(case):
            # Note: this works only with constant arguments.  I think that's ok
            # but: (FIXME) We should document that somewhere. 
            (n, iter) = codegen.llvmUnpack(type.Integer(32), begin, end, arg)
            return unpackFixed(n, skip, iter)(case)
        
        return _unpackRunLength
    
    def unpackDelim(width, skip):
        def _unpackDelim(case):
            delim = ""
            try:
                # FIXME: This is set in llvmOp(). We should come up with a
                # better interface for getting the original value. 
                delim = arg._value
            except AttributeError:
                pass
            
            if delim and len(delim) == 1:
                # We optimize for the case of a single, constant byte.
                
                delim = codegen.llvmConstInt(ord(delim), 8)
                block_body = codegen.llvmNewBlock("loop-body-start")
                block_cmp = codegen.llvmNewBlock("loop-body-cmp")
                block_exit = codegen.llvmNewBlock("loop-exit")
    
                # Copy the start iterator.
                builder = codegen.builder()
                builder.store(begin, iter)
                iter_casted = builder.bitcast(iter, codegen.llvmTypeGenericPointer())
                
                # Enter loop.
                builder.branch(block_body)
                
                # Loop body.
                
                    # Check whether end reached.
                builder = codegen.pushBuilder(block_body)
                done = codegen.llvmGenerateCCallByName("hlt::bytes_pos_eq", [builder.load(iter), end], [type.IteratorBytes(type.Bytes())] * 2, llvm_args=True)
                builder = codegen.builder()
                builder.cbranch(done, block_exit, block_cmp)
                codegen.popBuilder()

                    # Check whether we found the delimiter
                builder = codegen.pushBuilder(block_cmp)
                byte = builder.call(extract_one, [iter_casted, end, exception])
                done = builder.icmp(llvm.core.IPRED_EQ, byte, delim)
                builder.cbranch(done, block_exit, block_body)
                codegen.popBuilder()

                # Loop exit.
                builder = codegen.pushBuilder(block_exit)
                
                if not skip:
                    val = codegen.llvmGenerateCCallByName("hlt::bytes_sub", [begin, builder.load(iter)], [type.IteratorBytes(type.Bytes())] * 2, llvm_args=True)
                else:
                    val = llvm.core.Constant.null(codegen.llvmTypeGenericPointer())
                    
                codegen.llvmInit(val, bytes)
                    
            # Leave builder on stack.
                
            else:
                # Everything else we outsource to C.
                # FIXME: Well, not yet ...
                util.internal_error("byte unpacking with delimiters only supported for a single constant byte yet")
            
        return _unpackDelim

    cases = [
        ("Hilti::Packed::BytesRunLength", unpackRunLength(arg, False)),
        ("Hilti::Packed::BytesFixed", unpackFixed(arg, False, begin)),
        ("Hilti::Packed::BytesDelim", unpackDelim(arg, False)),
        
        ("Hilti::Packed::SkipBytesRunLength", unpackRunLength(arg, False)),
        ("Hilti::Packed::SkipBytesFixed", unpackFixed(begin, arg, True)),
        ("Hilti::Packed::SkipBytesDelim", unpackDelim(arg, True)),
        ]

    codegen.llvmSwitch(fmt, cases)
    
    # It's fine to check for an exception at the end.
    codegen._llvmGenerateExceptionTest(exception)
    
    return (codegen.builder().load(bytes), codegen.builder().load(iter))    
