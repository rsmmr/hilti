# $Id$

import binpac.util as util
import binpac.id as id

class Resolver(object):
    """Class that resolves all still unresolved identifiers in a module."""
    def __init__(self):
        pass

    def error(self, context, msg):
        """Reports an error during resolving.

        msg: string - The error message to report.

        context: any - Optional object with a *location* method returning a
        ~~Location suitable to include into the error message.
        """
        loc = context.location() if context else None
        util.error(msg, component="codegen", context=loc)
        self._errors += 1

    def resolve(self, mod):
        """Resolves all still unresolved identifiers in module. Any errors are
        directly reported to the user.

        mod: ~~Module  - The module.

        Returns: int - The integer is the number of errors found during resolving.
        """
        self._errors = 0
        self._scope = mod.scope()
        mod.resolve(self)
        self._scope = None

        return self._errors

    def scope(self):
        """Returns the scope in which unknown identifiers should be resolved.
        Only valid while ~~resolve is running.

        Returns: ~~Scope - The scope, or None if ~~resolve is not running.
        """
        return self._scope

    def lookupTypeID(self, ty):
        """XXXX"""
        # Before we resolve the type, let's see if it's actually refering
        # to a constanst.
        i = self.scope().lookupID(ty.idName())

        if i and isinstance(i, id.Constant):
            val = i.expr()
            ty = i.expr().type()
        else:
            val = None
            ty = ty.resolve(self)

        return (val, ty)

    def already(self, node):
        """Checks whether a node has already been resolved. The first time
        this method is called for a *node*, it returns True. If called
        susequently for the same *node*, it returns False.

        Note: This function must be used when overiding ~~Node.resolve; see
        there.
        """
        try:
            return node.__already
        except AttributeError:
            pass

        node.__already = 1
        return False



