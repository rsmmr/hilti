# $Id$

import util

class Cache(object):
    """A class for creating objects on-demand. Once an object has been
    created, it will be cached for later retrieval, avoiding to recreate it.
    """
    def __init__(self):
        super(Cache, self).__init__()
        self._cache = {}
        
    def cache(self, key, compute):
        """Caches onces computed values for later reuse. The first time the
        method is called for a particular *key*, it calls the *compute*
        function to compute a value (of arbitrary type), which is cached
        internally and then returned. On subsequent calls with the same *key*,
        the cached value is returned directly and *compute* is ignored. 
        
        key: string or any - If a string, the string comparision is used to
        match the key with subsequent calls. Otherwise, ``id(key)`` is used.
        
        compute: function - A function returning the value to be cached. The
        function will only be called if *cache* has not been called with the
        same key before. The function must not return None. *compute* can be
        set to None if the caller can guarentee that for the *key* a value has
        already been cached earlier.
        
        Note: Relying on ``id`` for determing the key can lead to
        subtle problems if garbage collection recycles an older
        object's memory.
        
        Note: The function detects if the *compute* recursively call
        *cache* again with the same object; it will abort with an
        internal error in that case. If that's not desired, one can
        preset the cache'd value with ~~setCacheEntry.
        """
        key = self._key(key)
            
        try:
            value = self._cache[key]
            if not value:
                util.internal_error("infinite recursion in Cache.cache() detected")
            return value    
        except KeyError:
            assert compute
            self._cache[key] = None
            try:
                value = compute()
            except KeyError, e:
                util.internal_error("KeyError raised for %s in compute() function" % e)
                
            assert value != None
            self._cache[key] = value
            return value

    def isCached(self, key):
        """Checks whether has already been cached for a key. 
        
        key: string or any - See ~~cache for details.
        
        Returns: bool - True if ~~cache() has already been called for that key.
        """
        key = self._key(key)
        return key in self._cache

    def setCacheEntry(self, key, value):
        """Initializes a cache entry explicity. This can be useful
        if the *compute* function passed to ~~cache might
        recursively access the same *key* during execution.
        
        key: string or any - See ~~cache for details.
        
        value: any - The value to associate with the key.  The value
        must not be None.
        """
        key = self._key(key)
            
        assert value
        self._cache[key] = value

    def removeCacheEntry(self, key):
        """Removes a cache entry.
        
        key: string or any - See ~~cache for details.
        """
        key = self._key(key)
        del self._cache[key]
        
    def _key(self, key):
        assert key
        if not isinstance(key, str):
            key = id(key)
            
        return key
        
