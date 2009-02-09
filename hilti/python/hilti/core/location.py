# $Id$

import os.path

class Location(object):
    """A Location stores a filename *name* (string) and a line number *line*
    (integer). If *line* is zero, the line number is assumed to be
    unspecified. Locations are associated with other objects derived from
    HILTI source files. If any input errors are found, the user can then be
    pointed to the offending code segment."""
    def __init__(self, name = None, line = 0):
        self._file = name
        self._line = int(line)
        
    def __str__(self):
        if not self._file:
            return "<no-location>"
        
        line = ":%d" % self._line if self._line > 0 else ""
        file = os.path.basename(self._file)
        
        return "%s%s" % (file, line)
        
