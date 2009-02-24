# $Id$
"""Provides a set of general utility functions used across the compiler."""

import sys

def _print(prefix1, prefix2, msg, context):
    
    if prefix1:
        prefix = "%s %s" % (prefix1, prefix2)
        
    else:
        prefix = prefix2
    
    if context:
        print >>sys.stderr, "%s:" % context,
    
    print >>sys.stderr, "%s, %s" % (prefix, msg)

def error(msg, component=None, context=None, fatal=True): 
    """Reports an error message to the user. 
    
    msg: string - The message to report. 
    
    component: string - Optional string that will prefix the message
    to indicate which component reported the problem.
    
    context: string - Optional string that will be inserted into the
    message at a point suitable to report location information; most
    commonly *context* will be the relevant ~~Location object (which
    converts into a string).

    *fatal*: bool - If True, the execution of the process is
    terminated after the error message has been printed.
    """
    _print(component, "error", msg, context) 
    if fatal: 
        sys.exit(1)
    
def warning(msg, component=None, context=None):
    """Reports an warning message to the user.
    
    msg: string - The message to report. 
    
    component: string - Optional string that will prefix the message
    to indicate which component reported the problem.
    
    context: string - Optional string that will be inserted into the
    message at a point suitable to report location information; most
    commonly *context* will be the relevant ~~Location object (which
    converts into a string).
    """
    _print(component, "warning", msg, context)
    
def debug(msg, component=None, context=None):
    """Reports an debug message to the user if debug output is enabled.
    
    msg: string - The message to report. 
    
    component: string - Optional string that will prefix the message
    to indicate which component reported the problem.
    
    context: string - Optional string that will be inserted into the
    message at a point suitable to report location information; most
    commonly *context* will be the relevant ~~Location object (which
    converts into a string).
    """
    _print(component, "debug", msg, context)
    
def internal_error(msg, component=None, context=None):
    """Reports an internal message to the user and aborts
    execution.
    
    msg: string - The message to report. 
    
    component: string - Optional string that will prefix the message
    to indicate which component reported the problem.
    """
    _print(component, "internal error", msg, context)
    assert False
    
