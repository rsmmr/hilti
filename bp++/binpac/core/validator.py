# $Id$
"""Validates the correctness of a BinPAC++ module"""

import ast
import module
import id
import type
import binpac.support.util as util

import hilti.checker

def validateModule(mod):
    """Validates the semantic correctness of  a BinPAC++ module.
    
    mod: ~~Module - The module to compile. 
    
    Returns: integer - Number of errors found. 
    """
    validator = Validator(mod)
    return validator._validate()

class Validator(object):
    def __init__(self, mod):
        """Main validator class. See ~~validateModule for arguments."""
        self._module = mod
        self._errors = 0

    def error(self, context, msg):
        """Reports an error during code generation.
        
        msg: string - The error message to report.
        
        context: any - Optional object with a *location* method returning a
        ~~Location suitable to include into the error message.
        """
        loc = context.location() if context else None
        util.error(msg, component="codegen", context=loc)
        self._errors += 1

    def _validate(self):
        # Top-level driver of the validation process.
        self._errors = 0
        
        for id in self._module.IDs():
            t = id.type()
            if isinstance(id.type(), type.TypeDecl):
                id.type().declType().validate(self)
        
        if self._errors > 0:
            return self._errors
        
        errors = hilti.checker.checkAST(self._module)                                                                                                                   
        
        return errors

