# $Id$
"""Compiles a BinPAC++ module into a HILTI module."""

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

    def debug(self):
        """Returns true if compiling in debugging mode. 
        
        Returns: bool - True if in debugging mode. 
        """
        return self._debug

    def currentModule(self):
        """Returns the module currently being validated.
        
        Returns: ~~Module - The module
        """
        return self._module
    
    def beginFunction(self, name, ftype):
        """Starts a new function. The method creates the function builder as
        well as the initial block builder (which will be returned by
        subsequent calls to ~~builder). 
        
        When done building this function, ~~endFunction must be called.
        
        name: string - The name of the function.
        ftype: hilti.type.Function - The HILTI type of the
        function.
        
        Returns: tuple (~~hilti.builder.FunctionBuilder,
        ~~hilti.builder.BlockBuilder) - The builders created
        for the new function.
        """
        fbuilder = hilti.builder.FunctionBuilder(self._mbuilder, name, ftype)
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
        return hook in self._hooks
        
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
        
    def _compile(self):
        """Top-level driver of the compilation process.
        
        Todo: Much of this implementation should move to Module.
        """
        hltmod = hilti.module.Module(self._module.name())
        self._mbuilder = hilti.builder.ModuleBuilder(hltmod)

        self._errors = 0

        paths = self.importPaths()
        
        for mod in ["hilti", "binpac", "binpacintern"]:
            if not hilti.importModule(self._mbuilder.module(), mod, paths):
                self.error(hltmod, "cannot import module %s" % mod)
                
        for i in self._module.scope().IDs():
            if isinstance(i, id.Type) and isinstance(i.type(), type.Unit):
                # FIXME: Should get rid of %export.
                if i.type().property("export").value() or i.linkage() == id.Linkage.EXPORTED:
                    gen = pgen.ParserGen(self, i.type())
                    gen.compile()
                    gen.export()
            
            if isinstance(i, id.Global):
                if i.expr():
                    init = i.expr().evaluate(self)
                else:
                    init = i.type().hiltiDefault(self)
                    
                hltmod.scope().add(hilti.id.Global(i.name(), i.type().hiltiType(self), init))
                
            if isinstance(i, id.Function):
                i.function().evaluate(self)

        self._initFunction()
                
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
        wbuilder.tuple_index(f, funcs, builder.constOp(0))

        wbuilder.call(None, wbuilder.idOp("BinPACIntern::call_init_func"), wbuilder.tupleOp(([f])))
        
        self.endFunction()
        
        
    
