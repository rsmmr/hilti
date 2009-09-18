# $Id$
#
# The parser.

import os.path
import warnings
import sys

import ply

from support import *
from support.parseutil import *
from core.operators import *
from core import *

import lexer

tokens = lexer.tokens

########## BinPAC++ Grammar.
       
### Top-level constructs.

def p_module(p):
    """module : MODULE IDENT _instantiate_module ';' module_decl_list"""
    p[0] = p.parser.state.module

def p_instantiate_module(p):
    """_instantiate_module :"""
    p.parser.state.module = module.Module(p[-1], location=_loc(p, -1))
    
def p_module_decl_list(p):
    """module_decl_list :   module_decl module_decl_list
                        |   """
    pass
    
def p_module_decl(p):
    """module_decl : type_decl"""
    pass
    
def p_type_decl(p):
    """type_decl : TYPE IDENT '=' type ';'"""
    i = id.ID(p[2], type.TypeDecl(), id.Role.GLOBAL, location=_loc(p, 1))
    p.parser.state.module.addID(i, p[4])

### Simple types.

def p_type(p):
    """type : INT
            | UINT
            | STRING
            | BYTES
    """
    p[0] = p[1]
    
### More complex types.
    
# Unit type.

def p_type_unit(p):
    """type : UNIT _instantiate_unit '{' unit_item_list '}'"""
    p[0] = p.parser.state.unit
    p.parser.state.unit = None 
    
def p_instantiate_unit(p):
    """_instantiate_unit :"""
    prod = grammar.Sequence([], location=_loc(p, -1))
    p.parser.state.unit = type.Unit(prod, location=_loc(p, -1))

def p_unit_item_list(p):
    """unit_item_list : unit_item unit_item_list
                      | """
    pass
    
def p_unit_item_var(p):
    """unit_item : ATTRIBUTE IDENT ':' type ';'"""
    p.parser.state.unit.addAttribute(id.ID(p[2], p[4], id.Role.LOCAL, location=_loc(p, 1)))
    
def p_unit_item_production(p):
    """unit_item : '>' production ';'"""
    p.parser.state.unit.startProduction().addProduction(p[2])

def p_production(p):
    """production : production_name_opt production_body production_cond_opt"""
    if p[1]:
        p[2].setName(p[1])
    
    if p[3]:
        p[2].setPredicate(p[3])

    p[0] = p[2]
        
def p_production_cond_opt(p):
    """production_cond_opt : IF expr
                           | """
    p[0] = p[2] if len(p) != 1 else None

def p_production_name_opt(p):
    """production_name_opt : IDENT ':'
                          | """
    p[0] = p[1] if len(p) != 1 else ""
    
def p_production_body_type(p):
    """production_body : type"""
    p[0] = grammar.Variable(p[1], location=_loc(p, 1))
    
def p_production_body_literal(p):
    """production_body : CONSTANT"""
    p[0] = grammar.Literal(p[1], location=_loc(p, 1))

def p_production_body_alternative(p):
    """production_body : '(' production_alternative_list ')'"""
    p[0] = grammar.Alternative(p[2], location=_loc(p, 1))

def p_production_alternative_list(p):
    """production_alternative_list : production '|' production_alternative_list
                                   | production """
    p[0] = [p[1]] + p[3] if len(p) != 2 else [p[1]]
    
# Support rules for types.

#def p_type_list(p):    
#    """type_list : type "," type
#                 | type """
#    p[0] = [p[1]] + p[3] if len(p) != 2 else [p[1]]

### Expressions

def p_expr_constant(p):
    """expr : CONSTANT"""
    p[0] = expr.Constant(p[1], location=_loc(p, 1))
    
def p_expr_ident(p):
    """expr : IDENT"""
    p[0] = expr.ID(p[1], location=_loc(p, 1))
    
def p_expr_add(p):
    """expr : expr '+' expr"""
    p[0] = expr.OverloadedBinary(Operator.Plus, (p[1], p[3]), location=_loc(p, 1))
    
def p_expr_sub(p):
    """expr : expr '-' expr"""
    p[0] = expr.OverloadedBinary(Operator.Minus, (p[1], p[3]), location=_loc(p, 1))
    
def p_expr_mult(p):
    """expr : expr '*' expr"""
    p[0] = expr.OverloadedBinary(Operator.Mult, (p[1], p[3]), location=_loc(p, 1))
    
def p_expr_div(p):
    """expr : expr '/' expr"""
    p[0] = expr.OverloadedBinary(Operator.Div, (p[1], p[3]), location=_loc(p, 1))
    
### Error handling.

def p_error(p):    
    if p:
        type = p.type.lower()
        value = p.value
        if type == value:
            error(p, "unexpected %s" % type, lineno=p.lineno)
        else:
            error(p, "unexpected %s '%s'" % (type, value), lineno=p.lineno)
    else:
        error(p, "unexpected end of file")
    
##########        

class BinPACState(State):
    """Tracks state during parsing."""
    def __init__(self, filename, import_paths):
        super(BinPACState, self).__init__(filename, import_paths)
        self.module = None
        self.unit = None

def _loc(p, num):
    assert p.parser.state._filename
    
    try:
        if p[num].location():
            return p[num].location()
    except AttributeError:
        pass
    
    return location.Location(p.parser.state._filename, p.lineno(num))
        
# Same arguments as parser.parse().
def _parse(filename, import_paths=["."]):
    state = BinPACState(filename, import_paths)
    lex = ply.lex.lex(debug=0, module=lexer)
    parser = ply.yacc.yacc(debug=0, write_tables=0)
    initParser(parser, lex, state)
    
    filename = os.path.expanduser(filename)
    
    try:
        lines = open(filename).read()
    except IOError, e:
        util.error("cannot open file %s: %s" % (filename, e))

    try:
        ast = parser.parse(lines, lexer=lex, debug=0)
    except ply.lex.LexError, e:
        # Already reported.
        ast = None

    if parser.state.errors() > 0:
        return (parser.state.errors, None, parser)
        
    return (0, ast, parser)    

def _importFile(parser, filename, location, _parse):
    if not checkImport(filename):
        return False
    
    (errors, ast, subparser) = _parse(fullpath, parser.state.importPaths())
    
    ## Do something with the parsed file here.
    
    return (errors == 0, ast)
    
    
    

        

