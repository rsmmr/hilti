# $Id$

import constant
import id
import node
import type
import visitor

class Scope(node.Node):
    """Groups a set of IDs into a common visibility space. A scope
    can have a parent scope that will be searched recursively when
    an ID is not found.
    
    parent: ~~Scope - An optional parent scope; None for no parent. 
    
    Todo: There's is still infrastructure in place for associating
    values with IDs. We can remove that. 
    """
    def __init__(self, parent, namespace=None, location=None):
        super(Scope, self).__init__(location)
        self._parent = parent
        self._ids = {}
        self._namespace = namespace
        
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
    
    def _canonName(self, namespace, name):
        if not namespace:
            namespace = self._namespace if self._namespace else "<no namespace>"
            
        return "%s~%s" % (namespace, name)
    
    def add(self, i):
        """Adds an ID to the scope.
        
        An ~~ID defined elsewhere can be imported into a scope by adding it
        with a namespace of that other module. In this case, subsequent
        lookups will only succeed if they are accordingly qualified
        (``<namespace>::<name>``). If the ID comes either without a namespace,
        or with a one that matches the scope's own namespace, subsequent
        lookups will succeed no matter whether they are qualified or not. 
        
        If the ID does not have a namespace, we set its namespace to the name
        of the scope itself.
        
        i: ~~ID - The ID to add to the module's scope.
        """
        if isinstance(i, id.Hook):
            util.internal_error("scope.Scope: Cannot add id.Hook to a scope. Use Module.addHook instead.")
        
        idx = self._canonName(i.namespace(), i.name())
        self._ids[idx] = (i, True)
        
        i.setScope(self)

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
            namespace = i.namespace()
            name = i.name()
            
        idx = self._canonName(namespace, name)
        
        try:
            (i, value) = self._ids[idx]
            return value if return_val else i
        
        except KeyError:
            return None        
        
    def lookup(self, id):
        """Looks up an ID in the module's namespace. 
        
        id: string or ~~ID - Either the name of the ID to lookup (see
        ~~Scope.add for the lookup rules used for qualified names), or the
        ~~ID itself. The latter is useful to check whether the ID is part of
        the scope's namespace.
        
        Returns: ~~ID - The ID, or None if the name is not found.
        """
        result = self._lookupID(id, False)
        if result:
            return result

        return self._parent.lookup(id) if self._parent else None

    def remove(self, id):
        """Removes an ID from the scope.
        
        id: ~~ID - The ID to remove.
        """
        idx = self._canonName(id.namespace(), id.name())
        try: 
            del self._ids[idx]
        except IndexError:
            pass
    
    # Overridden from Node.
    
    def resolve(self, resolver):
        for (i, val) in self._ids.values():
            i.resolve(resolver)
            
    def validate(self, vld):
        for (i, val) in self._ids.values():
            i.validate(vld)
    
    def output(self, printer):
        last = None
        for (i, val) in sorted(self._ids.values(), key=lambda (i, val): i.name()):
            
            # Do not print any internal IDs defined implicitly by the run-time
            # environment. 
            if i.location() and i.location().internal():
                continue

            # Don't print any IDs with a different namespace than the current
            # module. They are imported, and we print the import statement
            # instead.
            if i.namespace() and i.namespace() != printer.currentModule().name():
                continue
            
            # Don't print enum and bitset constants.
            if isinstance(i, id.Constant) and \
               (isinstance(i.type(), type.Bitset) or isinstance(i.type(), type.Enum)):
                   continue
            
            if last and last != i.__class__:
                printer.output()
            
            i.output(printer)
            
            if isinstance(i, id.Function): 
                if last == i.__class__:
                    printer.output()
                    
                if i.linkage() == id.Linkage.EXPORTED:
                    printer.output("export %s\n" % i.name(), nl=True)

            last = i.__class__
                    
    ### Overridden from Visitable.
    
    def visit(self, v):
        v.visitPre(self)

        # Sort the ID names so that we get a deterministic order. 
        for name in sorted(self._ids.keys()):
            (id, value) = self._ids[name]
            
            id.visit(v)
            if isinstance(value, visitor.Visitable):
                value.visit(v)
            
        v.visitPost(self)    
