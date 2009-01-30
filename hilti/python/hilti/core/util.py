# $Id$
# 
# Misc utility functions.

import sys

def _print(prefix1, prefix2, msg, context):
    
    if prefix1:
        prefix = "%s %s" % (prefix1, prefix2)
        
    else:
        prefix = prefix2
    
    if context:
        print >>sys.stderr, "%s:" % context,
    
    print >>sys.stderr, "%s, %s" % (prefix, msg)

def error(msg, prefix=None, context=None, fatal=True):
    _print(prefix, "error", msg, context)
    if fatal:
        sys.exit(1)
    
def warning(msg, prefix=None, context=None):
    _print(prefix, "warning", msg, context)
    
def debug(msg, prefix=None, context=None):
    _print(prefix, "debug", msg, context)
    
def internal_error(msg, prefix=None, context=None):
    _print(prefix, "internal error", msg, context)
    sys.exit(1)
    
