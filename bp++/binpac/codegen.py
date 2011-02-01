# $Id$
"""Compiles a BinPAC++ module into a HILTI module."""

builtin_id = id

import node
import type
import id
import pgen

import hilti
import hilti.builder
import binpac.util as util

def compileModule(mod, import_paths, debug=False, validate=True):
    """Compiles a BinPAC++ module into a HILTI module.  *mod* must be
    well-formed as verified by ~~validator.validateModule.

    mod: ~~Module - The module to compile.
    import_paths: list of strings - List of paths to be searched for HILTI modules.
    validate: bool - If true, the correctness of the generated HILTI code will
    be verified via HILTI's internal validator.
    debug: bool - If true, debug code may be compiled in.

    Returns: tuple (bool, llvm.core.Module) - If the bool is True, code
    generation (and if *validate* is True, also validation) was successful. If
    so, the second element of the tuple is the resulting HILTI module.
    """
    codegen = CodeGen(mod, import_paths, validate, debug)

    if not codegen._compile():
        return (False, None)

    hltmod = codegen.moduleBuilder().module()

    if validate:
        errors = hilti.validateModule(hltmod)
        if errors > 0:
            return (False, None)

    return (True, hltmod)

class CodeGen(object):
    def __init__(self, mod, import_paths, validate, debug):
        """Main code generator class. See ~~compileModule for arguments."""
        self._module = mod
        self._import_paths = import_paths
        self._validate = validate
        self._debug = debug

        self._errors = 0
        self._mbuilder = 0
        self._builders = [None]
        self._pgens = [None]
        self._hooks = [None]
        self._saved_modules = []

    def debug(self):
        """Returns true if compiling in debugging mode.

        Returns: bool - True if in debugging mode.
        """
        return self._debug

    def currentModule(self):
        """Returns the module currently being generated.

        Returns: ~~Module - The module
        """
        return self._module

    _anon_funcs = 0

    def beginFunction(self, name, ftype, hook=False):
        """Starts a new function. The method creates the function builder as
        well as the initial block builder (which will be returned by
        subsequent calls to ~~builder).

        When done building this function, ~~endFunction must be called.

        name: string - The name of the function; *may* be none for
        anonymous function like those added to a HILTI hook.

        ftype: hilti.type.Function - The HILTI type of the
        function.

        Returns: tuple (~~hilti.builder.FunctionBuilder,
        ~~hilti.builder.BlockBuilder) - The builders created
        for the new function.
        """

        CodeGen._anon_funcs += 1

        fbuilder = hilti.builder.FunctionBuilder(self._mbuilder, name if name else "<no-name-%d>" % CodeGen._anon_funcs, ftype, dontadd=(name == None))
        self._builders += [fbuilder.newBuilder(None)]

        return (fbuilder, self._builders[-1])

    def endFunction(self):
        """Finished building a function. Must be called after ~~beginFunction
        once the code of the function has been fully generated. It resets the
        block builder.
        """
        self._builders = self._builders[:-1]

    def setBuilder(self, builder):
        """Sets the current builder. The set builder will then be
        returned by subsequent ~~builder calls.

        builder: hilti.hilti.BlockBuilder: The builder to set.
        """
        assert builder
        self._builders[-1] = builder

    def builder(self):
        """Returns the current block builder.

        Returns: hilti.builder.BlockBuilder - The block
        builder, as set by ~~setBuilder."""
        assert self._builders[-1]
        return self._builders[-1]

    def functionBuilder(self):
        """
        Returns the current function builder.

        Returns: hilti.builder.FunctionBuilder - The function
        builder.
        """
        fbuilder = self.builder().functionBuilder()
        assert fbuilder
        return fbuilder

    def moduleBuilder(self):
        """Returns the builder for the HILTI module currently being built.
        Must only be used after ~~compile() has been called.

        Returns: ~~hilti.builder.ModuleBuilder - The HILTI module builder.
        """
        assert self._mbuilder
        return self._mbuilder

    def beginCompile(self, pgen):
        """Starts parser generation with a new generator.

        pgen: ~~ParserGen - The parser generator.
        """
        self._pgens += [pgen]

    def endCompile(self):
        """Finishes compiling with a parser generator.
        """
        self._pgens = self._pgens[:-1]

    def beginHook(self, hook):
        """Starts compiling a unit hook.

        hook: ~~UnitHook - The hook.
        """
        self._hooks += [hook]

    def endHook(self):
        """Finishes compiling a unit hook.
        """
        self._hooks = self._hooks[:-1]

    def inHook(self, hook):
        """Checks whether we are currently compiling a given hook. This can be
        used to avoid infinite recursion when one hook triggers another one.

        hook: ~~UnitHook - The hook.

        Returns: True if the hook is currently bein compiled.
        """
        for h in self._hooks:
            if h and h.hiltiName(self) == hook.name():
                return True

        return False

    def parserGen(self):
        """Returns the parser generator for grammar currently being compiled.

        Returns: ~~ParserGen - The parser generator.
        """
        assert self._pgens[-1]
        return self._pgens[-1]

    def importPaths(self):
        """Returns the HILTI library paths to search.

        Returns: list of string - The paths.
        """
        return self._import_paths

    def error(self, context, msg):
        """Reports an error during code generation.

        msg: string - The error message to report.

        context: any - Optional object with a *location* method returning a
        ~~Location suitable to include into the error message.
        """
        loc = context.location() if context else None
        util.error(msg, component="codegen", context=loc)
        self._errors += 1

    def _compileOne(self, hltmod, module):
        # The rest of the code expects to find the module currently being
        # compiled in self._module so we set it accordingly.
        #
        # FIXME: Clean this up.
        self._saved_modules += [self._module]
        self._module = module

        for i in module.scope().IDs():

            if i.imported():
                # We'll do this once we compile the source module itself.
                continue

            if isinstance(i, id.Type) and isinstance(i.type(), type.Unit):
                # FIXME: Should get rid of %export.
                if i.linkage() == id.Linkage.EXPORTED:
                    gen = pgen.ParserGen(self, i.type())
                    gen.compile()
                    gen.export()

            if isinstance(i, id.Global):
                if i.expr():
                    init = i.expr().coerceTo(i.type(), cg=self).evaluate(self)
                else:
                    init = i.type().hiltiDefault(self)

                hltmod.scope().add(hilti.id.Global(i.name(), i.type().hiltiType(self), init))

            if isinstance(i, id.Function):
                i.function().evaluate(self)

        self._initFunction()
        module.execute(self)

        self._module = self._saved_modules[-1]
        self._saved_modules = self._saved_modules[:-1]

    def _compile(self):
        """Top-level driver of the compilation process.

        Todo: This is getting kind of hackish ...
        """
        hltmod = hilti.module.Module(self._module.name())
        self._mbuilder = hilti.builder.ModuleBuilder(hltmod)

        paths = self.importPaths()
        for mod in ["libhilti", "hilti", "binpac"]:
            if not hilti.importModule(self._mbuilder.module(), mod, paths):
                self.error(hltmod, "cannot import module %s" % mod)

        self._errors = 0

        all_modules = [mod for (mod, path) in self._module.importedModules()]
        all_modules += [self._module]

        for mod in all_modules:
            self._compileOne(hltmod, mod)
            if self._errors:
                break

        self._errors += self._mbuilder.finish(validate=False)

        return self._errors == 0

    def _initFunction(self):
        """Generates a HILTI function initializing the module.

        We actually generate two: one of linkage HILTI with the
        actual module-level statements, and one wrappre of linkage
        INIT just calling the first one's stub function.
        """

        # Primary function.

        ftype = hilti.type.Function([], hilti.type.Void())
        name = "init_%s" % self._module.name()
        (fbuilder, builder) = self.beginFunction(name, ftype)
        fbuilder.function().id().setLinkage(hilti.id.Linkage.EXPORTED)

        for stmt in self._module.statements():
            stmt.execute(self)

        self.endFunction()

        # Wrapper function.

        wftype = hilti.type.Function([], hilti.type.Void())
        wname = "init_%s_wrapper" % self._module.name()
        (wfbuilder, wbuilder) = self.beginFunction(wname, wftype)
        wfbuilder.function().id().setLinkage(hilti.id.Linkage.INIT)

        funcs = wbuilder.addLocal("funcs", hilti.type.Tuple([hilti.type.CAddr()] * 2))
        wbuilder.caddr_function(funcs, wbuilder.idOp(fbuilder.function().name()))
        f = wbuilder.addLocal("f", hilti.type.CAddr())
        wbuilder.tuple_index(f, funcs, 0)

        wbuilder.call(None, wbuilder.idOp("BinPAC::call_init_func"), wbuilder.tupleOp(([f])))

        self.endFunction()

    def hookName(self, unit, field, ddarg=False, addl=None):
        """Returns the internal name of a HILTI hook function. The
        name is constructed based on the unit/field the hook belongs
        too.

        unit: ~~Unit - The unit type the hook belongs to.

        field: ~~Field or string - The unit field/variable/hook the hook
        belongs to. If a ~~Field, the hook is associated with that field. If a
        string, it must be either the name of one of the unit's variables, or
        the name of a global unit hook (e.g., ``%init``).

        ddarg: Bool - If true, the hook received an additional ``$$``
        argument. If so, the name is changed accordingly to avoid name
        conflicts.

        addl: string - If given, an additional string postfix that will be
        added to the generated hook name.

        Returns: string - The name of the internal hook functin.
        """
        ext = "_dollardollar" if ddarg else ""

        if addl:
            ext = ext + "_%s" % addl

        if isinstance(field, str):
            name = field

        else:
            name = field.name() if field.name() else "anon_%s" % builtin_id(field)

        name = name.replace("%", "__")

        module = unit.module().name()

        return "hook_on_%s_%s_%s%s" % (module, unit.name(), name, ext)

    def declareHook(self, unit, field, objtype, ddtype=None):
        """Adds a hook to the namespace of the current HILTI module. Returns
        the existing declaration if it already exists.

        unit: ~~Unit - The unit type the hook belongs to.

        field: ~~Field or string - The unit field/variable/hook the hook
        belongs to. If a ~~Field, the hook is associated with that field. If a
        string, it must be either the name of one of the unit's variables, or
        the name of a global unit hook (e.g., ``%init``).

        objtype: ~~hilti.Type - The type of the hook's ``self`` parameter.

        ddtype: ~~Type - The type of the hook's ``$$`` parameter, if any.

        Returns: ~~hilti.operand.ID - The ID referencing the hook.
        """
        name = self.hookName(unit, field, ddtype != None)
        args = [(hilti.id.Parameter("__self", objtype), None)]
        args += [(hilti.id.Parameter("__user", hilti.type.Reference(hilti.type.Unknown("BinPAC::UserCookie"))), None)]

        if ddtype:
            args += [(hilti.id.Parameter("__dollardollar", ddtype.hiltiType(self.cg()), None))]

        hid = self._mbuilder.declareHook(name, args, hilti.type.Void())

        return hilti.operand.ID(hid)

    def generateInsufficientInputHandler(self, args, eod_ok=False, iter=None):
        """Function generating code for handling insufficient input. If
        parsing finds insufficient input, it must execute the code generated
        by this method and then retry the same operation afterwards. The
        generated code handles both the case where more input may become
        available later (in which case it will return and let the caller try
        again), and the case where it may not (in which case it will throw an
        exception and never return). In the latter case, if *eod_ok* is not set,
        it will throw an ~~ParseError exception; if not it will throw an
        ~~ParseSuccess exception.

        args: An ~~Args objects with the current parsing arguments, as used by
        the ~~ParserGen.

        eod_ok: bool - Indicates whether running out of input is ok.

        Returns: ~~hilti.Operand - An boolean HILTI operand indicating whether
        end-of-data has been reached. Note that this will always be true if
        *eod_ok* is False because in that case any end-of-data will raise an
        exception; it is safe to just ignore the return value if *eod_ok* is
        false. If *eod_ok* is True, the calling code should first check the
        return value to see whether end-of-data has been reached, and repeat
        the original operation only if not.

        iter: hilti.Operand - An optional operand to use instead of
        ``args.cur``.
        """
        # If our flags indicate that we won't be getting more input, we throw
        # a parse error. Otherwise, we yield and return.
        fbuilder = self.functionBuilder()
        cont = fbuilder.addLocal("__cont", hilti.type.Bool())

        resume = fbuilder.newBuilder("resume")

        suspend = fbuilder.newBuilder("suspend")
        suspend.makeDebugMsg("binpac-verbose", "out of input, yielding ...")
        suspend.yield_()
        suspend.jump(resume)

        error = fbuilder.newBuilder("error")

        self.builder().bytes_is_frozen(cont, args.data)

        if eod_ok:
            error.makeDebugMsg("binpac-verbose", "parse error, insufficient input (but end-of-data would be ok)")

            self.builder().if_else(cont, resume, suspend)

            result = cont
        else:
            error.makeDebugMsg("binpac-verbose", "parse error, insufficient input")
            error.makeRaiseException("BinPAC::ParseError", "insufficient input")

            self.builder().if_else(cont, error, suspend)

            result = fbuilder.constOp(False)

        self.setBuilder(resume)

        if args.exported():
            self.parserGen().filterInput(args, resume=True)

        return result

    def debugPrintPtr(self, tag, ptr):
        cfunc = self.builder().idOp("BinPAC::debug_print_ptr")
        cargs = self.builder().tupleOp([self.builder().constOp(tag), ptr])
        self.builder().call(None, cfunc, cargs)

