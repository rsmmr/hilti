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
    
    LOCAL = 2
    """A ~~LOCAL ~~ID is defined inside another entity such as a function."""
    
    PARAM = 3
    """An ~~PARAM :class:`ID` is defined as a function argument."""
    
class ID(ast.Node):
    """Binds a name to a type.
    
    name: string - The name of the ID.
    
    type: ~~Type - The type of the ID.
    
    role: ~~Role - The role of the ID.
    
    location: ~~Location - A location to be associated with the ID. 
    
    Note: The class maps the ~~Visitor subtype to :meth:`~type`.
    """
    
    def __init__(self, name, type, role, location = None):
        assert name
        assert type
        
        super(ID, self).__init__(location)
        self._name = name
        self._type = type
        self._role = role
        self._location = location

    def name(self):
        """Returns the ID's name.
        
        Returns: string - The ID's name.
        """
        return self._name

    def setName(self, name): 
        """Sets the ID's name. The name may be qualified with a scope (as in
        ``scope::name``).
        
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
    
    def qualified(self):
        """Returns whether the ID's name is qualified.
        
        Returns: bool - True if name is qualified.
        """
        return self._name.find("::") >= 0
    
    def splitScope(self): 
        """Splits the ID's name into scope and local part. 
        
        Returns: tuple (scope, local) - If ID's name if qualified, *scope* is
        the ID's scope, and *local* the ID's local part; as in HILTI scopes
        are generally matched case-insensitive, *scope* will be lower-cased. 
        If the ID's name is not qualified, *scope* will be None and *local*
        the name.
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
    

