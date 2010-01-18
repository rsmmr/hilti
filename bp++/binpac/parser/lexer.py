# $Id$
#
# The lexer. 

import binpac.core.constant as constant
import binpac.core.type as type
import binpac.core.location as location
import binpac.support.util as util
import binpac.support.parseutil as parseutil

# Language keywords. They will be turned into the corresponding all-uppercase
# token.
keywords = ["module", "type", "export", "unit", "print", "list", "global", "const"]

# Keywords for simple types, and what we turn them into.
types = {
    "bytes": type.Bytes,
    "regexp": type.RegExp,
    "string": type.String,
    }

# Literals.
literals = ['(',')','{','}', '[', ']', '<', '>', '=', ',', ':', '*', ';', '+', '-', '*', '/', '|', '.', '!']

def _loc(t):
    return location.Location(t.lexer.parser.state._filename, t.lexer.lineno)

tokens = [
    "IDENT", "CONSTANT", "PACTYPE", "ATTRIBUTE", "PROPERTY",
    "EQUAL", "UNEQUAL",
    ] + [k.upper() for k in keywords]

# Operators with more than one character.

def t_EQUAL(t):
    r'=='
    return t

def t_UNEQUAL(t):
    r'!='
    return t
    
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

def t_CONST_REGEXP(t):
    r'/[^\n]*?(?<!\\)/'    
    t.type = "CONSTANT"
    t.value = constant.Constant(t.value[1:-1], type.RegExp(), location=_loc(t))
    return t

def t_CONST_BOOL(t):
    'True|False'
    t.type = "CONSTANT"
    t.value = constant.Constant(t.value == "True", type.Bool(), location=_loc(t))
    return t

# Identifiers.
def t_IDENT(t):
    r'[%&]?[_a-zA-Z]([a-zA-Z0-9_]|::)*'
    
    if t.value.startswith("&"):
        t.value = t.value[1:]
        t.type = "ATTRIBUTE"
        
    elif t.value.startswith("%"):
        t.value = t.value[1:]
        t.type = "PROPERTY"
    
    elif t.value in types:
        t.value = types[t.value]()
        t.type = "PACTYPE"
    
    elif t.value in keywords or t.value in types:
        token = t.value.upper()
        assert token in tokens
        t.type = token
        
    return t    
    
# Error handling.
def t_error(t):
    parseutil.error(t.lexer, "cannot parse input '%s...'" % t.value[0:10], lineno=t.lexer.lineno)
    
