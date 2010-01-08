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
import binpac.core.id as id 
import binpac.core.scope as scope

import binpac.support.util as util

import hilti.core.type

_AllowedConstantTypes = (type.Bytes, type.RegExp)

class Field:
    """One field within a unit data type.
    
    name: string or None - The name of the fields, or None for anonymous
    fields.
    
    value: ~~Constant or None - The value for constant fields.
    
    type: ~~Type or None - The type of the field; can be None if *constant* is
    given.
    
    parent: ~~Unit - The unit type this field is part of.
    
    Todo: Only ~~Bytes constant are supported at the moment. Which other's do
    we want? (Regular expressions for sure.)
    """
    def __init__(self, name, value, type, pscope, parent, location=None):
        self._name = name
        self._type = type if type else value.type()
        self._value = value
        self._parent = parent
        self._location = location
        self._hooks = []
        
        self._scope = scope.Scope(None, pscope)
        self._scope.addID(id.Parameter("self", parent, location=location))
        
        assert self._type

    def location(self):
        """Returns the location associated with the constant.
        
        Returns: ~~Location - The location. 
        """
        return self._location        
        
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

    def scope(self):
        """Returns the scope for hook blocks.
        
        Returns: ~~Scope - The scope.
        """
        return self._scope
    
    def hook(self):
        """Returns the hook statements associated with the field.
        
        Returns: list of (~~Block, int) - The hook blocks with their
        priorities.
        """
        return self._hooks
        
    def addHook(self, stmt, priority):
        """Adds a hook statement to the field. The statement will be executed
        when the field has been fully parsed.
        
        stmt: ~~Block - The hook block.
        
        priority: int - The priority of the statement. If multiple statements
        are defined for the same field, they are executed in order of
        decreasing priority.
        """
        self._hooks += [(stmt, priority)]
            
    def production(self):
        """Returns a production to parse the field.
        
        Returns: ~~Production 
        """
        # FIXME: We hardcode the constant types we support here. Should do
        # that somewhere else. 
        if self._value:
            for t in _AllowedConstantTypes:
                if isinstance(self._type, t):
                    prod = grammar.Literal(self._name, self._value, location=self._location)
                    break
            else:
                util.internal_error("unexpected constant type for literal")

        else:
            prod = self._type.production()
            assert prod
            prod.setName(self._name)
            
        assert prod
        for (stmt, prio) in self._hooks:
            prod.addHook(stmt, prio)

        return prod

    def validate(self, vld):
        """Validates the semantic correctness of the field.
        
        vld: ~~Validator - The validator triggering the validation.
        """
        for (stmt, prio) in self._hooks:
            stmt.validate(vld)
        
        if not isinstance(self._type, type.ParseableType):
            # If the production function has not been overridden, we can't
            # use that type in a unit field. 
            vld.error(self, "type %s cannot be used inside a unit field" % self._type)
                
        if self._value:
            # White-list the types we can deal with in constants.
            for a in _AllowedConstantTypes:
                if isinstance(self._type, a):
                    break
            else:
                vld.error(self, "type %s cannot be used in a constant unit field" % self.type())

    def pac(self, printer):
        """Converts the field into parseable BinPAC++ code.

        printer: ~~Printer - The printer to use.
        """
        printer.output("<UnitField TODO>")
                
    def __str__(self):
        tag = "%s: " % self._name if self._name else ""
        return "%s%s" % (tag, self._type)

@type.pac("unit")
class Unit(type.ParseableType):
    """Type describing an individual parsing unit.
    
    A parsing unit is composed of (1) fields parsed from the traffic stream,
    which are then turned into a grammar; and (2) a number of "hooks", which
    are statements to be run on certain occasions (like when an error has been
    found). 

    location: ~~Location - A location object describing the point of definition.
    
    Todo: We need to document the available hooks.
    """

    _valid_hooks = ("ctor", "dtor", "error")
    
    def __init__(self, location=None):
        super(Unit, self).__init__(location=location)
        self._props = {}
        self._fields = {}
        self._fields_ordered = []
        self._hooks = {}

    def fields(self):
        """Returns the unit's fields. 
        
        Returns: list of ~~Field - The fields sorted in the order the field
        were added.
        """
        return self._fields_ordered

    def fieldType(self, name):
        """Returns the type of a field.
        
        name: string - The name of the field. 
        
        Returns: ~~Type - The type, or None if there's no field of this type.
        """
        try:
            return self._fields[name].type()
        except KeyError:
            return None
        
    def addField(self, field):
        """Adds a field to the unit type.
        
        field: ~~Field - The field type.
        """
        assert builtin_id(field._parent) == builtin_id(self)
        idx = field.name() if field.name() else str(builtin_id(field))
        self._fields[idx] = field
        self._fields_ordered += [field]
        
    def hooks(self, hook):
        """Returns all statements registered for a hook. 
        
        hook: string - The name of the hook to retrieve the statements for.
        
        Returns: list of (~~Block, int) - The hook blocks with their
        priorities.
        """
        assert hook in _valid_hooks
        return self._hooks.get(hook, [])
        
    def addHook(self, hook, stmt, priority):
        """Adds a hook statement to the unit. The statement will be executed
        as determined by the hook's semantics.
        
        hook: string - The name of the hook for the statement to be added.
        
        stmt: ~~Block - The hook block itself.
        
        priority: int - The priority of the statement. If multiple statements
        are defined for the same hook, they are executed in order of
        decreasing priority.
        """
        assert hook in _valid_hooks
        
        try:
            self._hooks[hook] += [(func, priority)]
        except IndexError:
            self._hooks[hook] = [(func, priority)]

    # Overridden from Type.
    def hiltiType(self, cg):
        mbuilder = cg.moduleBuilder()

        def _makeUnitCode():
            # Generate the parsing code for our unit.
            seq = [f.production() for f in self._fields_ordered]

            seq = grammar.Sequence(seq=seq, symbol="start", location=self.location())
            name = self._props.get("name", "unit")
            g = grammar.Grammar(name, seq)
            
            gen = pgen.ParserGen(cg)
            return gen.compile(g)
        
        return mbuilder.cache(self, _makeUnitCode)

    def validate(self, vld):
        for f in self._fields.values():
            f.validate(vld)

        for (stmt, prio) in self._hooks:
            stmt.validate(vld)
            
    def pac(self, printer):
        printer.output("<bytes type - TODO>")
        
    # Overridden from ParseableType.

    def supportedAttributes(self):
        return {}
    
    def production(self):
        # XXX
        pass
    
    def generateParser(self, cg, dst):
        pass
    
@operator.Attribute(Unit, type.Identifier)
class Attribute:
    def validate(vld, lhs, ident):
        ident = ident.constant().value()
        if not lhs.type().fieldType(ident):
            vld.error(lhs, "unknown unit attribute '%s'" % ident)
        
    def type(lhs, ident):
        ident = ident.constant().value()
        return lhs.type().fieldType(ident)
    
    def evaluate(cg, lhs, ident):
        ident = ident.constant().value()
        type = lhs.type().fieldType(ident)
        
        builder = cg.builder()
        tmp = builder.makeTmp(type.hiltiType(cg))
        builder.struct_get(tmp, lhs.evaluate(cg), builder.constOp(ident))
        return tmp
    
            
