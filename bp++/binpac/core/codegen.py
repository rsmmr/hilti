# $Id$
"""Compiles a BinPAC++ module into a HILTI module."""

import hilti.checker
import hilti.codegen

import ast
import type
import id
import pgen

import binpac.support.util as util

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
        errors = hilti.checker.checkAST(hltmod)
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

    def debug(self):
        """Returns true if compiling in debugging mode. 
        
        Returns: bool - True if in debugging mode. 
        """
        return self._debug

    def beginFunction(self, name, ftype):
        """Starts a new function. The method creates the function builder as
        well as the initial block builder (which will be returned by
        subsequent calls to ~~builder). 
        
        When done building this function, ~~endFunction must be called.
        
        name: string - The name of the function.
        ftype: hilti.core.type.Function - The HILTI type of the
        function.
        
        Returns: tuple (~~hilti.core.builder.FunctionBuilder,
        ~~hilti.core.builder.BlockBuilder) - The builders created
        for the new function.
        """
        fbuilder = hilti.core.builder.FunctionBuilder(self._mbuilder, name, ftype)
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
        
        builder: hilti.core.hilti.BlockBuilder: The builder to set.
        """
        assert builder
        self._builders[-1] = builder
        
    def builder(self):
        """Returns the current block builder.
        
        Returns: hilti.core.builder.BlockBuilder - The block
        builder, as set by ~~setBuilder."""
        assert self._builders[-1]
        return self._builders[-1]

    def functionBuilder(self):
        """
        Returns the current function builder.
        
        Returns: hilti.core.builder.FunctionBuilder - The function
        builder.
        """
        fbuilder = self.builder().functionBuilder()
        assert fbuilder
        return fbuilder
    
    def moduleBuilder(self):
        """Returns the builder for the HILTI module currently being built.
        Must only be used after ~~compile() has been called.
        
        Returns: ~~hilti.core.builder.ModuleBuilder - The HILTI module builder.
        """
        assert self._mbuilder
        return self._mbuilder

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
        """Top-level driver of the compilation process."""
        hltmod = hilti.core.module.Module(self._module.name())
        self._mbuilder = hilti.core.builder.ModuleBuilder(hltmod)

        self._errors = 0

        paths = self.importPaths()
        hilti.parser.importModule(self._mbuilder.module(), "hilti", paths)
        hilti.parser.importModule(self._mbuilder.module(), "binpac", paths)
        hilti.parser.importModule(self._mbuilder.module(), "binpacintern", paths)
        
        for i in self._module.scope().IDs():
            if isinstance(i, id.Type) and isinstance(i.type(), type.Unit):
                if i.type().property("export").value():
                    gen = pgen.ParserGen(self)
                    grammar = i.type().grammar()
                    gen.compile(grammar)
                    gen.export(grammar)
            
            if isinstance(i, id.Global):
                if i.value():
                    default = i.type().hiltiConstant(self, i.value())
                else:
                    default = i.type().hiltiDefault(self)
                    
                val = hilti.core.instruction.ConstOperand(default) if default else None
                hltmod.addID(hilti.core.id.ID(i.name(), i.type().hiltiType(self), hilti.core.id.Role.GLOBAL), val)

        self._initFunction()
                
        self._errors += self._mbuilder.finish(validate=False)

        return self._errors == 0
    
    def _initFunction(self):
        """Generates HILTI function initializing the module."""
        
        ftype = hilti.core.type.Function([], hilti.core.type.Void())
        name = "init_%s" % self._module.name()
        (fbuilder, builder) = self.beginFunction(name, ftype)
        fbuilder.function().setLinkage(hilti.core.function.Linkage.INIT)

        for stmt in self._module.statements():
            stmt.execute(self)
        
        self.endFunction()
        
    
