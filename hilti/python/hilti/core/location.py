# $Id$
#
# Location information

import os.path

class Location(object):
    def __init__(self, file = None, line = 0):
        self._file = file
        self._line = int(line)
        
    def __str__(self):
        if not self._file:
            return "<no-location>"
        
        line = ":%d" % self._line if self._line > 0 else ""
        file = os.path.basename(self._file)
        
        return "%s%s" % (file, line)
        
