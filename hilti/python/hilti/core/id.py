# $Id$

import ast
import location

class Role:
    """The *Role* of an ~~ID separates IDs into several subgroups
    according to where they are defined."""

    UNKNOWN = 0
    """An ~~UNKNOWN` role is a place-holder used during parsing when we
    don't know an ~~ID's role yet."""
    
    GLOBAL = 1
    """A ~~GLOBAL ~~ID is defined at module-level scope."""
    
    CONST = 2
    """A ~~CONST ~~ID is defined at module-level scope and represents a
    constant value."""
    
    LOCAL = 3
    """A ~~LOCAL ~~ID is defined inside another entity such as a function."""
    
    PARAM = 4
    """An ~~PARAM :class:`ID` is defined as a function argument."""
    
class ID(ast.Node):
    """Binds a name to a type.
    
    name: string - The name of the ID.
    
    type: ~~Type - The type of the ID.
    
    role: ~~Role - The role of the ID.
    
    imported: bool - True to indicate that this ID was imported from another
    module. This does not change the semantics of the ID in any way but can be
    used to avoid importing it recursively at some later time.
    
    scope: string - An optional scope of the ID, i.e., the name of the module
    in which it is defined. 
    
    location: ~~Location - A location to be associated with the ID. 
    
    Note: The class maps the ~~Visitor subtype to :meth:`~type`.
    """
    
    def __init__(self, name, type, role, scope=None, location = None, imported=False):
        assert name
        assert type
        
        super(ID, self).__init__(location)
        self._name = name
        self._scope = scope.lower() if scope else None
        self._type = type
        self._role = role
        self._imported = imported
        self._location = location

    def name(self):
        """Returns the ID's name.
        
        Returns: string - The ID's name.
        """
        return self._name

    def scope(self):
        """Returns the ID's scope.
        
        Returns: string - The ID's scope, or None if it does not have one. The
        scope will be lower-case.
        """
        return self._scope
    
    def setScope(self, scope):
        """Sets the ID's scope.
        
        scope: string - The new scope.
        """
        self._scope = scope.lower() if scope else None
        
    def setName(self, name): 
        """Sets the ID's name.
        
        name: string - The new name.
        """
        self._name =name
    
    def type(self):
        """Returns the ID's type.
        
        Returns: ~~Type - The ID's type.
        """
        return self._type

    def role(self):
        """Returns the ID's role.
        
        Returns: ~~Role - The ID's role.
        """
        return self._role

    def imported(self):
        """Returns whether the ID was imported from another module.
        
        Returns: bool - True if the ID was imported.
        """
        return self._imported
    
    def __str__(self):
        if self._scope:
            return "%s %s::%s" % (self._type, self._scope, self._name)
        else:
            return "%s %s" % (self._type, self._name)
    
    # Visitor support.
    def visitorSubType(self):
        return self._type
    

