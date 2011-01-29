# $Id$
"""Validates the correctness of a BinPAC++ module"""

import node
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
        self._funcs = [None]

    def error(self, context, msg):
        """Reports an error during code generation.

        msg: string - The error message to report.

        context: any - Optional object with a *location* method returning a
        ~~Location suitable to include into the error message.
        """
        loc = context.location() if context else None
        util.error(msg, component="validation", context=loc)
        self._errors += 1

    def currentModule(self):
        """Returns the module currently being validated.

        Returns: ~~Module - The module
        """
        return self._module

    def beginFunction(self, func):
        """Indicates that the start of function's validation.

        func: ~~function.Function - The function now being validated.
        """
        self._funcs += [func]

    def endFunction(self):
        """Indicates that the end of function's validation.
        """
        self._funcs += self._funcs[:-1]

    def currentFunction(self):
        """Returns the function currently being validated.

        Returns: ~~function.Function - The function, or None if we are not
        within a function.
        """
        return self._funcs[-1]

    def _validate(self):
        # Top-level driver of the validation process.
        self._errors = 0

        self._module.validate(self)

        return self._errors

