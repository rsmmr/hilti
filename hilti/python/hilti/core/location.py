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
        
    def __str__(self):
        if not self._file:
            return "<no-location>"
        
        line = ":%d" % self._line if self._line > 0 else ""
        file = os.path.basename(self._file)
        
        return "%s%s" % (file, line)
        
