# $Id$

import location

class Role:
    """The *Role* of an :class:`ID` separates IDs into several subgroups
    according to where they are defined."""

    GLOBAL = 1
    """A :const:`GLOBAL` :class:`ID` is defined at module-level scope."""
    
    LOCAL = 2
    """A :const:`LOCAL` :class:`ID` is defined in a non-global scope."""
    
    PARAM = 3
    """An :const:`ARGUMENT` :class:`ID` is defined as a
    :class:`~hilti.core.function.Function`  argument."""
    
class ID(object):
    """An ID binds a name to a type. *name* is a string, and *type* is an
    instance of :class:`~hilti.core.type.Type`. *role* is the IDs
    :class:`Role`, and *location* gives a
    :class:`~hilti.core.location.Location` to be associated with this
    Function.
    
    This class implements :class:`~hilti.core.visitor.Visitor` support and
    maps the subtype to :meth:`~type`.
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
        """Sets the IDs name to the string *name*."""
        self._name = name
    
    def type(self):
        """Returns the :class:`~hilti.core.type.Type` of this ID."""
        return self._type

    def role(self):
        """Returns the :class:`Role` of this ID."""
        return self._role
    
    def location(self):
        """Returns the :class:`~hilti.core.location.Location` associated with
        this ID."""
        return self._location

    # Visitor support.
    def dispatch(self, visitor):
        visitor.visit(self)
        
    def visitorSubClass(self):
        return self._type
    
    def __str__(self):
        return "%s%s" % (self._name, self._type)

