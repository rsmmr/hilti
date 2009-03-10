# $Id$
"""Provides a set of general utility functions used across the compiler."""

import os
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
    
def expand_escapes(str):
    r"""Expands escape sequences. The following escape sequences are supported:
    
    ============   ========================
    Escape         Result
    ============   ========================
    \\ \\            Backslash
    \\n             Line feed
    \\r             Carriage return
    \\t             Tabulator
    \\uxxxx         16-bit Unicode codepoint
    \\uxxxxxxxx     32-bit Unicode codepoint
    ============   ========================
    
    
    str: string - The string to expand. 
    
    Returns: unicode - A unicode string with escape sequences expanded.
    
    Raises: ValueError - Raised when an illegal sequence was found. 
    """
    
    result = u""
    i = 0
    
    while i < len(str):
        
        c = str[i]
        i += 1
        
        try:
            if c == "\\":
                c = str[i]
                i += 1
                
                if c == "\\":
                    result += "\\"
                elif c == "n":
                    result += "\n"
                elif c == "r":
                    result += "\r"
                elif c == "t":
                    result += "\t"
                elif c == "\"":
                    result += "\""
                elif c == "u":
                    result += unichr(int(str[i:i+4], 16))
                    i += 4
                elif c == "U":
                    result += unichr(int(str[i:i+8], 16))
                    i += 8
                else:
                    raise ValueError
                
            else:
                result += c
                    
        except IndexError:
            raise ValueError
            
    return result
                
def findFileInPaths(filename, dirs, lower_case_ok=False):
    """Searches a file in a list of directories.
    
    filename: string - The name of the file to search.
    dirs: list of strings - The directories to search in.
    lower_case_ok: bool - If true, finding a lower-case version of the file
    name is ok too. 
    
    Returns: string or None - The full path of the file if found, or None if
    not.
    """
    
    for dir in dirs:
        
        for name in (filename, filename.lower()):
            fullpath = os.path.realpath(os.path.join(dir, name))
            if os.path.exists(fullpath) and os.path.isfile(fullpath):
                return fullpath
        
    return None
                
            
            
            
                
            
    
