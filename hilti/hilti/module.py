# $Id$

builtin_id = id

import llvm.core

import function
import location
import id
import node
import operand
import scope
import type
import resolver

import instructions.thread

class Module(node.Node):

    _hooks = {}

    """Represents a single HILTI link-time unit. A module has its
    own identifier scope defining which ~~ID objects are visibile
    inside its namespace.

    name: string - The globally visible name of the module; modules
    names are treated case-insensitive.

    location: ~~Location - A location to be associated with the function.
    """
    def __init__(self, name, location=None):
        super(Module, self).__init__(location)
        self._name = name.lower()
        self._location = location
        self._scope = scope.Scope(None, self._name)
        self._imported_modules = []
        self._exported = []
        self._tcontext = None
        self._tscopes = []

        # Add a "context" here that indicates none has been defined. The ID will
        # be overridden once a context is defined.
        i = id.Type("context", instructions.thread.NoContext)
        self.scope().add(i)

        if name.lower() == "main":
            # Automatically export the "run" function.
            ident = id.Unknown("run", type.Unknown())
            self._exported += [ident]

    def name(self):
        """Returns the name of the module. The module's name will have been
        converted to all lower-case in accordance with the policy to treat is
        case-independent.

        Returns: string - The name.
        """
        return self._name

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

    def importIDs(self, other):
        """Makes the IDs of another scope available to the current one.

        other: ~~Module - The other module.
        """
        resolver.Resolver().resolve(other)

        for i in other.scope().IDs():

            if i.imported():
                # Don't import IDs recursively.
                continue

            if i.linkage() == id.Linkage.EXPORTED:
                newid = i.clone()
                newid.setLocation(i.location())
                newid.setImported()
                newid.setNamespace(other.name())
                self.scope().add(newid)

        self._imported_modules += [(other, "<need to set path>")]

    def exportIdent(self, id):
        """Marks an identifier as exported. The code generator will then
        generate corresponding code making the ID referencable from external.

        id: ~~ID - The ID to export.
        """
        self._exported += [id]

    def _hookidx(self, hid):
        if isinstance(hid, id.ID):
            hid = hid.name()

        if hid.find("::") >= 0:
            (ns, name) = hid.split("::")
            return "%s::%s" % (ns.lower(), name)
        else:
            return "%s::%s" % (self.name().lower(), hid)

    def addHook(self, hid):
        """Adds a hook ID. Hooks are not part of the normal module namespace,
        but kept globally across all loaded modules so that functions can be
        added to externally declared hooks.

        hid: ~~id.Hook - The hook to add. If it already exists, the existing
        hook is replaced.
        """
        Module._hooks[self._hookidx(hid)] = (hid, self)

    def lookupHook(self, name):
        """Lookups a hook by its name. Hooks are not part of the normal module namespace,
        but kept globally across all loaded modules so that functions can be
        added to externally declared hooks. This method returns a hook if
        *any* other module has added it via ~~addHook.

        name: ~~ID or string - The hook identifier to look up. If a string, it
        can be fully qualified to specify an external module's hook.

        Returns: ~~id.Hook or None - The hook, or None if not yet added.
        """
        try:
            return Module._hooks[self._hookidx(name)][0]
        except KeyError:
            return None

    def threadContext(self):
        """Returns the module's threading context type.

        Returns: ~~Context - The context type if set, or None otherwise.
        """
        return self._tcontext

    def setThreadContext(self, context):
        """Sets the module's threading context type.

        context: ~~Context - The type of the module context.
        """
        assert isinstance(context, type.Context)
        self._tcontext = context

        # Add a "magic" identifier to reference the type. This overrides the
        # default identifier that's left undefined.
        i = id.Type("context", context, linkage=id.Linkage.EXPORTED, location=context.location())
        self.scope().add(i)

    def threadScopes(self):
        """Returns the threading scopes defined for the module.

        Returns: list of (name, ~~Scope) - The list of scopes with their
        names, which can be empty if no scopes have been defined.
        """
        return self._tscopes

    def addThreadScope(self, name, scope):
        """Adds a threading scope to the module. The scope's fields must be a
        subset of the fields the module's ~~context defines. The thread scope
        will also be added to the module's identifier scope under the given
        *name* name.

        name: string - The name of the scope.
        scope: ~~Scope - The scope to add.
        """
        self._tscopes += [(name, scope)]

        # Add an identifier to reference the scope.
        i = id.Type(name, scope, linkage=id.Linkage.EXPORTED, location=scope.location())
        self.scope().add(i)

    def __str__(self):
        return "module %s" % self._name

    ### Overridden from Node.

    def _resolve(self, resolver):
        resolver.startModule(self)
        self._scope.resolve(resolver)

        for (hook, mod) in Module._hooks.values():
            if builtin_id(mod) == builtin_id(self):
                hook.resolve(resolver)

        new_exported = []
        for i in self._exported:
            rid = self._scope.lookup(i.name())
            nid = rid if rid else i
            nid.setLinkage(id.Linkage.EXPORTED)
            new_exported += [nid]

        self._exported = new_exported
        resolver.endModule()

    def _validate(self, vld):
        vld.startModule(self)
        self._scope.validate(vld)

        for (hook, mod) in Module._hooks.values():
            if builtin_id(mod) == builtin_id(self):
                hook.validate(vld)

        if self.name().lower() == "main":
            run = self._scope.lookup("run")
            if not run:
                vld.error(self, "module Main must define a run() function")
                return

            if not isinstance(run, id.Function):
                vld.error(run, "in module Main, ID 'run' must be a function")
                return

        for i in self._exported:
            if isinstance(i, id.Unknown):
                vld.error(i, "export of unknown id %s" % i.name())
                continue

            i.validate(vld)

        ctx = set([i.name() for i in self._tcontext.fields()] if self._tcontext else [])

        for (name, scope) in self._tscopes:
            scope.validate(vld)

            for f in scope.fields():
                if not f in ctx:
                    vld.error(scope, "scope's field name %s is not defined in module's context" % f)

        vld.endModule()

    def _canonify(self, canonifier):
        canonifier.startModule(self)
        cmod = canonifier.currentModule()

        for i in self._scope.IDs():
            i.canonify(canonifier)

            # id.Function does not canonify its value to avoid getting into
            # functions recursively.
            if isinstance(i, id.Function):
                i.function().canonify(canonifier)

        for (hook, mod) in Module._hooks.values():
            if builtin_id(mod) == builtin_id(self):
                hook.canonify(canonifier)

        canonifier.endModule()

    def _codegen(self, cg):
        cg.startModule(self)

        for ty in self._exported:
            # Add the type information for the exported types to the module.
            if isinstance(ty, id.Type):
                cg.llvmTypeInfoPtr(ty.type())

        for i in self._scope.IDs():
            # Need to do globals first so that they are defined when functions
            # want to access them.
            if isinstance(i, id.Global):
                i.codegen(cg)

        for i in self._scope.IDs():
            # Todo: This is a bit of a hack: if this function is declared, we
            # generate it here directly. This is for libhilti to include it into
            # the library. However, we should build a nicer general mechanism
            # for static auto-generated functions.
            if i.name() == "__hlt_thread_mgr_run_callable":
                cg._llvmFunctionThreadMgrRunCallable()
                continue

            if isinstance(i, id.Function):
                i.codegen(cg)

        cg.endModule()

    def codegenHooks(self, cg):
        for (hook, mod) in Module._hooks.values():
            hook.codegen(cg)

    def output(self, printer):

        printer.startModule(self)
        printer.printComment(self)

        printer.output("\nmodule %s\n" % self.name(), nl=True)

        mods = [mod for (mod, path) in self._imported_modules]

        imported = False
        for mod in mods:
            if mod.name() == "hlt":
                # Implicitly imported.
                continue

            printer.output("import %s" % mod.name(), nl=True)
            imported = True

        if imported:
            printer.output("", nl=True)

        if self._tcontext:
            printer.output("context ");
            self._tcontext.output(printer)
            printer.output("", nl=True);

        for (name, scope) in self._tscopes:
            printer.output("scope %s " % name);
            scope.output(printer)

        if self._tscopes:
            printer.output("", nl=True);

        self._scope.output(printer)

        for (hook, mod) in Module._hooks.values():
            if builtin_id(mod) == builtin_id(self):
                hook.output(printer)

        printer.endModule()

    def autodoc(self):
        print ".. hlt:module:: %s\n" % self.name()

        for line in self.comment():
            print "    ", line

        print 

    # Visitor support.
    def visit(self, v):
        v.visitPre(self)
        self.scope().visit(v)
        v.visitPost(self)



