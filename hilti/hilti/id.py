# $Id$

import llvm.core

import constant
import copy
import location
import node
import type
import util

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

    INIT = 3
    """A ~~Function with linkage INIT will be executed once before any other
    (non-INIT) functions defined inside the same ~~Module. The function is
    only visible inside the ~~Module it is implemented in. A function with
    linkage INIT must not receive any arguments and it must not return any
    value. Note that if an INIT function throws an exception, execution of the
    program will be terminated.
    
    Only functions can have linkage INIT. 
    
    Todo: Init function cannot have local variables at the moment because they
    are compiled with the ~~C linkage.
    """
    
class ID(node.Node):
    """Binds a name to a type.
    
    name: string - The name of the ID.
    
    type: ~~Type - The type of the ID.
    
    linkage: ~~Linkage - The linkage of the ID.

    imported: bool - True to indicate that this ID was imported from another
    module. This does not change the semantics of the ID in any way but can be
    used to avoid importing it recursively at some later time.

    namespace: string - The ID's namespace, if any.

    location: ~~Location - A location to be associated with the ID. 
    
    Note: Derived classes must use ~~codegen to generate instantiate the IDs
    (where appropiate). To access/modify their values, use ~~llvmLoad and
    ~~llvmStore.
    """
    
    def __init__(self, name, type, linkage=Linkage.LOCAL, imported=False, namespace=None, location=None):
        assert name
        assert type
        super(ID, self).__init__(location)
        self._name = name
        self._type = type
        self._linkage = linkage
        self._imported = imported
        self._namespace = namespace
        self._scope = None

    def name(self):
        """Returns the ID's name.
        
        Returns: string - The ID's name.
        """
        return self._name

    def setName(self, name): 
        """Sets the ID's name.
        
        name: string - The new name.
        """
        self._name =name

    def namespace(self):
        """Returns the ID's namespace.
        
        Returns: string - The ID's namespace.
        """
        return self._namespace if self._namespace else ""
    
    def setNamespace(self, namespace):
        """Sets the ID's namespace.
        
        string - The ID's namespace.
        """
        self._namespace = namespace
        
    def scope(self):
        """Returns the ID's scope.
        
        Returns: ~~Scope - The ID's scope, or None if it has not yet been
        added to a scope. 
        """
        return self._scope
    
    def setScope(self, scope):
        """Sets the ID's scope.
        
        scope: ~~Scope - The ID's scope.
        
        Note: This method will be automatically called when the ID gets
        inserted into a scope. 
        """
        self._scope = scope
        
    def linkage(self):
        """Returns the ID's linkage.
        
        Returns: ~~Linkage - The ID's linkage.
        """
        return self._linkage
    
    def setLinkage(self, linkage):
        """Sets the ID's linkage.
        
        linkage: string - The new linkage.
        """
        self._linkage = linkage
            
    def type(self):
        """Returns the ID's type.
        
        Returns: ~~Type - The ID's type.
        """
        return self._type

    def setType(self, t):
        """Sets the ID's type.
        
        t: ~~Type - The new type.
        """
        self._type = t
    
    def imported(self):
        """Returns whether the ID was imported from another module.
        
        Returns: bool - True if the ID was imported.
        """
        return self._imported

    def setImported(self):
        """Marks the ID as being imported from another module."""
        self._imported = True

    def scopedName(self, printer):
        """Returns the name of the ID with its full namespace. However, the
        returned name is only fully qualified if the ID is defined outside of
        the current module as returned by Printer.currentModule.
        
        Returns: string - The name.
        """
        mod = printer.currentModule()
        if mod and self._namespace and self._namespace != mod.name():
            return "%s::%s" % (self._namespace, self._name)
        
        return self._name
        
    def __str__(self):
        if self._namespace:
            return "%s::%s" % (self._namespace, self._name)
        else:
            return self._name

    ### Methods for derived classes to override. 
    
    def llvmLoad(self, cg):
        """Loads the value the ID refers to.

        Must be overridden by derived classes.
        
        cg: The current code generator.
        
        Returns: llvm.core.Value - The value.
        """
        util.internal_error("llvmLoad not implemented for %s" % repr(self))
        
    def llvmStore(self, cg, val):
        """Stores a value in the what ID refers to.

        Must be overridden by derived classes.
        
        cg: The current code generator.
        
        val: llvm.core.Value - The value to store.
        
        """
        util.internal_error("llvmStore not implemented for %s" % repr(self))

    def clone(self):
        """Returns a deep-copy of the ID.

        Derived classes must overide this method.
        
        Returns: ~~ID - The deep-copy.
        
        Note: We need this because Python's deepcopy can't copy LLVM's C
        objects.
        """
        util.internal_error("id.ID.clone not overridden")

    ### Overidden from node.Node.

    def validate(self, vld):
        self._type.validate(vld)
        
    def resolve(self, resolver):
        self._type = self._type.resolve(resolver)

    def output(self, printer):
        printer.printComment(self)

        linkage = ""
            
        if self.linkage() == Linkage.INIT:
            linkage = "init "
        
        printer.output(linkage)
        
class Constant(ID):
    """An ID representing a constant value. See ~~ID for arguments not listed
    in the following.
    
    c: ~~Operand or ~~Constant - The constant to initialize the ID with.
    
    See ~~ID for the other arguments.
    """
    def __init__(self, name, c, linkage=Linkage.LOCAL, imported=False, namespace=None, location=None):
        # Can't import operand due to circular dependencies.
        if not isinstance(c, constant.Constant):
            assert c.constant()
            c = c.constant()
            
        super(Constant, self).__init__(name, c.type(), linkage, imported, namespace, location=location)
        self._const = c
        self._llvm = None
        
    def value(self):
        """Returns the const value. 
        
        Returns: ~~Constant - The constant.
        """
        return self._const

    ### Overidden from ID.

    def llvmLoad(self, cg):
        if not self._llvm:
            # If we have not already created the constant previously, do it now.
            val = self._const.llvmLoad(cg)
            self._llvm = cg.llvmNewGlobalConst(cg.nameGlobal(self), val)
        
        return cg.builder().load(self._llvm)
        
    def llvmStore(self, cg, val):
        util.internal_error("llvmStore called for ConstID")
        
    def clone(self):
        return Constant(self.name(), self._const, self.linkage(), self.imported(), self.namespace(), location=self.location())
    
    ### Overidden from node.Node.

    def validate(self, vld):
        ID.validate(self, vld)
        self._const.validate(vld)

        if not isinstance(self.type(), type.ValueType):
            vld.error(self, "type of const must be a value type")
            return 
        
        if not self.type().instantiable():
            vld.error(self, "cannot create constants of type %s" % self.type())
        
    def output(self, printer):
        ID.output(self, printer)
        
        printer.output("const ")
        printer.printType(self.type())
        printer.output(" %s = " % self._name)
        self._const.output(printer)
        printer.output(nl=True)
        
    def canonify(self, canonifier):
        self._const.canonify(canonifier)
        
class Local(ID):
    """An ID representing a local function variable. 
    
    See ~~ID for arguments.
    """
    def __init__(self, name, type, linkage=Linkage.LOCAL, imported=False, namespace=None, location=None):
        super(Local, self).__init__(name, type, linkage, imported, namespace, location=location)

    ### Overidden from ID.

    def llvmLoad(self, cg):
        return cg.llvmLoadLocalVar(self)
    
    def llvmStore(self, cg, val):
        return cg.llvmStoreLocalVar(self, val)

    def clone(self):
        return Local(self.name(), self.type(), self.linkage(), self.imported(), self.namespace(), location=self.location())
        
    ### Overidden from node.Node.

    def validate(self, vld):
        ID.validate(self, vld)

        if not isinstance(self.type(), type.ValueType):
            vld.error(self, "type of local must be a value type")
            return 
        
        if not self.type().instantiable():
            vld.error(self, "cannot create local variable of type %s" % self.type())
    
    def output(self, printer):
        printer.printComment(self)
        printer.printType(self.type())
        printer.output(" %s" % self._name)
    
class Parameter(ID):
    """An ID representing a function parameter. 
    
    See ~~ID for arguments.
    """
    def __init__(self, name, type, linkage=Linkage.LOCAL, imported=False, namespace=None, location=None):
        super(Parameter, self).__init__(name, type, linkage, imported, namespace, location=location)

    ### Overidden from ID.

    def llvmLoad(self, cg):
        return cg.llvmLoadLocalVar(self)
    
    def llvmStore(self, cg, val):
        return cg.llvmStoreLocalVar(self, val)

    def clone(self):
        return Parameter(self.name(), self.type(), self.linkage(), self.imported(), self.namespace(), location=self.location())

    def value(self):
        return None
    
    ### Overidden from node.Node.

    def validate(self, vld):
        ID.validate(self, vld)
    
        if not isinstance(self._type, type.ValueType):
            vld.error(self, "parameter type must be a value type")
            return 
        
    def output(self, printer):
        printer.printType(self.type())
        printer.output(" %s" % self._name)

class Global(ID):
    """An ID representing a module-global variable. 
    
    init: ~~Operand - The operand to initialize the global with, or None for
    initialization with the default value.
    
    See ~~ID for the other arguments.
    """
    def __init__(self, name, type, init, linkage=Linkage.LOCAL, imported=False, namespace=None, location=None):
        super(Global, self).__init__(name, type, linkage, imported, namespace, location=location)
        self._init = init
        
    def value(self):
        """Returns the initialization value of the global.
        
        Returns: ~~Operand - The init value, or None if none was set.
        """
        return self._init

    ### Overidden from ID.

    def llvmLoad(self, cg):
        return cg.llvmLoadGlobalVar(self)
    
    def llvmStore(self, cg, val):
        return cg.llvmStoreGlobalVar(self, val)

    def clone(self):
        return Global(self.name(), self.type(), self._init, self.linkage(), self.imported(), self.namespace(), location=self.location())
    
    ### Overidden from node.Node.

    def validate(self, vld):
        ID.validate(self, vld)

        if not isinstance(self.type(), type.ValueType):
            vld.error(self, "type of global must be a value type")
            return 
        
        if not self.type().instantiable():
            vld.error(self, "cannot create global variable of type %s" % self.type())

        if self._init:
            self._init.validate(vld)
            
    def output(self, printer):
        ID.output(self, printer)
        printer.output("global ")
        printer.printType(self.type())
        printer.output(" %s" % self._name)
        if self._init:
            printer.output(" = ")
            self._init.output(printer)
        printer.output()

    def codegen(self, cg):
        cg.llvmNewGlobalVar(self)
    
class Function(ID):        
    """An ID representing a function definition. 
    
    fname: string - The name of the function.
    
    ftype: ~~type.Function - The function's type.
    
    func: ~~Function - The function.
    
    See ~~ID for the other arguments.
    """
    def __init__(self, fname, ftype, func, linkage=Linkage.LOCAL, imported=False, namespace=None, location=None):
        super(Function, self).__init__(fname, ftype, linkage, imported, namespace, location=location)
        self._func = func

    def function(self):
        """Returns the function.
        
        Returns: ~~Function - The function.
        """
        return self._func
    
    ### Overidden from ID.

    def llvmLoad(self, cg):
        util.internal_error("llvmLoad called for Function")
        
    def llvmStore(self, cg, val):
        util.internal_error("llvmStore called for Function")

    def clone(self):
        return Function(self.name(), self.type(), self._func, self.linkage(), self.imported(), self.namespace(), location=self.location())
        
    ### Overidden from node.Node.

    def resolve(self, resolver):
        self._func.resolve(resolver)
        self._type = self._type.resolve(resolver)
        
    def validate(self, vld):
        ID.validate(self, vld)
        self._func.validate(vld)
        self._type.validate(vld)
        
    def output(self, printer):
        ID.output(self, printer)
        self._func.output(printer)
    
    def canonify(self, canonifier):
        pass
        #self._func.canonify(canonifier)

    def codegen(self, cg):
        self._func.codegen(cg)
        
    # Visitor support
    def visit(self, visitor):
        self._func.visit(visitor)
        
class Type(ID):
    """An ID representing a type-declaration. 
    
    See ~~ID for arguments.
    """
    def __init__(self, name, type, linkage=Linkage.LOCAL, imported=False, namespace=None, location=None):
        super(Type, self).__init__(name, type, linkage, imported, namespace, location)

    ### Overidden from ID.

    def llvmLoad(self, cg):
        util.internal_error("llvmLoad called for Type")
        
    def llvmStore(self, cg, val):
        util.internal_error("llvmStore called for Type")

    def clone(self):
        return Type(self.name(), self.type(), self.linkage(), self.imported(), self.namespace(), location=self.location())
        
    ### Overidden from node.Node.

    def validate(self, vld):
        ID.validate(self, vld)
        self.type().validate(vld)

    def output(self, printer):
        ID.output(self, printer)
        printer.output("type")
        printer.output(" %s = " % self._name)
        self.type().output(printer)
        printer.output(nl=True)
        
class Unknown(ID):
    """An ID representing a not yet resolved identifier. Such an ID can be
    used in place of the correct one if the latter is not known yet. It must
    later be replaced with it, usually by some object's ~~resolve method.
    
    name: string - The name oft the identifier
    
    scope: ~~Scope - The scope in which the identifier is to be resolved
    eventually.
    """
    def __init__(self, name, scope, location=None):
        super(Unknown, self).__init__(name, type.Unknown(name, location=location), Linkage.LOCAL, False, None, location=location)
        self._scope = scope

    def scope(self):
        return self._scope
        
    ### Overidden from ID.

    def llvmLoad(self, cg):
        util.internal_error("llvmLoad called for Unknown")
        
    def llvmStore(self, cg, val):
        util.internal_error("llvmStore called for Unknown")

    def clone(self):
        return Unknown(self.name(), location=self.location())
        
    ### Overidden from node.Node.

    def validate(self, vld):
        vld.error(self, "unknown identifier %s" % self.name())

    def output(self, printer):
        ID.output(self, printer)
        printer.output("<unknown>")

