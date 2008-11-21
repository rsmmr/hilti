# $Id$
#
# Global and local scopes.

import id
import location
import util
    
class Scope(object):
    def __init__(self):
        self._ids = {}

    def lookup(self, name):
        try:
            return self._ids[name]
        except KeyError:
            return None
        
    def insert(self, id):
        if id.name() in self._ids:
            util.error("name %s already defined" % id.name())
        
        self._ids[id.name()] = id
        
    def remove(self, name):
        del self._ids[id.name()]

    def IDs(self, type):
        return self.values()

    def __getitem__(self, key):
        return self._ids[key]

    def __contains__(self, key):
        return key in self._ids
    
    # Visitor support.
    def dispatch(self, visitor):
        visitor.visit_pre(self)
        
        for id in self._ids.values():
            id.dispatch(visitor)

        visitor.visit_post(self)
        



