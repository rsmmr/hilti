# $Id$

import util

class Visitable(object):
    """Visitable is the base class for classes that provide ~~Visitor support.
    To enable a class to be visited by a ~~Visitor, derive the class from
    Visitable. Override the methods :meth:`visit` and :meth:`visitorSubType`
    as needed."""
    
    def visit(self, visitor):
        """Must be overridden if the derived class has any childs that need to
        be visited as well; these childs must likewise be derived from
        Visitable. 
        
        If the derived class provides its own implemention, it must
        
        * first call the ~~visitPre method of *visitor*,
        
        * then call the ~~visit method of *visitor* for each child, and
        
        * finally call the ~~visitPost method of *visitor*.
        
        """
        visitor._visitSelf(self)
        
    def visitorSubType(self, visitor):
        """Can be overriden to allow visitor functions to be more specific in
        their choice of which nodes to handle. The method can return any
        value, which is then matched against the *subtype* parameter of the
        ~~Visitor object's ~when, ~~pre, and ~~post methods."""

class Visitor(object):
    """The Visitor class allows the traversal of a tree structure formed by
    node of ~~Visitable instances. It is the main tool to traverse an |ast|. 
    
    To traverse an such a tree, perform the following steps:
    
    1. Derive a class from Visitor, and instantiate an object of the new class.
    
    2. Define a visitor functions for each subclass of ~~Visitable you are
       interested in. Decorate all these functions with +@<obj>.<meth>+ where 
       +<obj>+ is the object instantiated in (1), and +<meth>+ is one of
       :meth:`when`, :meth:`pre`, or :class:`post`.
    
    3. Call :meth:`visit` with the root node. 
    
    When instantiating a Visitor, the boolean *all* indicates how to proceed
    when multiple visitor function match a node: if *True*, all of them are
    called; if *False*, only the one with the most specific type is called.
    """    
    def __init__(self, all=False):
        self._all = all
        self._pres = []
        self._posts = []
        self._visits = []

    def visit(self, node):
        """Traverses the tree rooted at ~~Visitable *node*."""
        node.visit(self)
        
    def visitPre(self, obj):
        """Must be called by a derived class' :meth:`visit` method if it has
        any child nodes to traverse. *obj* is the child itself."""
        self._dovisit(obj, self._pres)
        self._visitSelf(obj)
    
    def visitPost(self, obj):
        """Must be called by a derived class' :meth:`visit` method before it
        traverses any child nodes. *obj* is the child itself."""
        self._dovisit(obj, self._posts)
    
    def _visitSelf(self, obj):
        """Must be called by a derived class' :meth:`visit` method after it
        traverses any child nodes. *obj* is the child itself."""
        self._dovisit(obj, self._visits)

    # Decorators
    def when(self, t, subtype=None):
        """Class-decorator for a function, indicating that the function
        handles instances of the class *t* (which must be derived from
        ~~Visitable). If *subtype* is given, its value is matched against the
        one returned by the instance's ~~visitSubType; and the method is only
        called if the two match."""
    	def f(func):
            self._visits += [(t, subtype, func)]
        return f    
    
    def pre(self, t, subtype=None):
        """Class-decorator for a function, indicating that the function
        handles instances of the class *t* (which must be derived from
        ~~Visitable). If the instance of *t* has any childs, the method will
        be called *before* those are traversed. If *subtype* is given, its
        value is matched against the one returned by the instance's
        ~~visitSubType; and the method is only called if the two match."""
    	def f(func):
            self._pres += [(t, subtype, func)]
        return f    
    
    def post(self, t, subtype=None):
        """Class-decorator for a function, indicating that the function
        handles instances of the class *t* (which must be derived from
        ~~Visitable). If the instance of *t* has any childs, the method will
        be called *after* those are traversed. If *subtype* is given, its
        value is matched against the one returned by the instance's
        ~~visitSubType; and the method is only called if the two match."""
    	def f(func):
            self._posts += [(t, subtype, func)]
        return f    

    def skipOthers(self):
        """Can be called from a visitor function to indicate that all
        subsequent visitor functions matching the same object are not to be
        executed any more."""
        self._skipothers = True
    
    def _dovisit(self, obj, dict):
        
        self._skipothers = False
        
        candidate = None
        
        for (type, arg, func) in dict:
            
            if isinstance(obj, type):
                argmatch = (not arg or isinstance(obj.visitorSubType(), arg))
                
                ## Run all 
                
                if self._all and not self._skipothers:
                    if argmatch:
                        func(self, obj)
                    continue
                
                ## Find most specific.
                
                if not candidate:
                    if argmatch:
                        # First match.
                         candidate = (type, arg, func)
                
                else:
                    # Subsequent match. Is it more specific?
                    (othertype, otherarg, otherfunc) = candidate
                    
                    if othertype == type:
                        # Same type.
                        if arg and argmatch:
                            if not otherarg:
                                # We have a matching arg but the other doesn't.
                                candidate = (type, arg, func)
                                
                            elif isinstance(arg, otherarg):
                                # Our arg is subclass of the other.
                                candidate = (type, arg, func)
                                
                    else:
                    	if issubclass(type, othertype):
                            # Our object is subclass of the other.
                            candidate = (type, arg, func)

        # See if we have the matching candidate in our dictionary.
        if not self._all and candidate:
            (type, arg, func) = candidate
            func(self, obj)
                
    
