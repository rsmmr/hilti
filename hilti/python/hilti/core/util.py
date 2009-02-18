# $Id$
"""This module provides a set of general utility function used across the
compiler."""

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
    """Reports an error message to the user. *msg* is a string with the
    message to report. If *component* is a non-empty string, it will prefix
    the message to indicate which component reported the problem. If *context*
    is a non-empty string (or an object with a string representation), if will
    be inserted into the message at a point suitable to report location
    information; most commonly, *context* is set to a relevant ~~Location
    object. Finally, if *fatal* is *True*, the execution of the current
    process is terminated after the error message has been printed."""
    _print(component, "error", msg, context) 
    if fatal: 
        sys.exit(1)
    
def warning(msg, component=None, context=None):
    """Reports an warning message to the user. *msg* is a string with the
    message to report. If *component* is a non-empty string, it will prefix
    the message to indicate which component reported the problem. If *context*
    is a non-empty string (or an object with a string representation), if will
    be inserted into the message at a point suitable to report location
    information; most commonly, *context* is set to a relevant ~~Location
    object."""
    _print(component, "warning", msg, context)
    
def debug(msg, component=None, context=None):
    """Reports an debug message to the user if debug output is enabled. *msg*
    is a string with the message to report. If *component* is a non-empty
    string, it will prefix the message to indicate which component reported
    the problem. If *context* is a non-empty string (or an object with a
    string representation), if will be inserted into the message at a point
    suitable to report location information; most commonly, *context* is set
    to a relevant ~~Location object."""
    _print(component, "debug", msg, context)
    
def internal_error(msg, component=None, context=None):
    """Reports an internal error to the user and aborts execution immediately.
    *msg* is a string with the message to report. If *component* is a
    non-empty string, it will prefix the message to indicate which component
    reported the problem. If *context* is a non-empty string (or an object
    with a string representation), if will be inserted into the message at a
    point suitable to report location information; most commonly, *context* is
    set to a relevant ~~Location object."""
    _print(component, "internal error", msg, context)
    assert False
    
