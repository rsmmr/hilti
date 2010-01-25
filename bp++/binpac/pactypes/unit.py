# $Id$
#
# The unit type.

import inspect

builtin_id = id

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.stmt as stmt
import binpac.core.grammar as grammar
import binpac.core.pgen as pgen
import binpac.core.operator as operator
import binpac.core.id as id 
import binpac.core.scope as scope
import binpac.core.constant as constant

import binpac.support.util as util

import hilti.core.type

_AllowedConstantTypes = (type.Bytes, type.RegExp)

class Field:
    """One field within a unit data type.
    
    name: string or None - The name of the fields, or None for anonymous
    fields.
    
    value: ~~Constant or None - The value for constant fields.
    
    ty: ~~Type or None - The type of the field; can be None if *constant* is
    given.
    
    parent: ~~Unit - The unit type this field is part of.
    
    params: list of ~~Expr - If *type* is a unit type, the parameters passed to
    that on construction; if not (or if the unit type doesn't take any
    parameters), an empty list.
    
    Todo: Only ~~Bytes constant are supported at the moment. Which other's do
    we want? (Regular expressions for sure.)
    """
    def __init__(self, name, value, ty, parent, params=[], location=None):
        self._name = name
        self._type = ty if ty else value.type()
        self._value = value
        self._parent = parent
        self._location = location
        self._hooks = []
        self._ctlhook = None
        self._params = params

        assert self._type
        
        if isinstance(ty, type.ParseableType):
            ty.initParser(self)

    def location(self):
        """Returns the location associated with the constant.
        
        Returns: ~~Location - The location. 
        """
        return self._location        

    def setLocation(self, location):
        """XXXX"""
        self._location = location
    
    def parent(self):
        """Returns the parent type this field is part of.
        
        Returns: ~~Unit - The parent type.
        """
        return self._parent
    
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
    
    def parsedType(self):
        """XXX"""
        return self._type.parsedType()
    
    def value(self):
        """Returns the value for constant fields.
        
        Returns: ~~Constant or None - The constant, or None if not a constant
        field.
        """
        return self._value

    def params(self):
        """Returns the parameters passed to the sub-type. Only relevant if
        *type* is also a ~~Unit type. 
        
        Returns: list of ~~Expr - The parameters; empty list for no
        parameters.
        """
        return self._params
    
    def hooks(self):
        """Returns the hook statements associated with the field.
        
        Returns: list of ~~FieldHook - The hooks. 
        """
        return self._hooks
        
    def addHook(self, hook):
        """Adds a hook statement to the field. The statement will be executed
        when the field has been fully parsed.
        
        hook: ~~FieldHook - The hook.
        
        Note: The control hook set via ~~setControlHook must not be added via
        this method.
        """
        self._hooks += [hook]

    def setControlHook(self, hook):
        """XXX"""
        assert isinstance(hook, stmt.FieldControlHook)
        self._ctlhook = hook

    def controlHook(self):
        """XXX"""
        return self._ctlhook
        
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
            prod = self._type.production(self)
            assert prod
            prod.setName(self._name)
            prod.setType(self._type)
            
            if self._params:
                assert isinstance(prod, grammar.ChildGrammar)
                prod.setParams(self._params)

        assert prod
        
        # We add the hooks to a concatened epsilon production. If we woudl add
        # them to the returned production, they might end up being executed
        # multiple times if that contains some kind of loop of itself.
        eps = grammar.Epsilon()
        prod = grammar.Sequence(seq=[prod, eps])
                
        for hook in self._hooks:
            eps.addHook(hook)
            
        return prod

    def validate(self, vld):
        """Validates the semantic correctness of the field.
        
        vld: ~~Validator - The validator triggering the validation.
        """
        for hook in self._hooks:
            hook.validate(vld)
        
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

        if isinstance(self._type, Unit):
            if len(self._params) != len(self._type.params()):
                vld.error(self, "number of unit parameters do not match (have %d, but expected %d)" % (len(self._params), len(self._type.params())))

            i = 0
            for (have, want) in zip(self._params, self._type.params()):
                i += 1
                if have.type() != want.type():
                    vld.error(self, "unit parameter %d mismatch: is %s but need %s" % (have.type(), want.type()))

        else:
            if len(self._params):
                vld.error(self, "type does not receive any parameters")
                    
    def pac(self, printer):
        """Converts the field into parseable BinPAC++ code.

        printer: ~~Printer - The printer to use.
        """
        printer.output("<UnitField TODO>")

    def resolve(self, resolver):
        """See ~~Type.resolve."""
        if resolver.already(self):
            return

        self._type = self._type.resolve(resolver)
        return self
        
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

    pscope: ~~Scope - The parent scope for this unit. 

    params: list of ~~ID - Optional parameters for the unit type. 
    
    location: ~~Location - A location object describing the point of definition.
    
    Todo: We need to document the available hooks.
    """

    _valid_hooks = ("ctor", "dtor", "error")

    def __init__(self, pscope, params=[], location=None):
        Unit._counter += 1
        super(Unit, self).__init__(location=location)
        self._props = {}
        self._fields = {}
        self._fields_ordered = []
        self._hooks = {}
        self._prod = None 
        self._grammar = None
        self._params = params
        self._scope = scope.Scope(None, pscope)
        
        self._scope.addID(id.Parameter("self", self, location=location))
        for p in params:
            self._scope.addID(p)
        
    def name(self):
        """Returns the name of the unit. A default is automatically chosen,
        but a user-defined name can override the default via the ``.name``
        property.
        
        Returns: string - The name of the unit. 
        
        Note: This overrides ~~type.Type.name().
        """
        return self.property("name").value()

    def scope(self):
        """Returns the scope for the unit.
        
        Returns: ~~Scope - The scope.
        """
        return self._scope
    
    def fields(self):
        """Returns the unit's fields. 
        
        Returns: list of ~~Field - The fields sorted in the order the field
        were added.
        """
        return self._fields_ordered

    def params(self):
        """Returns the unit's parameters.
        
        Returns: list of ~~ID - The parameter; empty list for no parameters.
        """
        return self._params
    
    def fieldType(self, name):
        """Returns the type of a field.
        
        name: string - The name of the field. 
        
        Returns: ~~Type - The type, or None if there's no field of this type.
        """
        try:
            return self._fields[name].type()
        except KeyError:
            return None
            
    def parsedFieldType(self, name):
        """
        XXX
        """
        try:
            return self._fields[name].parsedType()
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
        
    def hooks(self, name):
        """Returns all hooks registered for a hook name. 
        
        name: string - The name of the hook to retrieve the hooks for.
        
        Returns: list of ~~FieldHook - The hooks with their priorities.
        """
        assert name in _valid_hooks
        return self._hooks.get(name, [])
        
    def addHook(self, name, hook, priority):
        """Adds a hook statement to the unit. The statement will be executed
        as determined by the hook's semantics.
        
        name: string - The name of the hook for the statement to be added.
        
        hook: ~~Block - The hook itself.
        """
        assert name in _valid_hooks
        
        try:
            self._hooks[name] += [hook]
        except IndexError:
            self._hooks[name] = [hook]

    def allProperties(self):
        """Returns a list of all recognized properties.
        
        Returns: dict mapping string to ~~Constant - For each allowed
        property, there is one entry under it's name (excluding the leading
        dot) mapping to its default value.
        """
        return { "export": constant.Constant(False, type.Bool()),
                 "name": constant.Constant(self._name, type.String())
               }

    def property(self, name):
        """Returns the value of property. The property name must be one of
        those returned by ~~allProperties.
        
        name: string - The name of the property. 
        
        Returns: ~~Constant - The value of the property. If not explicity
        set, the default value is returned. The returned constant will be of
        the same type as that of the default value returned by ~~allProperties. 
        """
        try:
            default = self.allProperties()[name]
        except IndexError:
            util.internal_error("unknown property '%s'" % name)

        return self._props.get(name, default)
        
    def setProperty(self, name, constant):
        """Sets the value of a property. The property name must be one of
        those returned by ~~allProperties. 
        
        name: string - The name of the property. 
        value: any - The value to set it to. The type must correspond to what
        ~~allProperties specifies for the default value. 
        
        Raises: ValueError if ``name`` is not a valid property; and TypeError
        if the type of *constant* is not correct. 
        """
        try:
            default = self.allProperties()[name]
        except:
            raise ValueError(name)
        
        if not constant.type() == default.type():
            raise TypeError(default.type())
            
        self._props[name] = constant
    
    def grammar(self):
        """Returns the grammar for this type."""
        if not self._grammar:
            seq = [f.production() for f in self._fields_ordered]
            seq = grammar.Sequence(seq=seq, type=self, symbol="start_%s" % self.name(), location=self.location())
            self._grammar = grammar.Grammar(self.name(), seq, self._params, location=self.location())
            
        return self._grammar
            
    # Overridden from Type.
    
    def hiltiType(self, cg):
        mbuilder = cg.moduleBuilder()

        def _makeType():
            # Generate the parsing code for our unit.
            gen = pgen.ParserGen(cg)
            return gen.objectType(self.grammar())
        
        return mbuilder.cache(self, _makeType)

    def validate(self, vld):
        
        error = self.grammar().check()
        if error:
            vld.error(self, error)
        
        for f in self._fields.values():
            f.validate(vld)

        for hook in self._hooks:
            hook.validate(vld)
            
    def pac(self, printer):
        printer.output("<bytes type - TODO>")

    def resolve(self, resolver):
        if resolver.already(self):
            return self
        
        for param in self._params:
            param.resolve(resolver)

        for f in self._fields.values():
            f.resolve(resolver)

        return self
            
    # Overridden from ParseableType.

    def supportedAttributes(self):
        return {}

    def production(self, field):
        #seq = [f.production() for f in self._fields_ordered]
        #seq = grammar.Sequence(seq=seq, type=self, symbol="start-%s" % self.name(), location=self.location())
        #return seq
        return grammar.ChildGrammar(self, location=self.location())

    def generateParser(self, cg, dst):
        # This will not be called because we handle the grammar.ChildGrammar
        # case separetely in the parser generator. 
        util.internal_error("cannot be reached")
    
@operator.Attribute(Unit, type.Identifier)
class Attribute:
    def validate(vld, lhs, ident):
        ident = ident.constant().value()
        if not lhs.type().parsedFieldType(ident):
            vld.error(lhs, "unknown unit attribute '%s'" % ident)
        
    def type(lhs, ident):
        ident = ident.constant().value()
        return lhs.type().parsedFieldType(ident)
    
    def evaluate(cg, lhs, ident):
        ident = ident.constant().value()
        type = lhs.type().parsedFieldType(ident)
        
        builder = cg.builder()
        tmp = builder.addTmp("__attr", type.hiltiType(cg))
        builder.struct_get(tmp, lhs.evaluate(cg), builder.constOp(ident))
        return tmp
    
            
