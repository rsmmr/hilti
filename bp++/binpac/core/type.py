# $Id$

class Type(object):
    """Base class for all data types provided by the BinPAC++ language.  

    Any class derived from Type must include a class-variable ``_name``
    containing a string that is suitable for use in error messages to
    describes the HILTI type that the class is representing.
    
    name: string - The name of type in a readable form suited to present to
    the user (e.g., in error messages and debugging output).
        
    docname: string - Optional string which, if given, will be used in the
    automatically generated instruction documentation instead of *name*; it is
    not used anywhere else than in the documentation.
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, name, docname=None, location=None):
        self._name = name
        self._docname = docname if docname else name
    
    def name(self):
        """Returns the name of the type.
        
        Returns: string - The name of the type.
        """
        return self._name
    
    def docname(self):
        """Returns the name of the type as used in the instruction
        documentation.
        
        Returns: string - The documentation name of the type.
        """
        return self._docname

    def location(self):
        """Returns the location where the type was defined.
        
        Returns: ~~Location - The location.
        """
        return self._location
        
    
    def __str__(self):
        return self.name()
    
    _name = "type"

class Integer(Type):
    """Base type for integers.  
    
    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, name, width, location=None):
        super(Integer, self).__init__(name, location=location)
        assert(width > 0 and width <= 64)
        self._width = width

    def width(self):
        """Returns the bit-width of the type's integers.
        
        Returns: int - The number of bits available to represent integers of
        this type. 
        """
        return self._width

    def setWidth(self, width):
        """Sets the bit-width of the type's integers.
        
        width: int - The new bit-width. 
        """
        self._width = width
    
    _name = "<integer>"

class SignedInteger(Integer):
    """Type for signed integers.  
    
    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(SignedInteger, self).__init__(SignedInteger._name, width, location=location)
        
    _name = "int"

class UnsignedInteger(Integer):
    """Type for unsigned integers.  
    
    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(UnsignedInteger, self).__init__(UnsignedInteger._name, width, location=location)
        
    _name = "uint"
    
class Bytes(Type):
    """Type for bytes objects.  
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Bytes, self).__init__(Bytes._name, location=location)

    _name = "bytes"

class TypeDecl(Type):
    """Type for type declarations.  
    
    t: ~~Type - The declared type.
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(TypeDecl, self).__init__(Bytes._name, location=location)
        
    def __str__(self):
        return "<type-decl>" 
    
    _name = "type-decl"
    
class Unit(Type):
    """Type describing an individual parsing unit.
    
    A parsing unit is composed of (1) fields parsed from the traffic
    stream, which are defined in the form of a grammar; (2)
    attributes which are stored along with the parsed fields and can
    be manipulated by user code; and (3) a number of "hooks", which
    are functions to be run on certain occastions (like when an
    error has been found). 
    
    prod: ~~Production - Optional start production of the grammar defining
    how the unit's fields are parsed.

    location: ~~Location - A location object describing the point of definition.
    """

    valid_hooks = ("ctor", "dtor", "error")
    
    def __init__(self, prod=None, location=None):
        super(Unit, self).__init__(self._name, location=location)
        self._prod = prod
        self._attrs = []
        self._hooks = {}

    def attributes(self):
        """Returns the attributes of the unit.
        
        Returns: list of ~~ID - The attributes.
        """
        return self._attrs
    
    def startProduction(self):
        """Returns the start production of the grammar defining the unit's
        fields.
        
        Returns: ~~Production or None - The production, or None if none has
        been set. 
        """
        return self._prod
    
    def hooks(self, hook):
        """Returns all functions registered for a hook. They are returned in
        order of decreasing priority, i.e., in the order in which they should
        be executed.
        
        hook: string - The name of the hook to retrieve the functions for.
        
        Returns: list of (func, priority) - The sorted list of functions.
        """
        return self._hooks.get(hook, []).sorted(lambda x, y: y[1]-x[1])
        
    def setStartProduction(self, prod):
        """Sets the start production of the grammar defining how the unit's
        fields are parsed.
        
        prod: ~~Production - The start production.
        """
        self._prod = prod
        
    def addAttribute(self, id):
        """Adds an attribute to the unit.
        
        id: ~~ID - The attribute.
        """
        self._attrs += [id]
        
    def addHook(self, hook, func, priority):
        """Adds a hook function to the unit. Hook functions are called when
        certain events happen. 
        
        hook: string - The name of the hook for the function to be added.
        func: ~~Function - The hook function itself.
        priority: int - The priority of the function. If multiple functions
        are defined for the same hook, they are executed in order of
        decreasing priority.
        """
        
        assert hook in _valid_hooks
        try:
            self._hooks[hook] += [(func, priority)]
        except IndexError:
            self._hooks[hook] = [(func, priority)]

    def __str__(self):
        attrs = [str(a) for a in self._attrs]
        hooks = self._hooks.items()
        return """unit
    grammar:    %s
    attributes: %s 
    hooks:      %s""" % (self._prod, ", ".join(attrs), ", ".join(hooks))
    
    _name = "unit"
        
