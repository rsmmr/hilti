# $Id$
"""Compiles a BinPAC++ module into a HILTI module."""

import hilti.checker
import hilti.codegen

import ast
import type
import id

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

    def debug(self):
        """Returns true if compiling in debugging mode. 
        
        Returns: bool - True if in debugging mode. 
        """
        return self._debug
    
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
        # Top-level driver of the compilation process.
        hltmod = hilti.core.module.Module(self._module.name())
        self._mbuilder = hilti.core.builder.ModuleBuilder(hltmod)
        
        self._errors = 0

        for i in self._module.IDs():
            if isinstance(i.type(), type.TypeDecl) and i.linkage() == id.Linkage.EXPORTED:
                # Make sure all necessary HILTI code for this type is generated.
                i.type().declType().hiltiType(self, i.name())
        
        self._errors += self._mbuilder.finish(validate=False)

        return self._errors == 0
