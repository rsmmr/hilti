# $Id$
#
# The lexer. 

import socket
import struct
import re
import util 

import ply.lex

import hilti
import instruction

# All the instructions we support.
instructions = instruction.getInstructions().keys()

# All the data types we support.
types = [t.token() for t in hilti._types.values() if t.token()]
types += ["int8", "int16", "int32", "int64"]

# Keywords in addition to instructions and types.
keywords = {
	"import": "IMPORT",
	"declare": "DECLARE",
	"export": "EXPORT",
	"local": "LOCAL", 
    "global" : "GLOBAL", 
    "module": "MODULE",
    "Null": "NULL",
    "at": "AT",
    "after": "AFTER",
    "with": "WITH",
    "const": "CONST",
    "init": "INIT",
    "type": "TYPE",
    "iterator": "ITERATOR",
    }

all = set([t.upper() for t in types + keywords.values()])

tokens = [
   'IDENT', 
   'INSTRUCTION',
   'NL',
   'COMMENTLINE',
   'ATTR_DEFAULT',
   'ATTR_NOSUB',

   # Constants.
   'CLABEL',
   'CBOOL', 
   'CBYTES',
   'CSTRING',
   'CINTEGER',
   'CDOUBLE',
   'CADDRESS',
   'CNET',
   'CPORT', 
   'CREGEXP', 
   
] + [a for a in all]

literals = ['[', ']', '(',')','{','}', '<', '>', '=', ',', ':', '*', '|' ]

states = (
   # In nolines mode, newlines are ignored. Per default, they are passed on as token NL.
   ('nolines', 'inclusive'),
)

def t_CADDR4(t): # must come before DOUBLE.
    r'\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}(/\d{1,2})?'

    try:
        (mask, len) = t.value.split("/")
    except ValueError:
        mask = t.value
        len = 0
    
    addr = struct.unpack("!1L", socket.inet_pton(socket.AF_INET, mask))
    
    if not len:
        t.type = "CADDRESS"
        t.value = (0, long(addr[0]))
    else:
        t.type = "CNET"
        t.value = (0, long(addr[0]), 96 + int(len))
        
    return t

def t_TRUE(t):
    r'True'
    t.type = "CBOOL"
    t.value = True
    return t

def t_FALSE(t):
    r'False'
    t.type = "CBOOL"
    t.value = False
    return t

def t_CLABEL(t):
    r'@[_a-zA-Z][a-zA-Z0-9._]*:'
    t.value = t.value[0:-1]
    return t

def t_CBYTES(t):
    'b"([^\n"]|\\\\")*"'
    t.type = "CBYTES"
    t.value = t.value[2:-1]    
    return t

def t_IDENT(t): # must come before ADDR6.
    r'@?[_a-zA-Z]([a-zA-Z0-9._]|::)*(?=([^a-zA-Z0-9._:]|:[^a-zA-Z0-9._]))'

    if t.value in instructions:
        t.type = "INSTRUCTION"
        
    elif t.value in keywords:
        t.type = keywords[t.value]

    elif t.value in types:
        t.type = t.value.upper()
        
    else:
        # Real IDENT.
        if t.value.startswith("__"):
            pass
            # error(t, "Identifiers starting with '__' are reserved for internal use")
        
    return t

def t_ATTRIBUTE(t): 
    r'\&[_a-zA-Z][a-zA-Z0-9._]*'
    
    if t.value == "&default":
        t.type = "ATTR_DEFAULT"
        
    elif t.value == "&nosub":
        t.type = "ATTR_NOSUB"
        
    else:
        util.parser_error(None, "unknown attribute %s" % t.value)
        
    return t

def t_CADDR6(t): # must come before DOUBLE.
    # FIXME: This is quick first shot at a regexp. Seems a correct one isn't
    # easy to build and v6b addresses might actually look like enum constants as
    # well ...
    r'(([.:0-9a-z]+::?[.0-9a-z]+)|:[.:0-9a-z]+)(/\d{1,3})?'

    try:
        (mask, len) = t.value.split("/")
    except ValueError:
        mask = t.value
        len = None
    
    try:
        addr = struct.unpack("!2Q", socket.inet_pton(socket.AF_INET6, mask))
    except socket.error:
        util.parser_error(None, "cannot parse IPv6 address %s" % mask, lineno=t.lexer.lineno)
        return None

    if not len:
        t.type = "CADDRESS"
        t.value = (addr[0], addr[1])
    else:
        len = int(len)
        if len < 0 or len > 128:
            util.parser_error(None, "prefix length is out of range", lineno=t.lexer.lineno)
        
        t.type = "CNET"
        t.value = (addr[0], addr[1], len)
        
    return t

def t_CREGEXP(t):
    #'/([^\n/]|\\/)*/'
    r'/[^\n]*?(?<!\\)/'
    t.value = t.value[1:-1]    
    return t

def t_CPORT(t):
    r'\d+/(tcp|udp)'
    return t

def t_CDOUBLE(t):
    r'-?\d+\.\d+'
    try:
        t.value = float(t.value)    
    except ValueError:
        error(t, "cannot parse double %s" % t.value)
        t.value = 0
        
    return t

def t_CINTEGER(t):
    r'-?\d+'
    try:
        t.value = int(t.value)    
    except ValueError:
        error(t, "cannot parse integer %s" % t.value)
        t.value = 0
        
    return t

def t_CSTRING(t):
    '"([^\n"]|\\\\")*"'
    t.value = t.value[1:-1]    
    return t

# Pass on comments on lines of its own. 
def t_COMMENTLINE(t):
    r'^\ *\#.*'
    t.value = t.value[t.value.find("#")+1:].strip()
    return t

# Ignore other comments.
def t_comment(t):
    r'\#.*'
    pass

# Track line numbers in nolines modes.
def t_nolines_newline(t):
    r'\n+'
    t.lexer.lineno += len(t.value)

# Ignore comment lines in nolines modes.
def t_nolines_COMMENTLINE(t):
    r'\#.*'
    t.lexer.lineno += 1
    
def t_INITIAL_newline(t):
    r'\n+'
    t.type = "NL"
    t.lexer.lineno += len(t.value)
    return t

# Ignore white space. 
t_ignore  = ' \t'

# Error handling rule
def t_error(t):
    util.parser_error(None, "cannot parse input '%s...'" % t.value[0:10], lineno=t.lexer.lineno)
    
