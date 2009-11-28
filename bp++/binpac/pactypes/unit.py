# $Id$
#
# The unit type.

import inspect

builtin_id = id

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.grammar as grammar
import binpac.core.pgen as pgen
import binpac.core.operator as operator

import hilti.core.type

_AllowedConstantTypes = (type.Bytes, type.RegExp)

class Field:
    """One field within a unit data type.
    
    name: string or None - The name of the fields, or None for anonymous
    fields.
    
    value: ~~Constant or None - The value for constant fields.
    
    type: ~~Type or None - The type of the field; can be None if *constant* is
    given.
    
    Todo: Only ~~Bytes constant are supported at the moment. Which other's do
    we want? (Regular expressions for sure.)
    """
    def __init__(self, name, value, type, location=None):
        self._name = name
        self._type = type if type else value.type()
        self._value = value
        self._location = location
        assert self._type
        
    def name(self):
        """Returns the name of the field.
        
        Returns: string - The name.
        """
        return self._name

    def type(self):
        """Returns the name of the field.
        
        Returns: string - The name.
        """
        return self._type
    
    def value(self):
        """Returns the value for constant fields.
        
        Returns: ~~Constant or None - The constant, or None if not a constant
        field.
        """
        return self._value

    def production(self):
        """Returns a production to parse the field.
        
        Returns: ~~Production 
        """
        # FIXME: We hardcode the constant types we support here. Should do
        # that somewhere else. 
        if self._value:
            for t in _AllowedConstantTypes:
                if isinstance(self._type, t):
                    return grammar.Literal(self._name, self._value, location=self._location)
            
            util.internal_error("unexpected constant type for literal")

        else:
            prod = self._type.production()
            assert prod
            prod.setName(self._name)
            return prod
    
    def __str__(self):
        tag = "%s: " % self._name if self._name else ""
        return "%s%s" % self._type
    
@type.pac
class Unit(type.Type):
    """Type describing an individual parsing unit.
    
    A parsing unit is composed of (1) fields parsed from the traffic stream,
    which are then turned into a grammar; and (2) a number of "hooks", which
    are functions to be run on certain occastions (like when an error has been
    found). 
    
    fields: list of ~~Field - The unit's fields.

    location: ~~Location - A location object describing the point of definition.
    """

    valid_hooks = ("ctor", "dtor", "error")
    
    def __init__(self, fields, location=None):
        super(Unit, self).__init__(location=location)
        self._fields = fields
        self._hooks = {}

    def fields(self):
        """Returns the unit's fields.
        
        Returns: list of ~~Field - The fields.
        """
        return self._fields

    def hooks(self, hook):
        """Returns all functions registered for a hook. They are returned in
        order of decreasing priority, i.e., in the order in which they should
        be executed.
        
        hook: string - The name of the hook to retrieve the functions for.
        
        Returns: list of (func, priority) - The sorted list of functions.
        """
        return self._hooks.get(hook, []).sorted(lambda x, y: y[1]-x[1])
        
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
    
    def name(self):
        # Overridden from Type.
        return "unit"
    
    def toCode(self):
        # Overridden from Type.
        s = "unit {\n"
        for f in self._fields:
            s += "    %s;\n" % f.name()
        s = "}"
    
    def validate(self, vld):
        # Overridden from Type.
        for f in self._fields:
            if not f.type().hasProduction():
                # If the production function has not been overridden, we can't
                # use that type in a unit field. 
                vld.error(self, "type %s cannot be used inside a unit field" % f.type())
                
            if f.value():
                # White-list the types we can deal with in constants.
                for a in _AllowedConstantTypes:
                    if isinstance(f.type(), a):
                        break
                else:
                    vld.error(self, "type %s cannot be used in a constant unit field" % f.type())
        
    def hiltiType(self, cg, tag):
        # Overridden from Type.

        mbuilder = cg.moduleBuilder()

        def _makeUnitCode():
            # Generate the parsing code for our unit.
            seq = [f.production() for f in self._fields]
            seq = grammar.Sequence("self", seq, symbol="start", location=self.location())
            g = grammar.Grammar("%s" % tag, seq)
            
            gen = pgen.ParserGen(cg)
            return gen.compile(g)
        
        return mbuilder.cache(self, _makeUnitCode)

    #def production(self):
    #    # Overridden from Type.
    #    pass
    
            
