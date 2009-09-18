# $Id$
#
# The lexer. 

import parser

from core import *
from support import *
from support.parseutil import *

# Language keywords. They will turned into the corresponding all-uppercase
# token.
keywords = ("attribute", "if", "module", "type")

# Type keywords. Also see below. 
types = ("bytes", "string", "unit")

# Literals.
literals = ['(',')','{','}', '<', '>', '=', ',', ':', '*', ';', '+', '-', '*', '/', '|' ]

def _loc(t):
    return location.Location(t.lexer.parser.state._filename, t.lexer.lineno)

tokens = [
    "IDENT", "CONSTANT",
    "INT", "UINT",
    ] + [k.upper() for k in keywords + types]

# Type keywords not covered by types.

def t_INT(t):
    r'int(8|16|32|64)'
    t.value = type.SignedInteger(int(t.value[3:]))
    return t
   
def t_UINT(t):
    r'uint(8|16|32|64)'
    t.value = type.UnsignedInteger(int(t.value[4:]))
    return t

# Ignore white space. 
t_ignore  = ' \t'

# Track line numbers.
def t_newline(t):
    r'\n+'
    t.lexer.lineno += len(t.value)

# Ignore comments.
def t_comment(t):
    r'\#.*'
    pass

# Constants.

def t_CONST_INT(t):
    r'[+-]\d+'
    t.type = "CONSTANT"
    
    try:
        t.value = constant.Constant(int(t.value), type.SignedInteger(64), location=_loc(t))
    except ValueError:
        error(t, "cannot parse signed integer %s" % t.value)
        t.value = constant.Constant(0, type.SignedInteger(64), location=_loc(t))
        
    return t

def t_CONST_UINT(t):
    r'\d+'
    t.type = "CONSTANT"
    
    try:
        t.value = constant.Constant(int(t.value), type.UnsignedInteger(64), location=_loc(t))
    except ValueError:
        error(t, "cannot parse unsigned integer %s" % t.value)
        t.value = constant.Constant(0, type.UnsignedInteger(64), location=_loc(t))
        
    return t

def t_CONST_STRING(t):
    '"([^\n"]|\\\\")*"'
    t.type = "CONSTANT"
    t.value = constant.Constant(t.value[1:-1], type.String(), location=_loc(t))
    return t

def t_CONST_BYTES(t):
    'b"([^\n"]|\\\\")*"'
    t.type = "CONSTANT"
    t.value = constant.Constant(t.value[2:-1], type.Bytes(), location=_loc(t))
    return t

# Identifiers.
def t_IDENT(t):
    r'[_a-zA-Z]([a-zA-Z0-9._]|::)*'
    if t.value in keywords or t.value in types:
        token = t.value.upper()
        assert token in tokens
        t.type = token
        
    return t    
    
# Error handling.
def t_error(t):
    parser.error(t.lexer, "cannot parse input '%s...'" % t.value[0:10], lineno=t.lexer.lineno)
    
