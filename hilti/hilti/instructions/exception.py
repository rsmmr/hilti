# $Id$
"""
The *exception* data type represents an exception that can divert control up
the calling stack to the closest function prepared to handle it.
"""

builtid_id = id

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

# Special place-holder for the root of all exceptions (initalized below).
_ExceptionRoot = None

@hlt.type("exception", 20)
class Exception(type.HeapType, type.Parameterizable):
    """Type for ``exception``.

    argtype: type.ValueType - The type of the optional exception argument, or None if no
    argument. If *baseclass* is specified and *baseclass* has an argument,
    *argtype* must match that one.

    baseclass: type.Exception - The base exception class this one is derived
    from, or None if it should be derived from the built-in top-level
    ``Hilti::Exception`` class.
    """
    def __init__(self, argtype=None, baseclass=None, location=None):
        super(Exception, self).__init__(location=location)
        self._argtype = argtype
        # Will be set to None for the root exception.
        self._baseclass = baseclass if baseclass else _ExceptionRoot

    def argType(self):
        """Returns the type of the exception's argument.

        Returns: Type - The type, or None if no argument.
        """
        return self._argtype

    def baseClass(self):
        """Returns the type of the exception's base class.

        Returns: type.Exception - The type of the base class.
        """
        return self._baseclass if self._baseclass else _ExceptionRoot

    def setBaseClass(self, base):
        """Set's the type of the exception's base class.

        base: type.Exception - The type of the base class.
        """
        self._baseclass = base

    def isRootType(self):
        """Checks whether the exception type represents the top-level root type.

        Returns: bool - True if *t* is the root exception type.
        """
        return builtin_id(self) == builtin_id(_ExceptionRoot)

    @staticmethod
    def root():
        """Returns the root exception type.

        Returns: ~~Exception - The root execption.
        """
        return _ExceptionRoot

    ### Overridden from Type.

    def name(self):
        if not self._baseclass.isRootType():
            base = " : %s" % self._baseclass.name()
        else:
            base = ""

        if self._argtype:
            return "exception<%s>%s" % (self._argtype, base)
        else:
            return "exception"

    def _validate(self, vld):
        if not self._baseclass:
            # The base class.
            return

        if self._circular():
            vld.error(self, "circuluar exception inheritance")
            return

        if self._argtype:
            self._argtype.validate(vld)

        self._baseclass.validate(vld)

        if self._baseclass._argtype and self._baseclass._argtype != self._argtype:
            vld.error(self, "exception argument type must match that of parent, which is %s" % self._baseclass._argtype)

        etype = self
        btype = self.baseClass()

        if not btype:
            vld.error(self, "no base class for exception")
            return

        if builtin_id(btype) == builtin_id(etype):
            vld.error(self, "exception type cannot be derived from itself")
            return

        if not etype.argType() and btype.argType() or \
           etype.argType() and btype.argType() and etype.argType() != btype.argType():
            vld.error(self, "exception argument type must match that of base class (%s)" % btype.argType())
            return

        # Make sure there's no loop in the inheritance tree.
        seen = [etype]
        while True:
            cur = seen[-1]
            parent = cur.baseClass()
            if parent.isRootType():
                break

            for c in seen[:-1]:
                if parent.exceptionName() == c.exceptionName():
                    vld.error(self, "loop in exception inheritance")
                    return

            seen += [parent]


    def _resolve(self, resolver):
        if self._circular():
            # Report in validate.
            return self

        if self._argtype:
            self._argtype = self._argtype.resolve(resolver)

        if self._baseclass:
            self._baseclass = self._baseclass.resolve(resolver)

        return self

    def output(self, printer):
        printer.output("exception<")
        if self._argtype:
            printer.printType(self._argtype)
        printer.output(">")

        if self._baseclass and not self._baseclass.isRootType():
            printer.output(" : ")
            printer.printType(self._baseclass)

    def cmpWithSameType(self, other):
        return builtin_id(self) == builtin_id(other)

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        """An ``exception`` is mapped to an ``hlt_exception *``."""
        return cg.llvmTypeExceptionPtr()

    ### Overridden from ValueType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_exception *"
        return typeinfo

    def llvmDefault(self, cg):
        return llvm.core.Constant.null(_llvmStringTypePtr(0))

    ### Overridden from Parameterizable.

    def args(self):
        return [self._argtype] if self._argtype else None

    ### Interal.

    def _circular(self):
        # Checks whether we find ourselves in the list of base classes.
        cur = self
        while True:
            parent = cur.baseClass()

            if isinstance(parent, type.Unknown):
                return False

            if parent.isRootType():
                return False

            if builtin_id(parent) == builtin_id(self):
                return True

            cur = parent


_ExceptionRoot = Exception(None, None)

# We check the second argument's type in checker.exception.
@hlt.overload(New, op1=cType(cException), op2=cOptional(cAny), target=cReferenceOfOp(1))
class New(Operator):
    """Allocates a new exception instance. If the exception type given in
    *op1* requires an argument, that must be passed in *op2*.
    """
    def _validate(self, vld):
        argt = self.op1().type().argType()
        ty = self.op2().type() if self.op2() else None

        if argt and not ty:
            vld.error(self, "exception argument of type %s missing" % argt)

        if not argt and ty:
            vld.error(self, "exception type does not take an argument")

        if argt and ty and argt != ty:
            vld.error(self, "exception argument must be of type %s" % argt)

    def _codegen(self, cg):
        arg = cg.llvmOp(self.op2()) if self.op2() else None
        excpt = cg.llvmNewException(self.op1().type(), arg, self.location())
        cg.llvmStoreInTarget(self, excpt)

@hlt.instruction("exception.throw", op1=cReferenceOf(cException), terminator=True)
class Throw(Instruction):
    """
    Throws an exception, diverting control-flow up the stack to the closest
    handler.
    """
    def _canonify(self, canonifier):
        canonifier.splitBlock(self)

    def _codegen(self, cg):
        exception = cg.llvmOp(self.op1())
        cg.llvmRaiseException(exception)

@hlt.instruction(".push_exception_handler", op1=cLabel, op2=cOptional(cType(cException)))
class PushHandler(Instruction):
    """Installs an exception handler for exceptions of *op1*, including any
    derived from that type. If such an exception is thrown subsequently,
    control is transfered to *op2*.
    """
    def _validate(self, vld):
        if not vld.currentFunction():
            vld.error(self, "cannot catch exceptions here")

    def _codegen(self, cg):
        handler = cg.lookupBlock(self.op1())
        excpt = self.op2().value() if self.op2() else type.Exception.root()
        cg.currentFunction().pushExceptionHandler(excpt, handler)

@hlt.instruction(".pop_exception_handler")
class PopHandler(Instruction):
    """Uninstalls the most recently installed exception handler. Exceptions of
    its type are no longer caught in subsequent code.
    """
    def _validate(self, vld):
        if not vld.currentFunction():
            vld.error(self, "cannot catch exceptions here")

    def _codegen(self, cg):
        cg.currentFunction().popExceptionHandler()








