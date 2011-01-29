# $Id$
#
# The lexer.

import socket
import struct

import binpac.type as type
import binpac.location as location
import binpac.util as util
import binpac.expr as expr

# Language keywords. They will be turned into the corresponding all-uppercase
# token.
keywords = ["module", "type", "export", "unit", "print", "list",
            "global", "const", "if", "else", "var", "on", "switch",
            "extern", "local", "return", "foreach", "enum", "bitfield", "iter",
            "tuple", "new", "cast", "mod"
            ]

control_props = ["%debug", "%init", "%done"]

# Keywords for simple types, and what we turn them into.
types = {
    "bytes": type.Bytes,
    "regexp": type.RegExp,
    "string": type.String,
    "bool": type.Bool,
    "void": type.Void,
    "addr": type.Addr,
    "sink": type.Sink,
    }

# Literals.
literals = ['(',')','{','}', '[', ']', '<', '>', '=', ',', ':', '*', ';', '+', '-', '*', '/', '|', '.', '!', '^', '&', '|']

def _loc(t):
    return location.Location(t.lexer.parser.state._filename, t.lexer.lineno)

tokens = [
    "IDENT", "CONSTANT", "BYTES", "REGEXP", "PACTYPE", "ATTRIBUTE", "PROPERTY",
    "EQUAL", "UNEQUAL", "LEQ", "GEQ", "HASATTR", "ARROW", "LOGICAL_AND", "LOGICAL_OR", "PLUSEQUAL",
    "PLUSPLUS", "MINUSMINUS", "DOTDOT", "IMPORT", "MODULE_IDENT", "SHIFT_LEFT", "SHIFT_RIGHT"
    ] + [k.upper() for k in keywords] \
      + [k.upper()[1:] for k in control_props]

states = [('modulename', 'exclusive')]

# Operators with more than one character.

def t_EQUAL(t):
    r'=='
    return t

def t_UNEQUAL(t):
    r'!='
    return t

def t_LEQ(t):
    r'<='
    return t

def t_GEQ(t):
    r'>='
    return t

def t_HASATTR(t):
    r'\?.'
    return t

def t_ARROW(t):
    r'->'
    return t

def t_LOGICAL_AND(t):
    r'&&'
    return t

def t_LOGICAL_OR(t):
    r'\|\|'
    return t

def t_PLUSEQUAL(t):
    r'\+='
    return t

def t_PLUSPLUS(t):
    r'\+\+'
    return t

def t_MINUSMINUS(t):
    r'\-\-'
    return t

def t_DOTDOT(t):
    r'\.\.'
    return t

def t_SHIFT_LEFT(t):
    r'<<'
    return t

def t_SHIFT_RIGHT(t):
    r'>>'
    return t

def t_IMPORT(t):
    r'import'
    t.lexer.begin('modulename')
    return t

# Type keywords not covered by types.

def t_INT(t):
    r'int(8|16|32|64)'
    t.type = "PACTYPE"
    t.value = type.SignedInteger(int(t.value[3:]))
    return t

def t_UINT(t):
    r'uint(8|16|32|64)'
    t.type = "PACTYPE"
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

def t_CTOR_BYTES(t):
    'b"([^\n"]|\\\\")*"'
    t.type = "BYTES"
    t.value = util.expand_escapes(t.value[2:-1], unicode=False)
    return t

def t_CONST_ADDR4(t): # must come before DOUBLE.
    r'\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}(/\d{1,2})?'

    try:
        (mask, len) = t.value.split("/")
    except ValueError:
        mask = t.value
        len = 0

    addr = struct.unpack("!1L", socket.inet_pton(socket.AF_INET, mask))

    if not len:
        t.type = "CONSTANT"
        t.value = ((0, long(addr[0])), type.Addr())

    else:
        t.type = "CONSTANT"
        t.value = ((0, long(addr[0]), 96 + int(len)), XXX)

    return t

def t_CONST_BOOL(t):
    'True|False'
    t.type = "CONSTANT"
    t.value = (t.value == "True", type.Bool())
    return t

# Identifiers.
def t_IDENT(t):
    r'[%&]?[_a-zA-Z]([a-zA-Z0-9_]|::%?)*'

    if t.value.startswith("&"):
        t.value = t.value[1:]
        t.type = "ATTRIBUTE"

    elif t.value.startswith("%"):

        if t.value in control_props:
            token = t.value[1:].upper()
            t.type = token

        else:
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
        t.type = "CONSTANT"
        t.value = ((addr[0], addr[1]), type.Addr())
    else:
        len = int(len)
        if len < 0 or len > 128:
            util.parser_error(None, "prefix length is out of range", lineno=t.lexer.lineno)

        t.type = "CONSTANT"
        t.value = ((addr[0], addr[1], len), XXXX)

    return t

def t_CONST_INT(t):
    r'[+-]\d+'
    t.type = "CONSTANT"

    try:
        t.value = (int(t.value), type.SignedInteger(64))
    except ValueError:
        error(t, "cannot parse signed integer %s" % t.value)
        t.value = (0, type.SignedInteger(64))

    return t

def t_CONST_UINT(t):
    r'\d+'
    t.type = "CONSTANT"

    try:
        t.value = (int(t.value), type.UnsignedInteger(64))
    except ValueError:
        error(t, "cannot parse unsigned integer %s" % t.value)
        t.value = (0, type.UnsignedInteger(64))

    return t

def t_CONST_STRING(t):
    '"([^\n"]|\\\\")*"'
    t.type = "CONSTANT"
    s =  util.expand_escapes(t.value[1:-1], unicode=True)
    t.value = (s, type.String())
    return t

def t_CTOR_REGEXP(t):
    r'/[^\n]*?(?<!\\)/'
    t.type = "REGEXP"
    t.value = t.value[1:-1]
    return t

def t_DOLLARDOLLAR(t):
    r'\$\$'
    t.value = "__dollardollar"
    t.type = "IDENT"
    return t

# Error handling.
def t_error(t):
    util.parser_error(t.lexer, "cannot parse input '%s...'" % t.value[0:10], lineno=t.lexer.lineno)

# Special mode for parsing module names.
def t_modulename_MODULE_IDENT(t):
    r'[_a-zA-Z]([.a-zA-Z0-9_])*'
    t.lexer.begin('INITIAL')
    return t

def t_modulename_error(t):
    util.parser_error(t.lexer, "cannot parse module name '%s...'" % t.value[0:10], lineno=t.lexer.lineno)

t_modulename_ignore  = ' \t'

