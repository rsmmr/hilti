# $Id$

import ast
import location

class Role:
    """The *Role* of an :class:`ID` separates IDs into several subgroups
    according to where they are defined."""

    UNKNOWN = 0
    """A :const:`UNKNOWN` role is a place-holder used during parsing when we
    don't know the role yet."""
    
    GLOBAL = 1
    """A :const:`GLOBAL` :class:`ID` is defined at module-level scope."""
    
    LOCAL = 2
    """A :const:`LOCAL` :class:`ID` is defined in a non-global scope."""
    
    PARAM = 3
    """An :const:`ARGUMENT` :class:`ID` is defined as a
    ~~Function  argument."""
    
class ID(ast.Node):
    """An ID binds a name to a type. *name* is a string. The name 
    may be qualified with a scope as in +scope::name+. *type* is an
    instance of ~~Type, and *role* is the IDs :class:`Role`.
    *location* gives a ~~Location to be associated with this
    Function.
    
    The class maps the ~~Visitor subtype to :meth:`~type`.
    """
    
    def __init__(self, name, type, role, location = None):
        assert name
        assert type
        
        self._name = name
        self._type = type
        self._role = role
        self._location = location

    def name(self):
        """Returns a string with the IDs name."""
        return self._name

    def setName(self, name): 
        """Sets the IDs name to the string *name*. The name may be qualified
        with a scope as in +scope::name+"""
        self._name =name
    
    def type(self):
        """Returns the ~~Type of this ID."""
        return self._type

    def role(self):
        """Returns the :class:`Role` of this ID."""
        return self._role
    
    def location(self):
        """Returns the ~~Location associated with
        this ID."""
        return self._location

    def qualified(self):
        """Returns True if the name is qualified."""
        return self._name.find("::") >= 0
    
    def splitScope(self): 
        """Return a tupel *(scope, name)* where scope is the qualified scope
        if there is any, and None otherwise; *name* the scope-less name. As
        scopes are generally considered case-insenstive, the returned *scope*
        is converted to lower-case.
        """
        i = self._name.find("::") 
        if i < 0:
            return (None, self._name)
        
        scope = self._name[0:i].lower()
        name = self._name[i+2:]
        return (scope, name)
    
    def __str__(self):
        return "%s %s" % (self._type, self._name)
    
    # Visitor support.
    def visitorSubType(self):
        return self._type
    

