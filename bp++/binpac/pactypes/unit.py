# $Id$
#
# The unit type.

import inspect

builtin_id = id

import binpac.type as type
import binpac.expr as expr
import binpac.stmt as stmt
import binpac.grammar as grammar
import binpac.pgen as pgen
import binpac.operator as operator
import binpac.id as id 
import binpac.scope as scope
import binpac.constant as constant

import binpac.util as util

import hilti.type

_AllowedConstantTypes = (type.Bytes, type.RegExp)

class Field(object):
    """One field within a unit data type.
    
    name: string or None - The name of the fields, or None for anonymous
    fields.
    
    value: ~~Constant or None - The value for constant fields.
    
    ty: ~~Type or None - The type of the field; can be None if
    *constant* is given. It can also be none for 'umbrella' fields
    that never get converted into an actual parsed field themselves
    (e.g., the ~~SwitchField).
    
    parent: ~~Unit - The unit type this field is part of.
    
    params: list of ~~Expr - If *type* is a unit type, the parameters passed to
    that on construction; if not (or if the unit type doesn't take any
    parameters), an empty list.
    
    Todo: Only ~~Bytes constant are supported at the moment. Which other's do
    we want? (Regular expressions for sure.)
    """
    def __init__(self, name, value, ty, parent, params=[], location=None):
        self._name = name
        self._type = ty if ty else (value.type() if value else None)
        self._value = value
        self._parent = parent
        self._location = location
        self._hooks = []
        self._ctlhook = None
        self._cond = None
        self._params = params
        self._noid = False

        if isinstance(ty, type.ParseableType):
            ty.initParser(self)

    def location(self):
        """Returns the location associated with the constant.
        
        Returns: ~~Location - The location. 
        """
        return self._location        

    def setLocation(self, location):
        """Set the location assocated with this constant.
        
        location: ~~Location - The location.
        """
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
        """Returns the type of values parsed by this type. Forwards simply to
        ~~Type.parsedType for the type returned by ~~type.
        
        Returns: ~~Type - The type of parsed values.
        """
        return self._type
    
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
        
        Returns: list of ~~UnitHook - The hooks. 
        """
        return self._hooks
        
    def addHook(self, hook):
        """Adds a hook statement to the field. The statement will be executed
        when the field has been fully parsed.
        
        hook: ~~UnitHook - The hook.
        
        Note: The control hook set via ~~setControlHook must not be added via
        this method.
        """
        self._hooks += [hook]

    def setControlHook(self, hook):
        """Sets the control hook for the field. The hook will be exectuted
        when the field has been fully parsed. Different from \"normal\" hooks,
        the control hook defines the ``$$`` identifier for the field's
        attribute expressions.
        
        hook: ~~FieldControlHook - The hook.
        """
        assert isinstance(hook, stmt.FieldControlHook)
        self._ctlhook = hook

    def controlHook(self):
        """Returns the field's control hook. The hook will be exectuted when
        the field has been fully parsed. Different from \"normal\" hooks, the
        control hook defines the ``$$`` identifier for the field's attribute
        expressions.
        
        Returns:  ~~FieldControlHook - The hook.
        """
        return self._ctlhook

    def condition(self, cond):
        """Returns any boolean condidition associated with the field.
        
        Returns: ~~Expression: The condition, or None if none has been set. 
        """
        return self._cond
    
    def setCondition(self, cond):
        """Associates a boolean condition with the field. The field will only
        be parsed if the condition evaluates to True.
        
        cond: ~~Expression: The condition, which must evaluate to a ~~Bool.
        """
        self._cond = cond

    def setNoID(self):
        """Sets a flags indicating the HILTI struct for the parent union
        should not contain an item for this field. This can be used if
        another field already creates an item of the same name (and type).
        """
        self._noid = True
        
    def isNoID(self):
        """Returns whether the HILTI struct for the parent union should
        contain an item for this field. The default is yes but that can be
        changed with ~~setNoID.
        
        Returns: bool - True if no item should be created.
        """
        return self._noid
        
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
            
            if self._noid:
                prod.setNoID()
            
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
            
        if self._cond:
            prod = grammar.Boolean(self._cond, prod, grammar.Epsilon())
            
        return prod

    def validate(self, vld):
        """Validates the semantic correctness of the field.
        
        vld: ~~Validator - The validator triggering the validation.
        """
        for hook in self._hooks:
            hook.validate(vld)
        
        if self._type:
            if not isinstance(self._type, type.ParseableType):
                # If the production function has not been overridden, we can't
                # use that type in a unit field. 
                vld.error(self, "type %s cannot be used inside a unit field" % self._type)

            self._type.validateInUnit(vld)
        
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
                
        if self._cond and not isinstance(self._cond.type(), type.Bool):
            vld.error(self, "field condition must be boolean")
            
        convert = self.type().attributeExpr("convert")
        if convert:
            fid = vld.currentModule().scope().lookupID(convert.name())
            if fid:
                funcs = fid.function().matchFunctions([self.parsedType()])
                if len(funcs) == 0:
                    vld.error("no matching function for &convert found")
                
                if len(funcs) > 1:
                    vld.error("&convert function is ambigious")
        else:
            vld.error(self, "unknown &convert function")
                    
    def pac(self, printer):
        """Converts the field into parseable BinPAC++ code.

        printer: ~~Printer - The printer to use.
        """
        printer.output("<UnitField TODO>")

    def resolve(self, resolver):
        """See ~~Type.resolve."""
        if resolver.already(self):
            return
        
        if self._type:
            self._type = self._type.resolve(resolver)
        
        if self._cond:
            self._cond.resolve(resolver)
        
        return self
        
    def __str__(self):
        tag = "%s: " % self._name if self._name else ""
        return "%s%s" % (tag, self._type)

class SwitchField(Field):
    """An umbrella field for a switch construct. This field contains all of
    the switch's cases as sub-fields, and it is in charge of generating their
    productions. The cases must be added via ~~addCase, not directly to the
    parent ~~Unit. 
    
    expr: ~~Expression - The switch condition.
    
    parent: ~~Unit - The unit type this field is part of.
    
    location: ~~Location - Location information for the switch.
    """
    def __init__(self, expr, parent, location=None):
        super(SwitchField, self).__init__("<switch>", None, None, parent, location=location)
        self._expr = expr
        self._cases = []

    def addCase(self, case):
        """Adds a case to the switch.
        
        case: ~~SwitchFieldCase - The case.
        """
        self._cases += [case]
        self.parent().addField(case)

    ### Overridden from Field.
        
    def production(self):
        grammar_cases = []
        default = None
        
        for case in self._cases:
            if case.isDefault():
                default = case.production()
                
            else:
                grammar_cases += [(case.expr(), case.production())]
                
        return grammar.Switch(self._expr, grammar_cases, default, symbol="switch", location=self.location())
    
    def resolve(self, resolver):
        super(SwitchField, self).resolve(resolver)
        self._expr.resolve(resolver)
        
        # Having multiple cases with the same field name is ok if they are
        # of the same type. In that case, make sure that only one of them
        # actually ends up in the HILTI struct.  Note that error reporting for
        # non-matching types will be done in validate.
        names = {}
        for field in self._cases:
            name = field.name()
            if not name in names:
                names[name] = field.type()
            elif field.type() == names[name]:
                field.setNoID()
        
    def validate(self, vld):
        super(SwitchField, self).validate(vld)
        
        self._expr.validate(vld)
        
        defaults = 0
        values = []
        for case in self._cases:
            if case.isDefault():
                defaults += 1
                continue
            
            if not case.expr():
                util.internal_error("no expression for switch case")
            
            if not case.expr().canCastTo(self._expr.type()):
                vld.error(self, "case expression is of type %s, but must be %s" % (case.expr().type(), self._expr.type()))
                
            if case.expr().isConst():
                c = case.expr().constant()
                if c.value() in values:
                    vld.error(self, "case '%s' defined more than once" % c.value())
                else:
                    values += [c.value()]
                
        if defaults > 1:
            vld.error(self, "more than one default case")

    def pac(self, printer):
        printer.output("<SwitchFieldCase TODO>")

    def __str__(self):
        return "<__str__ SwitchFieldCase TODO>"

class SubField(Field):
    """An abstract base class for fields that are part of another 'umbrealla'
    field. A ~~Unit will not directly create productions/IDs for SubFields,
    but delegate that to the umbrella field.
    """
    pass
    
class SwitchFieldCase(SubField):
    """A SubField for the cases of a switch. Instances of this class must be
    added to their umbrella ~~SwitchField, not directly to the ~~Unit they are
    part of. After an instance has been created, ~~setExpr must be called to
    set the case's expression (including for the default case). 
    
    See ~~Field for parameters.
    """
    def __init__(self, name, value, ty, parent, params=[], location=None):
        super(SwitchFieldCase, self).__init__(name, value, ty, parent, params=params, location=location)
        self._default = False
        self._expr = None

    def isDefault(self):
        """Returns whether this field is the default branch.
        
        Returns: Bool - True if the field is the default.
        """
        return self._default
        
    def expr(self):
        """Returns the expression associated with the case.         
        
        Returns: ~~Expression - The expression, or None for the default branch.
        """
        return self._expr
        
    def setExpr(self, expr):
        """Sets the expression associated with the field. This must be called
        once for every case (including the default case).
        
        expr: ~~Expression - The expression, or None to mark the case as the
        default.
        """
        self._expr = expr
        
        if not self._expr:
            self._default = True

    ### Overridden from Field.
        
    def resolve(self, resolver):
        super(SwitchFieldCase, self).resolve(resolver)
        if self._expr:
            self._expr.resolve(resolver)

    def validate(self, vld):
        assert self._default or self._expr
        
        super(SwitchFieldCase, self).validate(vld)
        if self._expr:
            self._expr.validate(vld)

    def pac(self, printer):
        printer.output("<SwitchFieldCase TODO>")

    def __str__(self):
        return "<__str__ SwitchFieldCase TODO>"
    
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

    _valid_hooks = ("%ctor", "%dtor", "%error")

    def __init__(self, pscope, params=[], location=None):
        Unit._counter += 1
        super(Unit, self).__init__(location=location)
        self._props = {}
        self._fields = {}
        self._fields_ordered = []
        self._vars = {}
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
    
    def field(self, name):
        """Returns the fields of a specific name.
        
        Returns: list ~~Field - The fields, which may be empty if there's no
        field of that name. 
        
        Note: In some case, there can multiple fields of the same name, and
        thus this returns a list.
        """
        try:
            return self._fields[name]
        except KeyError:
            return []

    def params(self):
        """Returns the unit's parameters.
        
        Returns: list of ~~ID - The parameter; empty list for no parameters.
        """
        return self._params
    
    def fieldType(self, name):
        """Returns the type of a field.
        
        name: string - The name of the field. 
        
        Returns: ~~Type - The type, or None if there's no field of this type.
        
        Note: Even there can multiple fields of the same name, they all must
        have the same type and thus returns just a single type.
        """
        try:
            return self._fields[name][0].type()
        except KeyError:
            return None
            
    def parsedFieldType(self, name):
        """Returns the type of values parsed for a field given by name. The
        method located the field by the given name, and passes on what that
        field's ~~parsedType method returns. If the field however has
        ``&convert`` Attribute defined, the return type will be the result
        type of that function.
        
        name: string - The name of the field. 
        
        Returns: ~~Type - The parsed value. 
        
        Note: Even there can multiple fields of the same name, they all must
        have the same type and thus returns just a single type.
        """
        try:
            field = self._fields[name][0]
            
            convert = field.type().attributeExpr("convert")
            if not convert:
                return field.parsedType()
            
            return convert.type().resultType()
            
        except KeyError:
            return None
        
    def addField(self, field):
        """Adds a field to the unit type.
        
        field: ~~Field - The field type.
        """
        assert builtin_id(field._parent) == builtin_id(self)
        idx = field.name() if field.name() else str(builtin_id(field))
        
        if idx in self._fields:
            self._fields[idx] += [field]
        else:
            self._fields[idx] = [field]
            
        self._fields_ordered += [field]

    def variables(self):
        """Returns all user-defined variables.
        
        Returns: dictionary of string -> ~~id.Variable - The variables,
        indexed by their name.
        """
        return self._vars

    def variable(self, name):
        """Returns a specific variable.
        
        Returns: ~~id.Variable - The variable, or None if there's no variable of that name. 
        """
        try:
            return self._vars[name]
        except KeyError:
            return None
    
    def addVariable(self, var):
        """Adds a user-defined variable to the unit type.
        
        var: id.Variable - The variable.
        """
        self._vars[var.name()] = var
        
    def hooks(self, name):
        """Returns all hooks registered for a hook name.  If *name* is the
        name of a field or variable, any hooks attached to that will be
        returned. Note that the result for theses is not well-defined until
        ~~resolve has been called.
        
        name: string - The name of the hook to retrieve the hooks for.
        
        Returns: list of ~~UnitHook - The hooks with their priorities. The
        list will be empty if no hooks have been registered for that name. 
        
        Note: If there are multiple fields with the same name, the returned
        list will contains the hooks for all of them.
        """
        hooks = []
        
        if name in self._fields:
            for f in self._fields[name]:
                hooks += f.hooks
                
            return hooks

        if name in self._vars:
            return self._vars[name].hooks()
        
        return self._hooks.get(name, [])
        
    def addHook(self, name, hook, priority):
        """Adds a hook statement to the unit. The statement will be executed
        as determined by the hook's semantics. If the name is the name of a
        field or variable, the hook will be attached to that one. 
        
        name: string - The name of the hook for the statement to be added.
        
        hook: ~~Block - The hook itself.
        """
        try:
            self._hooks[name] += [hook]
        except KeyError:
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
            seq = [f.production() for f in self._fields_ordered if not isinstance(f, SubField)]
            seq = grammar.Sequence(seq=seq, type=self, symbol="start_%s" % self.name(), location=self.location())
            self._grammar = grammar.Grammar(self.name(), seq, self._params, addl_ids=self._vars.values(), location=self.location())
            
        return self._grammar

    # Overridden from Type.
    
    def hiltiType(self, cg):
        mbuilder = cg.moduleBuilder()

        def _makeType():
            # Generate the parsing code for our unit.
            gen = pgen.ParserGen(cg, self)
            return gen.objectType()
        
        return mbuilder.cache(self, _makeType)

    def validate(self, vld):
        
        error = self.grammar().check()
        if error:
            vld.error(self, error)
        
        for fields in self._fields.values():
            for f in fields:
                f.validate(vld)

        for var in self._vars.values():
            var.validate(vld)
            
        for (name, hooks) in self._hooks:
            for h in hooks:
                h.validate(vld)
                
        for fields in self._fields.values():
            if len(fields) > 1:
                cnt_ids = 0
                for f in fields:
                    if f.type().hasAttribute("default"):
                        vld.error(self, "if multiple fields have the same name, none can have a &default")
                        
                    if f.type().hasAttribute("convert"):
                        vld.error(self, "if multiple fields have the same name, none can have a &convert")
                    
                    if not f.isNoId():
                        cnt_ids += 1
                        
                if cnd_ids > 1:
                    vld.error(self, "field name defined more than once with different types")

    def pac(self, printer):
        printer.output("<unit type - TODO>")

    def resolve(self, resolver):
        if resolver.already(self):
            return self
        
        for param in self._params:
            param.resolve(resolver)

        # Transfer all the hooks refereing to a field or variable over to that
        # one. We do this here so that hooks for these can be defined before
        # we actually now about the field. 
        for (name, hooks) in self._hooks.items():
            if name in self._fields:
                # Move over to field.
                fields = self._fields[name]
                
                for field in fields:
                    for h in hooks:
                        h.setField(field)
                        field.addHook(h)
                    
                del self._hooks[name]
                
            elif name in self._vars:
                # Move over to variable.
                var = self._vars[name]
                
                for h in hooks:
                    var.addHook(h)
                    
                del self._hooks[name]
                
            else:
                # Unit-global hook, not attached to field.
                assert name in _valid_hooks
                
                for h in hooks:
                    h.resolve(resolver)

        for fields in self._fields.values():
            for f in fields:
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
    @staticmethod 
    def attrType(lhs, ident):
        name = ident.constant().value()
        
        if name in lhs.type().variables():
            return lhs.type().variables()[name].type()
        
        else:
           return lhs.type().parsedFieldType(name)
    
    
    def validate(vld, lhs, ident):
        name = ident.constant().value()
        if not name in lhs.type().variables() and not lhs.type().parsedFieldType(name):
            vld.error(lhs, "unknown unit attribute '%s'" % name)
        
    def type(lhs, ident):
        return Attribute.attrType(lhs, ident)
    
    def evaluate(cg, lhs, ident):
        name = ident.constant().value()
        type = Attribute.attrType(lhs, ident)
        builder = cg.builder()
        tmp = builder.addLocal("__attr", type.hiltiType(cg))
        builder.struct_get(tmp, lhs.evaluate(cg), builder.constOp(name))
        return tmp

    def assign(cg, lhs, ident, rhs):
        name = ident.constant().value()
        type = Attribute.attrType(lhs, ident)
        builder = cg.builder()
        obj = lhs.evaluate(cg)
        builder.struct_set(obj, builder.constOp(name), rhs)
        
        # TODO: Need to clean this hook calling up once we have real hooks in
        # HILTI.
        
        fields = lhs.type().field(name)
        if fields:
            for field in fields:
                field.parent().parserGen().runHooks(cg, obj, field.hooks())
            return
        
        var = lhs.type().variable(name)
        if var:
            rc = cg.functionBuilder().addTmp("__hookrc", hilti.type.Bool())
            builder = cg.builder()
            
            for hook in var.hooks():
                
                if cg.inHook(hook):
                    # Avoid recursion.
                    continue
                
                hookf = lhs.type().parserGen().functionHook(cg, hook)
                builder.call(rc, builder.idOp(hookf.name()), builder.tupleOp([obj]))
                
            return
            
        # Cannot be reached
        assert False

@operator.HasAttribute(Unit, type.Identifier)
class HasAttribute:
    def validate(vld, lhs, ident):
        name = ident.constant().value()
        if not name in lhs.type().variables() and not lhs.type().parsedFieldType(name):
            vld.error(lhs, "unknown unit attribute '%s'" % name)
        
    def type(lhs, ident):
        return hilti.type.Bool()
    
    def evaluate(cg, lhs, ident):
        name = ident.constant().value()
        tmp = cg.builder().addLocal("__has_attr", hilti.type.Bool())
        cg.builder().struct_is_set(tmp, lhs.evaluate(cg), cg.builder().constOp(name))
        return tmp

    
