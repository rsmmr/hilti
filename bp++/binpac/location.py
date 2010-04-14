# $Id$

import os.path

class Location(object):
    """Stores a file name and a line number.
    
    name: string - The file name. 
    
    line: int - The line number. If zero, the line is assumed to be
    not specified. 
    """
    def __init__(self, name, line = 0):
        self._file = name
        self._line = int(line)
        self._internal = False

    def file(self):
        """Returns the location's file name.
        
        Returns: string - The file name, or ``<no file>`` if none set.
        """
        return self._file if self._file else "<no file>"
    
    def line(self):
        """Returns the location's line number.
        
        Returns: int - The line number, or zero if none is set.
        """
        return self._line
    
    def markInternal(self):
        """Marks the location as one internal to parsing system.
        This can for example by used to differentiate user-defined
        IDs from those defined internally."""
        self._internal = True
        
    def internal(self):
        """Returns whether the function has been marked as internal.
        
        Returns: bool - True if ~~markInternal() has been called for
        this location.
        """
        return self._internal
    
    def __str__(self):
        if not self._file:
            return "<no-location>"
        
        line = ":%d" % self._line if self._line > 0 else ""
        file = os.path.basename(self._file)
        intern = " (internal)" if self._internal else ""
        
        return "%s%s%s" % (file, line, intern)
        
