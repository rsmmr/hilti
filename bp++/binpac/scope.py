# $Id$

import id
import node
import type
import binpac.visitor as visitor

class Scope(visitor.Visitable):
    """Groups a set of IDs into a common visibility space. A scope can have a
    parent scope that will be searched recursively when an ID is not found.

    namespace: string - Optional namespace to associate with the scope.

    parent: ~~Scope - An optional parent scope; None for no parent.

    Todo: There's is still infrastructure in place for associating values with
    IDs. We can remove that.
    """
    def __init__(self, namespace, parent):
        super(Scope, self).__init__()
        self._namespace = namespace if namespace else "<noname>"
        self._parent = parent
        self._ids = {}

    def IDs(self):
        """Returns all IDs in the module's scope. This does not include any
        IDs in the parent scope.

        Returns: list of ~~ID - The IDs.
        """
        return [id for (id, value) in self._ids.values()]

    def parent(self):
        """Returns the parent scope.

        Returns: ~~Scope - The parent scope, or None if none.
        """
        return self._parent

    def setParent(self, pscope):
        """Sets a new parent scope.

        pscope: ~~Scope - The new parent scope.
        """
        self._parent = pscope

    def _canonName(self, namespace, name):
        m = name.split("::")
        assert len(m) <= 2

        if len(m) == 2:
            (namespace, name) = m

        if not namespace:
            namespace = self._namespace

        return "%s~%s" % (namespace, name)

    def addID(self, id):
        """Adds an ID to the scope.

        An ~~ID defined elsewhere can be imported into a scope by adding it
        with a namespace of that other module. In this case, subsequent
        lookups will only succeed if they are accordingly qualified
        (``<namespace>::<name>``). If the ID comes either without a namespace,
        or with a one that matches the scope's own namespace, subsequent
        lookups will succeed no matter whether they are qualified or not.

        If the ID does not have a namespace, we set its namespace to the name
        of the scope itself.

        id: ~~ID - The ID to add to the module's scope.
        """

        idx = self._canonName(id.namespace(), id.name())
        self._ids[idx] = (id, True)

        if not id.namespace():
            id.setNamespace(self._namespace)

    def _lookupID(self, i, return_val):
        if isinstance(i, str):
            # We first look it up as a scope-local variable, assuming there's no
            # namespace in the name.
            try:
                idx = self._canonName(None, i)
                (i, value) = self._ids[idx]
                return value if return_val else i
            except KeyError:
                pass

            # Now see if there's a namespace given.
            n = i.find("::")
            if n < 0:
                # No namespace.
                return None

            # Look up with the namespace.
            namespace = i[0:n].lower()
            name = i[n+2:]

        else:
            assert isinstance(i, id.ID)
            namespace = i.namespace()
            name = i.name()

        idx = self._canonName(namespace, name)

        try:
            (i, value) = self._ids[idx]
            return value if return_val else i

        except KeyError:
            return None

    def lookupID(self, id):
        """Looks up an ID in the module's namespace.

        id: string or ~~ID - Either the name of the ID to lookup (see
        :meth:`addID` for the lookup rules used for qualified names), or the
        ~~ID itself. The latter is useful to check whether the ID is part of
        the scope's namespace.

        Returns: ~~ID - The ID, or None if the name is not found.
        """
        result = self._lookupID(id, False)
        if result:
            return result

        return self._parent.lookupID(id) if self._parent else None

    # Visit support.
    def _visitType(self, v, ids, visited, filter):

        objs = []

        for name in ids:
            if name in visited:
                continue

            (id, value) = self._ids[name]
            if not filter(id):
                continue

            objs += [(id, value)]
            visited += [name]

        for (id, value) in sorted(objs, key=lambda id: id[0].name()):
            v.visit(id)
            if isinstance(value, visitor.Visitable):
                v.visit(value)

    def visit(self, v):
        v.visitPre(self)

        # Sort the ID names so that we get a deterministic order.
        ids = sorted(self._ids.keys())

        visited = []
        self._visitType(v, ids, visited, lambda i: isinstance(i, id.Type))
        self._visitType(v, ids, visited, lambda i: isinstance(i, id.Constant))
        self._visitType(v, ids, visited, lambda i: True) # all the rest.

        v.visitPost(self)
