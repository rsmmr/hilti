# $Id$
#
# Location information

class Location(object):
    def __init__(self, file = None, line = 0):
        self._file = file
        self._line = int(line)
        
    def __str__(self):
        file = self._file if self._file else "<no-location>"
        line = ":%d" % self._line if self._line > 0 else ""
        
        return "%s%s" % (file, line)
        
