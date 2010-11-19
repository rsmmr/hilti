# $Id$

import node
import location
import copy
import type

import binpac.util as util

class Linkage:
    """The *linkage* of an ~~IDFunction specifies its link-time
    visibility."""

    LOCAL = 1
    """An ~~ID with linkage LOCAL is only visible inside the
    ~~Module it is implemented in. This is the default linkage."""

    EXPORTED = 2
    """An ~~ID with linkage EXPORTED is visible across all
    compilation units.
    """

class ID(node.Node):
    """Binds a name to a type.

    name: string - The name of the ID.

    type: ~~Type - The type of the ID.

    linkage: ~~Linkage - The linkage of the ID.

    imported: bool - True to indicate that this ID was imported from another
    module. This does not change the semantics of the ID in any way but can be
    used to avoid importing it recursively at some later time.

    namespace: string - An optional namespace of the ID, i.e., the name of the
    module in which it is defined.

    location: ~~Location - A location to be associated with the ID.

    Note: The class maps the ~~Visitor subtype to :meth:`~type`.
    """

    def __init__(self, name, type, linkage=Linkage.LOCAL, namespace=None, location = None, imported=False):
        assert name
        assert type

        super(ID, self).__init__(location)
        self._name = name
        self._namespace = namespace.lower() if namespace else None
        self._type = type
        self._linkage = linkage
        self._imported = imported
        self._location = location

    def name(self):
        """Returns the ID's name.

        Returns: string - The ID's name.
        """
        return self._name

    def namespace(self):
        """Returns the ID's namespace.

        Returns: string - The ID's namespace, or None if it does not have one. The
        namespace will be lower-case.
        """
        return self._namespace

    def setNamespace(self, namespace):
        """Sets the ID's namespace.

        namespace: string - The new namespace.
        """
        self._namespace = namespace.lower() if namespace else None

    def setLinkage(self, linkage):
        """Sets the ID's linkage.

        linkage: string - The new linkage.
        """
        self._linkage = linkage

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

    def setType(self, ty):
        """Sets the ID's type."""
        self._type = ty

    def linkage(self):
        """Returns the ID's linkage.

        Returns: ~~Linkage - The ID's linkage.
        """
        return self._linkage

    def imported(self):
        """Returns whether the ID was imported from another module.

        Returns: bool - True if the ID was imported.
        """
        return self._imported

    def setImported(self):
        """Marks the ID as being imported from another module."""
        self._imported = True

    def clone(self):
        """Returns a deep-copy of the ID.

        Returns: ~~ID - The deep-copy.
        """
        return copy.deepcopy(self)

    def _internalName(self):
        """Returns the internal HILTI name of this ID.

        Returns: string - The name.
        """
        # Todo: Not sure this is the best place for this ...
        if self._name == "self":
            return "__self"

        return self._name

    def __str__(self):
        return self._name

    ### Overidden from node.Node.

    def resolve(self, resolver):
        self._type = self._type.resolve(resolver)

    def validate(self, vld):
        self._type.validate(vld)

    ### Methods for derived classes to override.

    def evaluate(self, cg):
        """Generates HILTI code to load the ID's value.

        This method must be overidden for all derived class that represent IDs
        that can be directly accessed.

        cg: ~~CodeGen - The code generator to use.

        Returns: ~~hilti.operand.Operand - An operand representing
        the evaluated ID.
        """
        util.internal_error("id.evaluate() not overidden for %s" % self.__class__)

    # Visitor support.
    def visitorSubType(self):
        return self._type

class Constant(ID):
    """An ID representing a constant value. See ~~ID for arguments not listed
    in the following.

    expr: ~~Expression - The expression to initialize the constant with, or
    None for initialization with the default value.
    """
    def __init__(self, name, type, expr, linkage=None, namespace=None, location=None, imported=False):
        super(Constant, self).__init__(name, type, linkage, namespace, location, imported)
        self._expr = expr

    def expr(self):
        """Returns the initialization expression for the local.

        Returns: ~~Expression - The init expression, or None if none was set.
        """
        return self._expr

    ### Overidden from node.Node.

    def resolve(self, resolver):
        ID.resolve(self, resolver)
        self._expr.resolve(resolver)

    def validate(self, vld):
        ID.validate(self, vld)

        if self._expr and self._expr.type() != self.type():
            vld.error("type of initializer expression does not match")

        if self._expr and not self._expr.isInit():
            vld.error("expression cannot be used as initializer")

    def pac(self, printer):
        printer.output("const %s: " % self.name(), nl=False)
        self.type().pac(printer)
        printer.output(" = ")
        printer._const.pac(printer)
        printer.output(";", nl=True)

    ### Overidden from ID.

    def evaluate(self, cg):
        return self._expr.hiltiInit(cg)

class Local(ID):
    """An ID representing a local function variable. See ~~ID for arguments.

    expr: ~~Expression - The expression to initialize the local with, or None
    for initialization with the default value.
    """
    def __init__(self, name, type, expr=None, linkage=None, namespace=None, location=None, imported=False):
        super(Local, self).__init__(name, type, linkage, namespace, location, imported)
        self._expr = expr

    def expr(self):
        """Returns the initialization expression for the local.

        Returns: ~~Expression - The init expression, or None if none was set.
        """
        return self._expr

    ### Overidden from node.Node.

    def resolve(self, resolver):
        ID.resolve(self, resolver)
        self._expr.resolve(resolver)

    def validate(self, vld):
        ID.validate(self, vld)

        if self._expr and self._expr.type() != self.type():
            vld.error("type of initializer expression does not match")

        if self._expr and not self._expr.isInit():
            vld.error("expression cannot be used as initializer")

    def pac(self):
        printer.output("local %s: " % self.name())
        self.type().pac(printer)
        printer.output(";", nl=True)

    ### Overidden from ID.

    def evaluate(self, cg):
        return cg.functionBuilder().idOp(self._internalName())

class Parameter(ID):
    """An ID representing a function parameter. See ~~ID for arguments.
    """
    def __init__(self, name, type, linkage=None, namespace=None, location=None, imported=False):
        super(Parameter, self).__init__(name, type, linkage, namespace, location, imported)

    ### Overidden from node.Node.

    def pac(self):
        printer.output("%s: " % self.name())
        self.type().pac(printer)

    ### Overidden from ID.

    def evaluate(self, cg):
        return cg.builder().idOp(self._internalName())

class UnitParameter(Parameter):
    """An ID representing a unit parameter. See ~~ID for arguments.
    """
    def __init__(self, name, type, location=None):
        super(UnitParameter, self).__init__(name, type, None, None, location, False)

    ### Overidden from node.Node.

    def pac(self):
        printer.output("%s: " % self.name())
        self.type().pac(printer)

    ### Overidden from ID.

    def evaluate(self, cg):
        # The value is stored in the unit's type object.
        builder = cg.builder()
        tmp = builder.addLocal(self.name(), self.type().hiltiType(cg))
        obj = builder.idOp("__self")
        builder.struct_get(tmp, obj, builder.constOp("__param_%s" % self.name()))
        return tmp

class Global(ID):
    """An ID representing a module-global variable. See ~~ID for arguments.

    expr: ~~Expression - The expression to initialize the global with, or None
    for initialization with the default value.
    """
    def __init__(self, name, type, expr, linkage=None, namespace=None, location=None, imported=False):
        super(Global, self).__init__(name, type, linkage, namespace, location, imported)
        self._expr = expr

    def expr(self):
        """Returns the initialization expressopm of the global.

        Returns: ~~Expression - The init expression, or None if none was set.
        """
        return self._expr

    ### Overidden from node.Node.

    def resolve(self, resolver):
        ID.resolve(self, resolver)
        if self._expr:
            self._expr.resolve(resolver)

    def validate(self, vld):
        ID.validate(self, vld)

        if self._expr and self._expr.type() != self.type():
            vld.error(self, "type of initializer expression does not match")

        if self._expr and not self._expr.isInit():
            vld.error(self, "expression cannot be used as initializer")

    def pac(self, printer):
        printer.output("global %s: " % self.name())
        self.type().pac(printer)
        printer.output(";", nl=True)

    ### Overidden from ID.

    def evaluate(self, cg):
        return cg.functionBuilder().idOp(self._internalName())

class Type(ID):
    """An ID representing a type-declaration. See ~~ID for arguments.
    """
    def __init__(self, name, type, linkage=None, namespace=None, location=None, imported=False):
        super(Type, self).__init__(name, type, linkage, namespace, location, imported)

    ### Overidden from node.Node.

    def pac(self, printer):
        printer.output("type %s = " % self.name())
        self.type().pac(printer)
        printer.output(";", nl=True)

    ### Overidden from ID.

    def evaluate(self, cg):
        util.internal_error("evaluate must not be called for id.Type")

class Attribute(ID):
    """An ID representing the RHS of an attribute expression.  See ~~ID for
    arguments not listed in the following.
    """
    def __init__(self, name, type, location=None):
        super(Attribute, self).__init__(name, type, location=location)

    ### Overidden from node.Node.

    def pac(self, printer):
        printer.output("%s" % self.name())

    ### Overidden from ID.

    def evaluate(self, cg):
        util.internal_error("evaluate must not be called for id.Attribute")

class Variable(ID):
    """An ID representing a user-defined unit variable. See ~~ID for arguments
    not listed in the following.
    """
    def __init__(self, name, type, location=None):
        super(Variable, self).__init__(name, type, location=location)

    ### Overidden from node.Node.

    def validate(self, vld):
        self.type().validate(vld)

    def resolve(self, resolver):
        ID.resolve(self, resolver)

    def validate(self, vld):
        ID.validate(self, vld)

    def pac(self, printer):
        printer.output("var %s" % self.name())

    ### Overidden from ID.

    def evaluate(self, cg):
        util.internal_error("evaluate must not be called for id.Variable")

class Function(ID):
    """An ID representing an (overloaded) function.

    name: string - The ID's name.

    func: ~~OverloadedFunction: The function referenced by this ID.
    """
    def __init__(self, name, func, location=None):
        super(Function, self).__init__(name, func.type(), location=location)
        self._func = func

    def function(self):
        """
        Returns the (overloaded) function referenced by this ID.

        Returns: ~~OverloadedFunction - The function.
        """
        return self._func

    ### Overidden from ID.

    def resolve(self, resolver):
        ID.resolve(self, resolver)
        self._func.resolve(resolver)

    def validate(self, vld):
        ID.validate(self, vld)
        self._func.validate(vld)

    def pac(self, printer):
        printer.output(self.name())
