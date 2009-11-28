# $Id$

import util

class Visitable(object):
    """Base class for classes that provide visitor support. To enable a class
    to be visited by a ~~Visitor, derive the class from Visitable. Override
    the methods :meth:`visit` and :meth:`visitorSubType` as needed."""

    def __init__(self):
        super(Visitable, self).__init__()
    
    def visit(self, visitor):
        """Must be overridden if the derived class has any childs that need to
        be visited as well; these childs must likewise be derived from
        Visitable. 
        
        If the derived class provides its own implemention, it must
        
        * first call the ~~visitPre method of *visitor*,
        
        * then call the ~~visit method of *visitor* for each child, and
        
        * finally call the ~~visitPost method of *visitor*.
        
        visitor: ~~Visitor - The current visitor object.
        """
        visitor._visitSelf(self)
        
    def visitorSubType(self, visitor):
        """Can be overriden to allow visitor functions to be more specific in
        their choice of which nodes to handle. The method can return any
        value, which is then matched against the *subtype* parameter of the
        ~~Visitor object's ~when, ~~pre, and ~~post methods.
        
        visitor: ~~Visitor - The current visitor object.
        """

class Visitor(object):
    """Implements the traversal of a tree structure. The class traverses
    nodes formed by instances of ~~Visitable. It is the primary mechanism to
    traverse an |ast|. 
    
    To traverse a tree of ~~Visitable objects, perform the following steps:
    
    1. Derive a class from Visitor, and instantiate an object of the new class.
    
    2. Define a visitor functions for each subclass of ~~Visitable that you
       are interested in. Decorate all these functions with ``@<obj>.<meth>``
       where ``<obj>`` is the object instantiated in (1), and ``<meth>`` is
       one of :meth:`when`, :meth:`pre`, or :class:`post`.
    
    3. Call :meth:`visit` with the root node. 
    
    all: boolean - Indicates how to proceed when multiple visitor functions
    match a node: if *True*, all of them are called; if *False*, only the one
    with the most specific type is called.
    """    
    def __init__(self, all=False):
        super(Visitor, self).__init__()
        self._all = all
        self._pres = []
        self._posts = []
        self._visits = []

    def visit(self, node):
        """Traverses a tree. 
        
        node: ~~Visitable: The root node of the tree to traverse.
        """
        node.visit(self)
        
    def visitPre(self, obj):
        """Must be called by a ~~Visitable derived class' :meth:`visit` method
        before it traverse any child nodes. 
        
        obj: ~~Visitable: The ~~Visitable calling the method.
        """
        self._dovisit(obj, self._pres)
        self._visitSelf(obj)
    
    def visitPost(self, obj):
        """Must be called by an ~~Visitable derived class' :meth:`visit`
        method after it traverse any child nodes. 
        
        obj: ~~Visitable: The ~~Visitable calling the method.
        """
        self._dovisit(obj, self._posts)
    
    def _visitSelf(self, obj):
        """Must be called by an ~~Visitable derived class' :meth:`visit`
        method if it has no childs to traverse.
        
        obj: ~~Visitable: The ~~Visitable calling the method.
        """
        self._dovisit(obj, self._visits)

    # Decorators
    def when(self, t, subtype=None, always=False):
        """Class-decorator indicating which nodes a function will handle.
        
        t: ~~Visitable class - The node class the function will handle.
        
        subtype: any - If given, its value will be matched against the one
        returned by an ~~Visitable's ~~visitorSubType method; and the method is
        only called if the two match.
        
        always: bool - If true, run this function whenever it matches, but
        don't consider it otherwise for the search of a most-specific match.
        """
        def f(func):
            self._visits += [(t, subtype, func, always)]
        return f    
    
    def pre(self, t, subtype=None, always=False):
        """Class-decorator indicating which nodes a function will handle. If
        the handled node has any childs, the method will be called *before*
        those are traversed.  
        
        t: ~~Visitable class - The node class the function will handle.
        
        subtype: any - If given, its value will be matched against the one
        returned by an ~~Visitable's ~~visitorSubType method; and the method is
        only called if the two match.
        
        always: bool - If true, run this function whenever it matches, but
        don't consider it otherwise for the search of a most-specific match.
        """
    	def f(func):
            self._pres += [(t, subtype, func, always)]
        return f    
    
    def post(self, t, subtype=None, always=False):
        """Class-decorator indicating which nodes a function will handle. If
        the handled node has any childs, the method will be called *after*
        those are traversed.  
        
        t: ~~Visitable class - The node class the function will handle.
        
        subtype: any - If given, its value will be matched against the one
        returned by an ~~Visitable's ~~visitorSubType method; and the method is
        only called if the two match.
        
        always: bool - If true, run this function whenever it matches, but
        don't consider it otherwise for the search of a most-specific match.
        """
        def f(func):
            self._posts += [(t, subtype, func, always)]
        return f	

    def skipOthers(self):
        """Indicates that no further visitor functions should be called for
        the current object. Can be called from a visitor function to skip
        further handlers for the *same* object and proceed to the next node.
        """
        self._skipothers = True
    
    def _dovisit(self, obj, dict):
        
        self._skipothers = False
        
        candidate = None
        
        for (type, arg, func, always) in dict:
            
            if isinstance(obj, type):
                argmatch = (not arg or isinstance(obj.visitorSubType(), arg))
                
                ## Run all 
                
                if self._all and not self._skipothers:
                    if argmatch:
                        func(self, obj)
                    continue
                
                if always:
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
                
    
