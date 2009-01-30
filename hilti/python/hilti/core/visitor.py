# $Id$
#
# Visitor infrastructure.

class Visitor(object):
    
    # If all is False, of all matching handlers, only the most specific one is called. 
    # If all is True, all matching handlers are run.
    def __init__(self, all=False):
        self._all = all
        self._pres = []
        self._posts = []
        self._visits = []

    # Start traversing with object,
    def dispatch(self, obj):
        obj.dispatch(self)
        
    # Objects with childs call visit_pre() before going through the childs 
    # and visit_post() when we're done. visit_pre also call visit() in turn, 
    # and objects without childs call visit() directly.
    def visit_pre(self, obj, arg=None):
        self._dovisit(obj, self._pres)
        self.visit(obj)
    
    def visit_post(self, obj):
        self._dovisit(obj, self._posts)
    
    def visit(self, obj):
        self._dovisit(obj, self._visits)

    # Decorators
    def when(self, obj, arg=None):
    	def f(func):
            self._visits += [(obj, arg, func)]
        return f    
    
    def pre(self, obj, arg=None):
    	def f(func):
            self._pres += [(obj, arg, func)]
        return f    
    
    def post(self, obj, arg=None):
    	def f(func):
            self._posts += [(obj, arg, func)]
        return f    

    def skipOthers(self):
        self._skipothers = True
    
    def _dovisit(self, obj, dict):
        
        self._skipothers = False
        
        candidate = None
        
        for (type, arg, func) in dict:
            
            if isinstance(obj, type):
                argmatch = (not arg or isinstance(obj.visitorSubClass(), arg))
                
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
                
    
