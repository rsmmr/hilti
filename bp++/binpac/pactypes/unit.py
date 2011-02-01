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
import binpac.node as node
import binpac.property as property

import binpac.util as util

import hilti.type

_AllowedConstantTypes = (type.Bytes, type.RegExp)

class Field(node.Node):
    """One field within a unit data type.

    name: string or None - The name of the fields, or None for anonymous
    fields.

    value: ~~Expression or None - The value for constant fields.

    ty: ~~Type or None - The type of the field; can be None if
    *constant* is given. It can also be none for 'umbrella' fields
    that never get converted into an actual parsed field themselves
    (e.g., the ~~SwitchField).

    parent: ~~Unit - The unit type this field is part of.

    args: list of ~~Expr - If *type* is a unit type, the parameters passed to
    that on construction; if not (or if the unit type doesn't take any
    parameters), an empty list.

    Todo: Only ~~Bytes constant are supported at the moment. Which other's do
    we want? (Regular expressions for sure.)
    """
    def __init__(self, name, value, ty, parent, args=None, location=None):
        if value:
            assert isinstance(value, expr.Expression)

        self._name = name
        self._type = ty if ty else (value.type() if value else None)
        self._value = value
        self._parent = parent
        self._location = location
        self._cond = None
        self._args = args if args else []
        self._noid = False
        self._sink = None
        self._scope = scope.Scope(None, parent.scope())

        if self._type:
            self._type.setField(self)

        if isinstance(ty, type.Container):
            self._scope.addID(id.Parameter("__dollardollar", ty.itemType()))

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

    def scope(self):
        """Returns the field's scope.

        ReturnsL ~~Scope - The scope.
        """
        return self._scope

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
        """Returns the type of the field.

        Returns: string - The name.
        """

        t = self._type.fieldType() if self._type else None
        return self._type.fieldType() if self._type else None

    def parsedType(self):
        """Returns the type of values parsed by this type. Forwards simply to
        ~~Type.parsedType for the type returned by ~~type.

        Returns: ~~Type - The type of parsed values.
        """
        return self._type.parsedType()

    def value(self):
        """Returns the value for constant fields.

        Returns: ~~Expression or None - The expression, or None if not a constant
        field.
        """
        return self._value

    def args(self):
        """Returns the parameters passed to the sub-type. Only relevant if
        *type* is also a ~~Unit type.

        Returns: list of ~~Expr - The parameters; empty list for no
        parameters.
        """
        return self._args

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

    def sink(self):
        """Returns the sink associated with the field, if any.

        Returns: ~~Expr - An expression referencing the sink parsed data
        should be forwarded to; or None if not set.
        """
        return self._sink

    def setSink(self, sink):
        """Sets the sink associated with the field.

        sink: ~~Expr - An expression referencing the sink parsed data
        should be forwarded to; or None if not set.
        """
        self._sink = sink

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
                    prod = self._type.productionForLiteral(self, self._value)
                    break
            else:
                util.internal_error("unexpected constant type for literal: %s" % repr(self._type))

        else:
            prod = self._type.production(self)
            assert prod

            if self._noid:
                prod.setNoID()

            if self._args:
                assert isinstance(prod, grammar.ChildGrammar)
                prod.setParams(self._args)

        prod.setName(self._name)
        prod.setType(self._type)

        if isinstance(prod, grammar.Terminal):
            prod.setSink(self._sink)

        # We add the hooks to a concatened epsilon production. If we woudl add
        # them to the returned production, they might end up being executed
        # multiple times if that contains some kind of loop of itself.
        eps = grammar.Epsilon()
        prod = grammar.Sequence(seq=[prod, eps])
        eps.setField(self)

        assert prod

        if self._cond:
            prod = grammar.Boolean(self._cond, prod, grammar.Epsilon())

        return prod

    def __str__(self):
        tag = "%s: " % self._name if self._name else ""
        return "%s%s" % (tag, self._type)

    ### Overriden from node.Node.

    def _resolveWithConstant(self, resolver, ty):
        """Helper function that resolves a type by also checking whether the
        given ID refers to a constant and if so resolving to that's type.
        """
        if isinstance(ty, type.Unknown):
            i = resolver.scope().lookupID(ty.idName())

            if i and isinstance(i, id.Constant):
                nty = i.expr().type()
            else:
                nty = ty.resolve(resolver)

            nty.copyAttributesFrom(ty)
            val = i.expr() if isinstance(i, id.Constant) else None
            return (val, nty)

        else:
            return (None, ty.resolve(resolver))

    def _resolve(self, resolver):
        super(Field, self)._resolve(resolver)

        if self._type:
            (expr, self._type) = self._resolveWithConstant(resolver, self._type)

            if expr:
                self._value = expr

            if self._type.attributeExpr("until"):
                self._type.attributeExpr("until").resolve(resolver)

        if self._cond:
            self._cond.resolve(resolver)

        if isinstance(self.type(), type.Container):
            dd = self._scope.lookupID("__dollardollar")
            (expr, ty) = self._resolveWithConstant(resolver, dd.type())
            dd.setType(ty.parsedType())

    def _validate(self, vld):
        super(Field, self).validate(vld)

        if self._value:
            util.check_class(self._value, expr.Expression, "Field.validate")

        if self._type:
            if not isinstance(self._type, type.ParseableType):
                # If the production function has not been overridden, we can't
                # use that type in a unit field.
                vld.error(self, "type %s cannot be used inside a unit field" % self._type)

            self._type.validate(vld)
            self._type.validateInUnit(self, vld)

        if self._value:
            # White-list the types we can deal with in constants.
            for a in _AllowedConstantTypes:
                if isinstance(self._type, a):
                    break
            else:
                vld.error(self, "type %s cannot be used in a constant unit field" % self.type())

        if isinstance(self._type, Unit):
            if len(self._args) != len(self._type.args()):
                vld.error(self, "number of unit parameters do not match (have %d, but expected %d)" % (len(self._args), len(self._type.args())))

            i = 0
            for (have, want) in zip(self._args, self._type.args()):
                i += 1
                if not have.canCoerceTo(want.type()):
                    vld.error(self, "unit parameter %d mismatch: is %s but need %s" % (i, have.type(), want.type()))

        else:
            if len(self._args):
                vld.error(self, "type does not receive any parameters")

        if self._cond and not isinstance(self._cond.type(), type.Bool):
            vld.error(self, "field condition must be boolean")

        convert = self.type().attributeExpr("convert") if self.type() else None

        if convert and not operator.hasOperator(operator.Operator.Call, [convert, [expr.Hilti(None, self.parsedType())]]):
            vld.error(self, "no matching function for &convert found")

        if self._sink and not isinstance(self.type(), type.Sinkable):
            vld.error(self, "cannot attach sink to type %s" % self.type())

    def pac(self, printer):
        """Converts the field into parseable BinPAC++ code.

        printer: ~~Printer - The printer to use.
        """
        printer.output("<UnitField TODO>")

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
        case._number = len(self._cases)

    ### Overridden from Field.

    def production(self):
        grammar_cases = []
        default = None

        for case in self._cases:
            if case.isDefault():
                default = case.production()

            else:
                grammar_cases += [(case.exprs(), case.production())]

        return grammar.Switch(self._expr, grammar_cases, default, symbol="switch", location=self.location())

    def __str__(self):
        return "<__str__ SwitchFieldCase TODO>"

    ### Overriden from node.Node.

    def _resolve(self, resolver):
        super(SwitchField, self)._resolve(resolver)

        self._expr.resolve(resolver)

        # Having multiple cases with the same field name is ok if they are
        # of the same type. In that case, make sure that only one of them
        # actually ends up in the HILTI struct.  Note that error reporting for
        # non-matching types will be done in validate.
        names = {}
        for field in self._cases:
            field.resolve(resolver)

            name = field.name()
            if not name in names:
                names[name] = field.type()
            elif field.type() == names[name]:
                field.setNoID()

    def _validate(self, vld):
        super(SwitchField, self).validate(vld)

        self._expr.validate(vld)

        defaults = 0
        values = []
        for case in self._cases:
            if case.isDefault():
                defaults += 1
                continue

            if not case.exprs():
                util.internal_error("no expression for switch case")

            for e in case.exprs():
                if not e.canCoerceTo(self._expr.type()):
                    vld.error(self, "case expression is of type %s, but must be %s" % (e.type(), self._expr.type()))

            for e in case.exprs():
                if isinstance(e, expr.Ctor):
                    if e.value() in values:
                        vld.error(self, "case '%s' defined more than once" % e.value())
                    else:
                        values += [e.value()]

        if defaults > 1:
            vld.error(self, "more than one default case")

    def pac(self, printer):
        printer.output("<SwitchFieldCase TODO>")

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

    def __init__(self, name, value, ty, parent, args=None, location=None):
        super(SwitchFieldCase, self).__init__(name, value, ty, parent, args=args, location=location)
        self._default = False
        self._exprs = None
        self._number = -1

    def caseNumber(self):
        """Returns a case sequence number that is unique among all cases
        belonging to the same ~~SwitchField. This method must only be called
        the case has already been added to the switch via ~~addCase.

        Returns: integer - The sequence number.
        """
        assert self._number >= 0
        return self._number

    def isDefault(self):
        """Returns whether this field is the default branch.

        Returns: Bool - True if the field is the default.
        """
        return self._default

    def exprs(self):
        """Returns the expressions associated with the case.

        Returns: list of ~~Expression - The expressions, or None for the default branch.
        """
        return self._exprs

    def setExprs(self, exprs):
        """Sets the expression associated with the field. This must be called
        once for every case (including the default case).

        expr: ~~Expression or list of ~~Expressions - One or more expressions,
        or None to mark the case as the default.
        """
        if not isinstance(exprs, list):
            self._exprs = [exprs]

        self._exprs = exprs if exprs else []

        if not self._exprs:
            self._default = True

    def __str__(self):
        return "<__str__ SwitchFieldCase TODO>" + str(self.type()._attrs)

    ### Overridden from node.Node

    def _resolve(self, resolver):
        super(SwitchFieldCase, self)._resolve(resolver)
        for expr in self._exprs:
            expr.resolve(resolver)

    def _validate(self, vld):
        super(SwitchFieldCase, self).validate(vld)

        assert self._default or self._exprs

        super(SwitchFieldCase, self).validate(vld)
        for expr in self._exprs:
            expr.validate(vld)

    def pac(self, printer):
        printer.output("<SwitchFieldCase TODO>")

@type.pac("unit")
class Unit(type.ParseableType, property.Container):
    """Type describing an individual parsing unit.

    A parsing unit is composed of (1) fields parsed from the traffic stream,
    which are then turned into a grammar; and (2) a number of "hooks", which
    are statements to be run on certain occasions (like when an error has been
    found).

    module: ~~Module - The module in which this unit is defined.

    args: list of ~~ID - Optional parameters for the unit type.

    location: ~~Location - A location object describing the point of definition.

    Todo: We need to document the available hooks.
    """

    _cache = {}

    def __init__(self, module, args=None, location=None):
        property.Container.__init__(self)
        Unit._counter += 1
        super(Unit, self).__init__(location=location)
        self._fields = {}
        self._fields_ordered = []
        self._vars = {}
        self._prod = None
        self._grammar = None
        self._args = args if args else []
        self._scope = scope.Scope(None, module.scope())
        self._pobjtype = None
        self._module = module

        self._scope.addID(id.Parameter("self", self, location=location))

        for p in self._args:
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

    def module(self):
        """Returns the module in which this unit is defined.

        Returns: ~~Module - The module.
        """
        return self._module

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

    def args(self):
        """Returns the unit's parameters.

        Returns: list of ~~ID - The parameter; empty list for no parameters.
        """
        return self._args

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

    def grammar(self):
        """Returns the grammar for this type."""
        if not self._grammar:
            if self.namespace():
                name = "%s::%s" % (self.namespace(), self.name())
            else:
                name = self.name()

            # We save the grammar we're going to  build first to avoid infinite
            # recursions in the case of cyclic grammars.
            self._grammar = grammar.Grammar(name, None)

            seq = [f.production() for f in self._fields_ordered if not isinstance(f, SubField)]
            seq = grammar.Sequence(seq=seq, type=self, symbol="start_%s" % self.name(), location=self.location())

            self._grammar.__init__(name, seq, self._args, addl_ids=self._vars.values(), location=self.location())

        return self._grammar

    def hiltiParseObjectType(self):
        """Returns the HILTI type of the record object the unit is being
        parsed into. This method must only be called after a ~~ParserGen has
        already compiled the unit's grammar.

        Returns: hilti.type.Type - The HILTI type.
        """
        return self._pgen.objectType()

    def nameFunctionNew(self):
        """Returns the ID of a function for allocating a new instance of the
        type's HILTI object. The function will be generated by the ~~ParserGen
        and it parameters correspond to what ~~args returns. The function's
        return value is the new object.

        Note: The function creates the object but does not yet run the
        ``%init`` hooks. That will happen only when parsing is initialized.
        """
        return "__new_%s" % self.name().replace(" ", "")

    # Overriden from property.Container

    def allProperties(self):
        return {
            "name": expr.Ctor(self._name, type.String()),
            "byteorder": False,
            "mimetype": [],
            }

    # Overridden from Type.

    def hiltiType(self, cg):
        mbuilder = cg.moduleBuilder()

        def _makeType():
            # Generate the parsing code for our unit.
            gen = pgen.ParserGen(cg, self)
            return gen.objectType()

        return mbuilder.cache(self, _makeType)

    def _resolve(self, resolver):
        super(Unit, self)._resolve(resolver)

        for param in self._args:
            param.resolve(resolver)

        for fields in self._fields.values():
            for f in fields:
                f.resolve(resolver)

        self.resolveProperties(resolver)

    def validate(self, vld):
        super(Unit, self).validate(vld)

        g = self.grammar()
        if g.name() == "Message":
            error = g.check()
            if error:
                vld.error(self, error)

        for fields in self._fields.values():
            for f in fields:
                f.validate(vld)

        for var in self._vars.values():
            var.validate(vld)

        for fields in self._fields.values():
            if len(fields) > 1:
                cnt_ids = 0
                for f in fields:
                    if f.type().hasAttribute("default"):
                        vld.error(self, "if multiple fields have the same name, none can have a &default")

                    if f.type().hasAttribute("convert"):
                        vld.error(self, "if multiple fields have the same name, none can have a &convert")

                    if not f.isNoID():
                        cnt_ids += 1

                if cnt_ids > 1:
                    vld.error(self, "field name defined more than once with different types")

        for param in self._args:
            param.validate(vld)

        if self.property("mimetype") and self._args:
            vld.error(self, "a unit with a MIME type must not take any additional parameters")

    def pac(self, printer):
        printer.output("<unit type - TODO>")

    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return { "parse": (type.IteratorBytes(), False, None) }

    def production(self, field):
        #seq = [f.production() for f in self._fields_ordered]
        #seq = grammar.Sequence(seq=seq, type=self, symbol="start-%s" % self.name(), location=self.location())
        #return seq
        return grammar.ChildGrammar(self, location=self.location())

    def generateParser(self, cg, var, args, dst, skipping):
        # This will not be called because we handle the grammar.ChildGrammar
        # case separetely in the parser generator.
        util.internal_error("cannot be reached")

@operator.Coerce(type.Unit)
class Coerce:
    def canCoerceTo(srctype, dsttype):
        if isinstance(dsttype, type.Bool):
            return True

        if not isinstance(dsttype, Unit):
            return False

        return srctype.name() == dsttype.name()

    def coerceExprTo(cg, e, dsttype):
        if isinstance(dsttype, type.Bool):
            tmp = cg.builder().addLocal("__nonnull", hilti.type.Bool())
            cg.builder().ref_cast_bool(e.evaluate(cg))
            return expr.Hilti(tmp, type.Bool())

        return e

@operator.Attribute(Unit, type.String)
class _Attribute:
    @staticmethod
    def attrType(lhs, ident):
        name = ident.name()
        if name in lhs.type().variables():
            return lhs.type().variables()[name].type()

        else:
            t = lhs.type().field(name)[0].type()
            return lhs.type().field(name)[0].type()

    def validate(vld, lhs, ident):
        name = ident.name()
        if name and not name in lhs.type().variables() and not lhs.type().field(name):
            vld.error(lhs, "unknown unit attribute '%s'" % name)

    def type(lhs, ident):
        return _Attribute.attrType(lhs, ident)

    def evaluate(cg, lhs, ident):
        name = ident.name()
        type = _Attribute.attrType(lhs, ident)
        builder = cg.builder()
        tmp = builder.addLocal("__attr", type.hiltiType(cg))
        builder.struct_get(tmp, lhs.evaluate(cg), name)
        return tmp

    def assign(cg, lhs, ident, rhs):
        name = ident.name()
        type = _Attribute.attrType(lhs, ident)
        builder = cg.builder()
        obj = lhs.evaluate(cg)
        builder.struct_set(obj, name, rhs)

        # Run hooks (but not recursively).
        unit = lhs.type()
        objt = unit.hiltiParseObjectType()
        op1 = cg.declareHook(unit, ident, objt)

        if not cg.inHook(op1.value()):
            user = cg.builder().idOp("__user")
            op2 = builder.tupleOp([obj, user])
            builder.hook_run(None, op1, op2)

@operator.HasAttribute(Unit, type.String)
class _:
    def validate(vld, lhs, ident):
        name = ident.name()
        if not name in lhs.type().variables() and not lhs.type().field(name):
            vld.error(lhs, "unknown unit attribute '%s'" % name)

    def type(lhs, ident):
        return type.Bool()

    def evaluate(cg, lhs, ident):
        name = ident.name()
        tmp = cg.builder().addLocal("__has_attr", hilti.type.Bool())
        cg.builder().struct_is_set(tmp, lhs.evaluate(cg), name)
        return tmp

@operator.MethodCall(type.Unit, expr.Attribute("input"), [])
class _:
    """Returns an ``iter<bytes>`` referencing the first byte of the raw data
    for parsing the unit. This method must only be called while the unit is
    being parsed, and will throw an ``UndefinedValue`` exception otherwise.
    """
    def type(obj, method, args):
        return type.IteratorBytes()

    def evaluate(cg, obj, method, args):
        tmp = cg.functionBuilder().addLocal("__iter", hilti.type.IteratorBytes())
        builder = cg.builder()
        builder.struct_get(tmp, obj.evaluate(cg), "__input")
        return tmp

@operator.MethodCall(type.Unit, expr.Attribute("offset"), [])
class _:
    """Returns the offset of the current parsing position relative to the
    start of the current parsing unit. This method must only be called while
    the unit is being parsed, and will throw an ``UndefinedValue`` exception
    otherwise.

    Note that when being inside a field hook, the current parsing position
    will have already moved on to the start of the *next* field because the
    hook is only run after the current field has been fully parsed. On the
    other hand, if the method is called from an expression evaluated before
    the parsing of a field starts (such as in a field's ``&length``
    attribute), the returned offset will reflect the beginning of that field.
    """

    def type(obj, method, args):
        return type.UnsignedInteger(64)

    def evaluate(cg, obj, method, args):
        offset = cg.functionBuilder().addLocal("__offset", hilti.type.Integer(64))
        input = cg.functionBuilder().addLocal("__input", hilti.type.IteratorBytes())
        cur = cg.functionBuilder().addLocal("__cur", hilti.type.IteratorBytes())

        obj = obj.evaluate(cg)

        builder = cg.builder()
        builder.struct_get(input, obj, "__input")
        builder.struct_get(cur,   obj, "__cur")
        builder.bytes_diff(offset, input, cur)

        return offset

@operator.MethodCall(type.Unit, expr.Attribute("set_input"), [type.IteratorBytes])
class _:
    """Changes the position in the input stream to continue parsing from. The
    new position is a new ``iter<bytes>`` where subsequent parsing will
    proceed. Note this changes the position *globally*: all subsequent field
    will be parsed from the new position, including those of a potential
    higher-level unit this unit is part of. Returns an ``iter<bytes>`` with
    the old position.
    """
    def type(obj, method, args):
        return type.IteratorBytes()

    def evaluate(cg, obj, method, args):
        obj = obj.evaluate(cg)
        pos = args[0].evaluate(cg)
        cg.builder().struct_set(obj, "__cur", pos)
        return pos

@operator.New(Unit, operator.Any())
class _:
    def type(name, args):
        return name.type()

    def validate(vld, name, args):
        if not isinstance(name, expr.Name):
            vld.error(name, "operator new must use static type name")

        proto = name.type().args()

        if len(args) != len(proto):
            vld.error(name, "wrong number of parameters for ctor (expected %d, have %d)" % (len(proto), len(args)))

        for (arg, proto, i) in zip(args, proto, range(len(args))):
            if not arg.canCoerceTo(proto.type()):
                vld.error(name, "incompatible type for parameter %d" % i)

    def evaluate(cg, name, args):
        assert isinstance(name, expr.Name)
        tid = name.scope().lookupID(name.name())
        assert tid

        obj = cg.functionBuilder().addLocal("__pobj", tid.type().hiltiType(cg))

        name = tid.type().nameFunctionNew()
        fid = hilti.operand.ID(hilti.id.Unknown(name, cg.moduleBuilder().module().scope()))
        hargs = [arg.evaluate(cg) for arg in args]
        null = cg.builder().constOp(None, hilti.type.Reference(None))
        hargs += [null, hilti.operand.Ctor("", hilti.type.Reference(hilti.type.Bytes()))]
        obj = cg.builder().call(obj, fid, cg.builder().tupleOp(hargs))
        return obj

@operator.MethodCall(type.Unit, expr.Attribute("disconnect"), [])
class _:
    """Disconnect the unit from its parent sink. The unit gets signaled a
    regular end of data, so if it still has input pending, that might be
    processed before the method returns.  If the unit is not connected to a
    sink, the method does not have any effect.
    """
    def type(obj, method, args):
        return type.Void()

    def evaluate(cg, obj, method, args):
        pobj = obj.evaluate(cg)

        sink = cg.builder().addLocal("__sink", cg.functionBuilder().typeByID("BinPAC::Sink"))
        sink_set = cg.builder().addLocal("__sink_set", hilti.type.Bool())
        cg.builder().struct_is_set(sink_set, pobj, "__sink")

        fbuilder = cg.functionBuilder()
        disconnect = fbuilder.newBuilder("disconnect")
        done = fbuilder.newBuilder("done")
        cg.builder().if_else(sink_set, disconnect, done)

        cg.setBuilder(disconnect)

        cg.builder().struct_get(sink, pobj, "__sink")

        cfunc = cg.builder().idOp("BinPAC::sink_disconnect")
        cargs = cg.builder().tupleOp([sink, pobj])
        cg.builder().call(None, cfunc, cargs)
        cg.builder().struct_unset(pobj, "__sink")

        cg.builder().jump(done)

        cg.setBuilder(done)

        return None


@operator.MethodCall(type.Unit, expr.Attribute("mime_type"), [])
class _:
    """Returns the MIME type that was specified when the unit was
    instantiated (e.g., via ~~sink.connect_mime_type()). Returns an
    empty ``bytes`` object if none was specified. This method can
    only be called for exported types.
    """
    def type(obj, method, args):
        return type.Bytes()

    def validate(vld, obj, method, args):
        if not obj.type().exported():
            vld.error(obj, "mime_type() can only be called with exported types")

    def evaluate(cg, obj, method, args):
        pobj = obj.evaluate(cg)
        mt = cg.builder().addLocal("__mime_type", hilti.type.Reference(hilti.type.Bytes()))
        cg.builder().struct_get(mt, pobj, "__mimetype")
        return mt

@operator.MethodCall(type.Unit, expr.Attribute("add_filter"), [type.Enum])
class _:
    """Adds an input filter of type *op1* (of type ~~BinPAC::Filter) to the
    unit object. *op1* The filter will receive all parse input first,
    transform it according to its semantics, and then the unit will parse the
    *output* of the filter.

    Multiple filters can be added to a parsing unit, in which case they will
    be chained into a pipeline and the data is passed through them in the
    order they have been added. The actual unit parsing will then be carried
    out on the output of the last filter in the chain.

    Note that filters must be added before the first data chunk is passed in.
    If parsing has alrady started when a filter is added, behaviour is
    undefined.  Also note that filters can only be added to *exported* unit
    types.

    Currently, only a set of predefined filters can be used; see
    ~~BinPAC::Filter. One cannot define own filters in BinPAC++.

    Todo: We should probably either enables adding filters laters, or catch
    the case of adding them too late at run-time an abort with an exception.
    """
    def type(obj, method, args):
        return type.Void()

    def validate(vld, obj, method, args):
        if not obj.type().exported():
            vld.error(obj, "add_filter() can only be called with exported types")

        # TODO: Make sure it's the right enum ...

    def evaluate(cg, obj, method, args):
        obj = obj.evaluate(cg)
        ftype = args[0].evaluate(cg)

        filter = cg.builder().addLocal("__filter", cg.functionBuilder().typeByID("BinPAC::ParseFilter"))
        cg.builder().call(filter, cg.builder().idOp("BinPAC::filter_new"),  cg.builder().tupleOp([ftype]))

        head = cg.builder().addLocal("__filter_head", cg.functionBuilder().typeByID("BinPAC::ParseFilter"))
        null = hilti.operand.Constant(hilti.constant.Constant(None, filter.type()))
        cg.builder().struct_get_default(head, obj, "__filter", null)
        cfunc = cg.builder().idOp("BinPAC::filter_add")
        cargs = cg.builder().tupleOp([head, filter])
        cg.builder().call(head, cfunc, cargs)

        cg.builder().struct_set(obj, "__filter", head)

        return None
