# $Id$
#
# The lexer. 

import ply.lex

from hilti.core import *

import parser

# All the instructions we support.
instructions = instruction.getInstructions().keys()

# All the data types we support.
types = type.getHiltiTypeNames()

# Keywords in addition to instructions and types.
keywords = {
	"import": "IMPORT",
	"declare": "DECLARE",
	"local": "LOCAL", 
    "global" : "GLOBAL", 
    "module": "MODULE",
    "struct": "STRUCT", 
    "void": "VOID", 
    }

tokens = (
   'LOCAL',
   'GLOBAL',
   'DECLARE',
   'IDENT', 
   'IMPORT', 
   'INSTRUCTION',
   'LABEL',
   'MODULE',
   'NL',
   'NUMBER',
   'BOOL',
   'STRING',
   'STRUCT',
   'TYPE',
   'VOID',
) 

literals = ['(',')','{','}', '<', '>', '=', ',', ':' ]

states = (
   # In nolines mode, newlines are ignored. Per default, they are passed on as token NL.
   ('nolines', 'inclusive'),
)

def t_NUMBER(t):
    r'-?\d+'
    try:
        t.value = int(t.value)    
    except ValueError:
        error(t, "cannot parse number %s" % t.value)
        t.value = 0
        
    return t

def t_TRUE(t):
    r'True'
    t.type = "BOOL"
    t.value = True
    return t

def t_FALSE(t):
    r'False'
    t.type = "BOOL"
    t.value = False
    return t

def t_STRING(t):
    '"([^\n"]|\\\\")*"'
    t.value = t.value[1:-1]    
    return t

def t_LABEL(t):
    r'@[_a-zA-Z][a-zA-Z0-9._]*:'
    t.value = t.value[0:-1]
    return t

def t_IDENT(t):
    r'@?[_a-zA-Z]([a-zA-Z0-9._]|::)*'

    if t.value in instructions:
        t.type = "INSTRUCTION"
        
    elif t.value in types:
        t.type = "TYPE"
        
    elif t.value in keywords:
        t.type = keywords[t.value]
        
    else:
        # Real IDENT.
        if t.value.startswith("__"):
            pass
            # error(t, "Identifiers starting with '__' are reserved for internal use")
        
    return t

# Track line numbers.
def t_nolines_newline(t):
    r'\n+'
    t.lexer.lineno += len(t.value)

def t_INITIAL_newline(t):
    r'\n+'
    t.type = "NL"
    t.lexer.lineno += len(t.value)
    return t

# Ignore comments.
def t_comment(t):
    r'\#.*'
    pass
    
# Ignore white space. 
t_ignore  = ' \t'

# Error handling rule
def t_error(t):
    parser.error(None, "cannot parse input '%s...'" % t.value[0:10], lineno=t.lexer.lineno)
    
# Build the lexer
def buildLexer():
    return ply.lex.lex()
