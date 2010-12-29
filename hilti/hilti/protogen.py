#! /usr/bin/env
#
# C prototype geneator.

import os.path

import function
import id
import module
import type

class ProtoGen:
    """Generates C prototypes for the externally visible parts of a module.

    cg: ~~CodeGen - The code generator.

    mod: ~~mod - The module for which to generate prototypes.

    fname: string - The name of the file to write the prototypes to.
    """
    def protogen(self, cg, mod, fname):
        self._cg = cg
        self._fname = fname
        self._output = open(fname, "w")
        self._module = mod
        self._lines = []

        self.header()

        # Do globals.
        for i in sorted(mod.scope().IDs(), key=lambda i: i.name()):
            if isinstance(i, id.Constant):
                self.doConst(i)

        # Do functions.
        for i in sorted(mod.scope().IDs(), key=lambda i: i.name()):
            if isinstance(i, id.Function):
                self.doFunction(i)

        for line in sorted(self._lines):
            print >>self._output, line

        self.footer()

    def cg(self):
        """Returns the code generator in use.

        Returns: ~~CodeGen - The code generator.
        """
        return self._cg

    def output(self, line):
        """Outputs a single line into the prototype file.

        line: string - The line without the trailing newline.
        """
        self._lines += [line]

    def header(self):
        """Outputs the static header of the prototype file."""
        ifdefname = "HILTI_" + os.path.basename(self._fname).upper().replace("-", "_").replace(".", "_")

        print >>self._output, """
/* Automatically generated. Do not edit. */

#ifndef %s
#define %s

#include <hilti.h>
""" % (ifdefname, ifdefname)

    def footer(self):
        """Outputs the static footer of the prototype file."""
        print >>self._output, "\n#endif"

    def doFunction(self, i):
        """Outputs the prototype for a function.

        i: ~~id.Function - The ID defining the function.
        """

        f = i.function()

        if i.linkage() != id.Linkage.EXPORTED or f.callingConvention() != function.CallingConvention.HILTI:
            return

        if isinstance(f.type().resultType(), type.Void):
            result = "void"
        else:
            ti = self.cg().typeInfo(f.type().resultType())
            assert ti.c_prototype
            result = ti.cPrototype()

        args = []
        for a in f.type().args():
            if isinstance(a.type(), type.Void):
                args += ["void"]
            else:
                if isinstance(a.type(), type.Reference):
                    ti = self.cg().typeInfo(a.type().refType())
                else:
                    ti = self.cg().typeInfo(a.type())
                assert ti and ti.c_prototype
                args += [ti.c_prototype]

        args = "".join(["%s, " % a for a in args])
        self.output("%s %s(%shlt_exception **);" % (result, self.cg().nameFunction(f, prefix=False), args))

            # Resume function.
        self.output("%s %s_resume(const hlt_exception *, hlt_exception **);" % (result, self.cg().nameFunction(f, prefix=False)))


    def doConst(self, i):
        """Outputs the prototype for a constant.

        i: ~~id.Constant - The ID defining the constant.
        """
        # Don't generate constants for recusively imported IDs.
        if i.imported():
            return

        scope = self._module.name()
        scope = scope[0].upper() + scope[1:]

        if isinstance(i.type(), type.Enum):

            if i.type().isUndef(i.value()):
                return

            value = i.type().labels()[i.value().value()]
        else:
            value = i.value().value()

        self.output("static const hlt_enum %s_%s = { 0, 0, %s };" % (scope, i.name().replace("::", "_"), value))










