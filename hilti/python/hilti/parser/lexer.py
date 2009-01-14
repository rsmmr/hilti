# $Id$
#
# The lexer. 

import instruction
import ins
import util
import type

import ply.lex

instructions = instruction.getInstructions().keys()
types = type.typenames()
keywords = {"local": "LOCAL", "global" : "GLOBAL", "struct": "STRUCT", "module": "MODULE"}

tokens = (
   'LOCAL',
   'GLOBAL',
   'STRUCT',
   'MODULE',
   
   'NUMBER',
   'STRING',
   'IDENT', 
   'LABEL',
   
   'INSTRUCTION',
   'TYPE',
   
   'NL',
) 

literals = ['(',')','{','}', '=', ',', ':' ]

states = (
   # In nolines mode, newlines are ignored. Per default, they are passed on as token NL.
   ('nolines', 'inclusive'),
)

def t_NUMBER(t):
    r'\d+'
    try:
        t.value = int(t.value)    
    except ValueError:
        error(t, "cannot parse number %s" % t.value)
        t.value = 0
        
    return t

def t_STRING(t):
    r'"[^"]*"'
    t.value = t.value[1:-1]    
    return t

def t_LABEL(t):
    r'@[_a-zA-Z][a-zA-Z0-9._]*:'
    t.value = t.value[0:-1]
    return t

def t_IDENT(t):
    r'[_a-zA-Z][a-zA-Z0-9._]*'

    if t.value in instructions:
        t.type = "INSTRUCTION"
        
    elif t.value in types:
        t.type = "TYPE"
        t.value = type.type(t.value)
        
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
    error(t, "cannot parse input '%s...'" % t.value[0:10], fatal=True)
    
def error(t, msg, fatal = False):
    t.lexer.errors += 1
    util.error(msg, fatal=fatal)
    
# Build the lexer
lexer = ply.lex.lex()
lexer.errors = 0
