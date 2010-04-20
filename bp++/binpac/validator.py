# $Id$
"""Validates the correctness of a BinPAC++ module"""

import ast
import module
import id
import type
import binpac.util as util

import hilti

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

    def currentModule(self):
        """Returns the module currently being validated.
        
        Returns: ~~Module - The module
        """
        return self._module
        
    def _validate(self):
        # Top-level driver of the validation process.
        self._errors = 0

        self._module.validate(self)
        
        return self._errors

