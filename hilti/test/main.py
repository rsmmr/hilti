#! /usr/bin/env python2.6

import ins.integer
import ins.flow
import instruction
import type
import visitor
import block
import module
import function
import printer
import id
import constant
import checker
import codegen.codegen
import codegen.cps

m = module.Module("my_module")
print ">>> <module.Module>", m

ft = type.FunctionType((id.ID("arg1", type.Int32), id.ID("arg2", type.Int32)), type.Void)
print ">>> <type.FunctionType>", ft

Op = instruction.Operand
ConstOp = instruction.ConstOperand
IDOp = instruction.IDOperand
TupleOp = instruction.TupleOperand
ID=id.ID
Const=constant.Constant

add1 = ins.integer.Add(IDOp(ID("x", type.Int16)), IDOp(ID("y", type.Int32)), IDOp(ID("z", type.Int32)))
print ">>> <ins.integer.Add>", add1

add2 = ins.integer.Add(ConstOp(Const(4, type.Int16)), ConstOp(Const("5", type.Int16)), ConstOp(Const("z", type.Int16)))
add3 = ins.integer.Add(IDOp(ID("y", type.Int16)), IDOp(ID("x", type.Int16)), IDOp(ID("z", type.Int16)))
#call = ins.flow.Call(Op("my_func", ft), Op(("x", "y"), type.TupleType((type.Int16, type.Int16))))
call = ins.flow.Call(IDOp(ID("my_func", ft)), TupleOp((IDOp(ID("x", type.Int16)), IDOp(ID("y", type.Int16)))), IDOp(ID("footarget", type.Int16)))
print ">>> <ins.flow.Call>", call

funcblock = block.Block([add1, add2, call, add3], "myblock")
print ">>> <block.Block>", funcblock

f = function.Function("my_func", ft)
f.addBlock(funcblock)
print ">>> <function.Function>", f

lid1 = id.ID("my_local1", type.String)
print ">>> <id.ID>", lid1

lid2 = id.ID("my_local2", type.Int16)
f.addID(lid1)
f.addID(lid2)

m.addFunction(f)

sid1 = id.ID("foo", type.String)
sid2 = id.ID("bar", type.Int16)
sid3 = id.ID("susi", type.Int32)
s = type.StructType((sid1, sid2, sid3))
print ">>> <type.StructType>", s

sd = type.StructDeclType(s)
print ">>> <type.StructDeclType>", sd

m.addID(id.ID("my_struct", sd))

gid1 = id.ID("my_global1", type.Int32)
gid2 = id.ID("my_global2", type.Int32)
m.addID(gid1)
m.addID(gid2)

checker.checker.dispatch(m)
printer.printer.dispatch(m)

print
print "------------------"
print

codegen.cps.cps.dispatch(m)
printer.printer.dispatch(m)

print
print "------------------"
print


codegen.codegen.codegen.dispatch(m)

print codegen.codegen.codegen.llvmModule()
