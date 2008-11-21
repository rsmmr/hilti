# $Id$
# 
# Misc utility functions.

import sys

def _print(prefix, msg, context):
    if context:
        print >>sys.stderr, "%s - " % context,
    
    print >>sys.stderr, "%s: %s" % (prefix, msg)

def error(msg, context=None, fatal=True):
    _print("error", msg, context)
    if fatal:
        sys.exit(1)
    
def warning(str, context=None):
    _print("warning", msg, context)
    
def debug(str, context=None):
    _print("error", msg, context)
    
def internal_error(msg, context=None):
    _print("internal error", msg, context)
    sys.exit(1)
    
