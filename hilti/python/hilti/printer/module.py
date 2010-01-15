# $Id$
#
# Overall control structures plus generic instruction printing.

import socket
import struct

builtin_id = id

from hilti.core import *
from printer import printer

def _suppressID(id):
    
    if not id.location():
        return False
    
    # Don't print any standard IDs defined implicitly by the run-time
    # environment.
    if id.location().internal():
        return True

    # Don't print any IDs from an imported module; we will print out the import
    # statements instead. 
    # Todo: Is this now redundant with the next test for scopes?
    
    if printer._module:
        paths = [path for (mod, path) in printer._module.importedModules()]
        if id.location().file() in paths:
            return True

        # Don't print any IDs with a different scope. 
        if id.scope() and id.scope() != printer._module.name():
            return True
    
    return False

def _scopedID(id):
    if printer._module:
        if id.scope() and id.scope() != printer._module.name():
            return "%s::%s" % (id.scope(), id.name())
        
    return id.name()

def _fmtOp(op):
    # FIXME: Should we outsource this to a per-type decorator?
    
    if isinstance(op, instruction.ConstOperand):
        c = op.constant()

        if isinstance(c.type(), type.String):
            return '"%s"' % c.value().encode("utf-8").replace("\\", "\\\\")
        
        if isinstance(c.type(), type.Bitset):
            for (label, val) in c.type().labels().items():
                if c.value() == val:
                    return "%s::%s" % (_findTypeName(c.type()), label)
            assert False

        if isinstance(c.type(), type.Enum):
            for (label, val) in c.type().labels().items():
                if c.value() == val:
                    return "%s::%s" % (_findTypeName(c.type()), label)
            assert False

        if isinstance(c.type(), type.Reference) and \
           isinstance(c.type().refType(), type.Bytes):
            return 'b"%s"' % c.value().encode("string-escape")
        
        if isinstance(c.type(), type.Reference) and \
           isinstance(c.type().refType(), type.RegExp):
            return " | ".join(["/%s/" % re for re in c.value()])
        
        if isinstance(c.type(), type.Reference) and c.value() == None:
            return "Null"
        
        if isinstance(c.type(), type.Addr):
            (b1, b2) = c.value()
            
            if b1 == 0 and b2 & 0xffffffff00000000 == 0:
                # IPv4
                return socket.inet_ntop(socket.AF_INET, struct.pack("!1L", b2))
            else:
                # IPv6
                return socket.inet_ntop(socket.AF_INET6, struct.pack("!2Q", b1, b2))
            
        if isinstance(c.type(), type.Net):
            (b1, b2, mask) = c.value()
            
            if b1 == 0 and b2 & 0xffffffff00000000 == 0:
                # IPv4
                return "%s/%d" % (socket.inet_ntop(socket.AF_INET, struct.pack("!1L", b2)), mask - 96)
            else:
                # IPv6
                return "%s/%d" % (socket.inet_ntop(socket.AF_INET6, struct.pack("!2Q", b1, b2)), mask)
            
        if isinstance(c.type(), type.Reference) and \
           isinstance(c.type().refType(), type.List):
            return "%s(%s)" % (_fmtType(c.type().refType()), ", ".join([_fmtOp(c) for c in c.value() ]))

    if isinstance(op, instruction.IDOperand):
        i = op.value()
        return _scopedID(i)
    
    if isinstance(op, instruction.TypeOperand):
        return _fmtType(op.value())
    
    if isinstance(op, instruction.TupleOperand):
        elems = [_fmtOp(o) for o in op.value()]
        elems = [o if o else "<None>" for o in elems]
        return "(%s)" % ", ".join(elems)
        
    # For the others, the default printing does the trick.
    return str(op)

def _fmtInsOp(name, op, sig, prefix, postfix):
    if not op:
        return ""
    
    _printComment(op, name)
    return "%s%s%s" % (prefix, _fmtOp(op), postfix) 

def _fmtInstruction(ins):
    _printComment(ins, separate=True)
    target = _fmtInsOp("target", ins.target(), ins.signature().target(), "", " = ")
    op1 = _fmtInsOp("op1", ins.op1(), ins.signature().op1(), " ", "")
    op2 = _fmtInsOp("op2", ins.op2(), ins.signature().op2(), " ", "")
    op3 = _fmtInsOp("op3", ins.op3(), ins.signature().op3(), " ", "")
    printer.output("%s%s%s%s%s" % (target, ins.name(), op1, op2, op3))
    terminator = ins.signature().terminator()

def _findTypeName(ty):
    for id in printer._module.IDs():
        if not isinstance(id.type(), type.TypeDeclType):
            continue

        if builtin_id(id.type().declType()) == builtin_id(ty):
            return _scopedID(id)
        
    assert False

def _fmtType(ty):
    # If it's reference, recursively print the reference type.
    if isinstance(ty, type.Reference):
        return "ref<%s>" % _fmtType(ty.refType())
    
    # Likewise for iterators. 
    if isinstance(ty, type.Iterator):
        return "iterator<%s>" % _fmtType(ty.containerType())

    # Unknown types internally have a unique name, which isn't so nice for
    # printing (though note that when an unknown type is printed, there's some
    # error in the program anyway). 
    if isinstance(ty, type.Unknown):
        return "(unresolvable type)"

    if isinstance(ty, type.HiltiType):
        args = ty.args()

        if len(args) == 1 and args[0] is type.Wildcard:
            return "%s<*>" % ty._plain_name

        if args:
            # FIXME: This is a hack. If there's at least one type arg, we format
            # all the args ourselves to make sure recursive types are printed
            # correctly. This should really be moved into type-specific methods.
            have_type = False
            for a in args:
                if isinstance(a, type.Type):
                    have_type = True
                    
            if have_type:
                nargs = []
                for a in args:
                    if isinstance(a, type.Type):
                        nargs += [_fmtType(a)]
                    else:
                        nargs += [_fmtOp(a)]
                    
                return "%s<%s>" % (ty._plain_name, ",".join(nargs))

    # See if we find a type declaration for this type in the modules scope. If
    # so return the name of the type.
    if printer._module:
        for id in printer._module.IDs():
            if not isinstance(id.type(), type.TypeDeclType):
                continue

            if builtin_id(id.type().declType()) == builtin_id(ty):
                return _scopedID(id)

    return str(ty)

def _printComment(node, prefix=None, separate=False):
    if not node.comment():
        return

    if separate:
        printer.output("")
    
    prefix = "(%s) " if prefix else ""
    
    for c in node.comment():
        printer.output("# %s%s" % (prefix, c))
        
@printer.when(module.Module)
def _(self, m):
    self._module = m
    _printComment(m)
    self.output("\nmodule %s\n" % m.name())
    
    mods = [mod for (mod, path) in printer._module.importedModules()]
    for mod in mods:
        if mod == "libhilti.hlt":
            continue
        
        self.output("import %s" % mod)
        
    if mods:
        self.output()
    
@printer.when(id.ID, type.TypeDeclType)
def _(self, id):
    if _suppressID(id):
        return

    name = id.name()
    role = id.role()
    ty = id.type().type()

    _printComment(id)

    if isinstance(ty, type.Enum):
        labels = [label for label in ty.labels().keys() if label != "Undef"]
        labels.sort()
        self.output("enum %s { %s }" % (name, ", ".join(labels)))
        
    if isinstance(ty, type.Bitset):
        labels = ["%s=%d" % (n, v) for (n, v) in ty.labels().items()]
        labels.sort()
        self.output("bitset %s { %s }" % (name, ", ".join(labels)))

    if isinstance(ty, type.Struct):
        self.output("struct %s {" % name)
        self.push()

        for (fid, default) in ty.fields():
            defop = " default=%s" % _fmtOp(default) if default else ""
            sep = "," if fid != ty.fields()[-1][0] else ""
            self.output("%s %s%s%s" % (_fmtType(fid.type()), fid.name(), defop, sep))
            
        self.output("}")
        self.pop()
        self.output()

    if isinstance(ty, type.Exception):
        arg = "(%s)" % _fmtType(ty.argType()) if ty.argType() else ""
        base = " : %s" % _fmtType(ty.baseClass().exceptionName()) if not ty.baseClass().isRootType() else ""
        self.output("exception %s%s%s" % (ty.exceptionName(), arg, base))
        
    if isinstance(ty, type.Overlay):
        self.output("overlay %s {" % name)
        self.push()
        
        for f in ty.fields():
            
            if isinstance(f.start(), str):
                pos = "after %s" % f.start()
            else:
                pos = "at %d" % f.start()
            
            packed = printer._module.lookupID("Hilti::Packed")
            assert packed
                
            fmt = instruction.ConstOperand(constant.Constant(f.fmt(), packed.type().declType()))
            arg = " %s" % _fmtOp(f.arg()) if f.arg() else ""
            
            sep = "," if f.name() != ty.fields()[-1].name() else ""
            self.output("%s: %s %s unpack with %s%s%s" % (f.name(), _fmtType(f.type()), pos, _fmtOp(fmt), arg, sep))
            
        self.output("}")
        self.pop()
        self.output()

@printer.when(id.ID, type.ValueType)
def _(self, i):
    if not self._module:
        return
    
    if _suppressID(i):
        return
    
    if i.role() != id.Role.GLOBAL and i.role() != id.Role.CONST:
        return

    _printComment(i)
    
    val = self._module.lookupIDVal(i)
    init = " = %s" % _fmtOp(val) if val else ""
    role = "global" if i.role() == id.Role.GLOBAL else "const"
    self.output("%s %s %s%s" % (role, _fmtType(i.type()), i.name(), init))

@printer.when(id.ID, type.Function)
def _(self, i):
    if _suppressID(i):
        return

    _printComment(i)
    
    name = i.name()
    ftype = i.type()
    func = self._module.lookupIDVal(i)
    assert func
    
    linkage = ""
    if not func.blocks():
        linkage = "declare "
    else:
        if func.linkage() == function.Linkage.EXPORTED:
            linkage = "export "
        if func.linkage() == function.Linkage.INIT:
            linkage = "init "

    cc = ""
    if func.callingConvention() == function.CallingConvention.C:
        cc += "\"C\" "
            
    if func.callingConvention() == function.CallingConvention.C_HILTI:
        cc += "\"C-HILTI\" "
            
    result = _fmtType(ftype.resultType())
    
    args = [(arg.type(), arg.name(), default) for (arg, default) in ftype.argsWithDefaults()]
    args = ", ".join(["%s %s%s" % (_fmtType(t), n, " = %s" % _fmtOp(d) if d else "") for (t, n, d) in args])
    self.output("\n%s%s%s %s(%s) " % (linkage, cc, result, name, args), nl=False)

    if not func.blocks():
        return
    
    self.output("{")
    self.push()

    locals = False
    for i in sorted(func.IDs(), key=lambda i: i.name()):
        if i.role() == id.Role.LOCAL and not isinstance(i.type(), type.Label):
            self.output("local %s %s" % (_fmtType(i.type()), i.name()))
            locals = True
            
    if locals:
        self.output("")
            
    first = True
    for block in func.blocks():
        if not first:
            self.output("")
            
        if block.name():
            self.pop()
            self.output("%s:" % block.name())
            self.push()

        _printComment(block)
            
        last = None
        for ins in block.instructions():
            _fmtInstruction(ins)
            last = ins
            
        if block.next() and (not last or not last.signature().terminator()):
            # Debugging aid: show the implicit jump at the end of a block that
            # has not terminator instruction there.
            self.output("# jump %s" % (block.next().name()))
            
        first = False
            
    self.pop()
    self.output("}")

@printer.when(id.ID)
def _(self, i):
    if _suppressID(i):
        return
    
    if i.role() == id.Role.CONST:
        val = self._module.lookupIDVal(i)
        
        if isinstance(val, instruction.ConstOperand):
            op = instruction.ConstOperand(val)
            self.output("const %s %s = %s" % (_fmtType(i.type()), i.name(), _fmtOp(op)))

@printer.when(instruction.Instruction)
def _(self, i):
    # Only for when we print *only* an instruction.
    if not self._module:
        _fmtInstruction(i)
