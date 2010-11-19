# $Id$

builtin_id = id

import node
import id
import scope
import type
import stmt
import location
import property
import binpac.visitor as visitor

class Module(node.Node, property.Container):
    """Represents a single HILTI link-time unit. A module has its
    own identifier scope defining which ~~ID objects are visibile
    inside its namespace.

    name: string - The globally visible name of the module; modules
    names are treated case-insensitive.

    location: ~~Location - A location to be associated with the function.

    Note: When traversion a module with a ~~Visitor, it will visit all ~~ID
    objects in its scope *and* all their values (see :meth:`addID`)
    """
    def __init__(self, name, location=None):
        super(Module, self).__init__(location)
        property.Container.__init__(self)
        self._name = name.lower()
        self._orgname = name
        self._location = location
        self._scope = scope.Scope(name, None)
        self._stmts = []
        self._hooks = []
        self._external_hooks = []
        self._imported_modules = []

    def name(self, org=False):
        """Returns the name of the module. The module's name will
        have been converted to all lower-case in accordance with the
        policy to treat it case-independent.

        org: bool - If true, the name is returned in its original spelling.

        Returns: string - The name.
        """
        return self._name if not org else self._orgname

    def scope(self):
        """Returns the module's scope.

        Returns: ~~Scope - The scope.
        """
        return self._scope

    def importedModules(self):
        """Returns the modules which have been imported into this modules
        namespace.

        Returns: list of (module, path) - List of imported modules. ``module``
        is the name of the module as it specified to be imported, and ``path``
        is the full path of the file that got imported."""
        return self._imported_modules

    def addImport(self, other, path):
        """Registers another module as being imported. The import's IDs will
        be available to this module during code-generation.

        other: ~~Module: The other module
        path: string: The full path where the other module was parsed from.
        """

        self._imported_modules += [(other, "<need to set path>")]

    def _importIDs(self, other):
        """Makes the IDs of another module available to the current one.

        other: ~~Module - The other module.
        """
        for i in other._scope.IDs():

            if i.imported():
                # Don't import IDs recursively.
                continue

            newid = i.clone()
            newid.setLocation(i.location())
            if not newid.namespace():
                newid.setNamespace(other.name())
            newid.setImported()

            self.scope().addID(newid)

    def addStatement(self, stmt):
        """Adds a global statements to the module. Global statements will
        executed at module initialization time.

        stmt: ~~Statement - The statement.
        """
        self._stmts += [stmt]

    def statements(self):
        """Returns the module-global statements. These will be execute at
        module initialization time.

        Returns : list of ~~Statement - The statements.
        """
        return self._stmts

    def addExternalHook(self, unit, ident, stmts, debug=False):
        """Adds an external hook to an existing unit.

        unit: ~~Unit - The unit to add a hook to. Can be None if
        *ident* is fully qualified.

        ident: string - A string referencing the hook. If *unit* is
        given, this must be only the unit-internal name of the hook
        (i.e., either a field name, a variable name, or a global
        hook name such as ``%init``). If *unit* is not given, this
        must suitably qualified, e.g., ``MyUnit::my_hook`` or
        ``MyModule::MyUnit::my_hook``.

        stmts: ~~Block - The hook's body.

        debug: bool - If True, this hook will only compiled in if
        the code generator is including debug code, and it will only be executed
        if at run-time, debug mode is enabled (via the C function
        ~~binpac_enable_debug).
        """
        self._external_hooks += [(unit, ident, stmts, debug)]

    def addHook(self, hook):
        self._hooks += [hook]

    def __str__(self):
        return "module %s" % self._name

    ### Overidden from node.Node.

    def resolve(self, resolver):

        for (mod, path) in self._imported_modules:
            mod.resolve(resolver)

        for (mod, path) in self._imported_modules:
            self._importIDs(mod)

        for id in self._scope.IDs():
            id.resolve(resolver)

        for s in self.statements():
            s.resolve(resolver)

        for (unit, ident, stmts, debug) in self._external_hooks:
            stmts.resolve(resolver)

            hook = self._makeHook(unit, ident, stmts, debug)
            if not hook:
                resolver.error(unit, "%s does not reference a valid unit field/hook" % ident)
                continue

            self._hooks += [hook]

        self._external_hooks = []

        for h in self._hooks:
            h.resolve(resolver)

        self.resolveProperties(resolver)

    def validate(self, vld):
        for (mod, path) in self._imported_modules:
            mod.validate(vld)

        for id in self._scope.IDs():
            id.validate(vld)

        for stmt in self.statements():
            stmt.validate(vld)

        for h in self._hooks:
            h.validate(vld)

    def execute(self, cg):
        for hook in self._hooks:
            self._compileHook(cg, hook)

    def pac(self, printer):
        printer.output("module %s\n" % self._name, nl=True)

        for id in self._scope.IDs():
            id.pac(printer)

        for stmt in self.statements():
            stmt.pac(printer)

        for h in self._hooks:
            h.pac(vld)

    # Overriden from property.Container

    def allProperties(self):
        return {
            "byteorder": None,
            }

    # Visitor support.
    def visit(self, v):
        v.visit(self._scope)

    def _makeHook(self, unit, ident, stmts, debug):
        """Instantiates a hook previously added via ~~addExternalHook."""
        if not unit:
            try:
                (unit, name) = ident.rsplit("::", 1)
                unit = self.scope().lookupID(unit)

                if not unit:
                    return None

                unit = unit.type()

                if not isinstance(unit, type.Unit):
                    return None

            except ValueError:
                return None
        else:
            name = ident

        assert isinstance(unit, type.Unit)

        f = unit.field(name)
        if f:
            return stmt.FieldHook(unit, f[0], 0, stmts=stmts, debug=debug, location=unit.location())

        v = unit.variable(name)
        if v:
            return stmt.VarHook(unit, v, 0, stmts=stmts, debug=debug, location=unit.location())

        return stmt.UnitHook(unit, name, 0, stmts=stmts, debug=debug, location=unit.location())

    def _compileHook(self, cg, hook):
        """Compiles a hook into HILTI code."""
        ftype = hook.hiltiFunctionType(cg)
        hid = cg.moduleBuilder().declareHook(hook.hiltiName(cg), ftype.args(), ftype.resultType())

        (fbuilder, builder) = cg.beginFunction(None, ftype, hook=True)
        func = fbuilder.function()
        func.setPriority(hook.priority())

        hid.addFunction(func)

        cg.beginHook(hook)
        hook.execute(cg)
        cg.endHook()

        cg.endFunction()
