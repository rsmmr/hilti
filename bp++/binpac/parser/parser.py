# $Id$
#
# The parser.

import os.path
import warnings
import sys

import ply

import binpac.core.id as id
import binpac.core.type as type
import binpac.core.module as module
import binpac.core.expr as expr
import binpac.core.operators as operators
import binpac.core.location as location
import binpac.support.util as util
import binpac.support.parseutil as parseutil
import binpac.pactypes.unit as unit

# FIXME: Why does this not work?
#import binpac.parser.lexer as lexer
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
    """type_decl : opt_linkage TYPE IDENT '=' type ';'"""
    i = id.ID(p[3], type.TypeDecl(p[5]), id.Role.GLOBAL, linkage=p[1], location=_loc(p, 2))
    p.parser.state.module.addID(i, p[4])

def p_def_opt_linkage(p):
    """opt_linkage : EXPORT
                   | """
    if len(p) == 1:
        p[0] = id.Linkage.LOCAL
    else:
        if p[1] == "export":
            p[0] = id.Linkage.EXPORTED
        else:
            util.internal_error("unexpected state in p_def_opt_linkage")
    
### Simple types.

def p_type(p):
    """type : PACTYPE"""
    p[0] = p[1]
    
### More complex types.
    
# Unit type.

def p_type_unit(p):
    """type : UNIT '{' unit_field_list '}'"""
    p[0] = type.Unit(p[3], location=_loc(p, 1))
    
def p_unit_field_list(p):
    """unit_field_list : unit_field unit_field_list
                       | unit_field"""
    p[0] = [p[1]] + p[2] if len(p) != 2 else [p[1]]
    
def p_unit_field_var(p):
    """unit_field : opt_unit_field_name type ';'"""
    p[0] = unit.Field(p[1], None, p[2], location=_loc(p, 2))
    
def p_unit_field_constant(p):
    """unit_field : opt_unit_field_name CONSTANT ';'"""
    p[0] = unit.Field(p[1], p[2], None, location=_loc(p, 2))
    
def p_opt_unit_field_name(p):
    """opt_unit_field_name : IDENT ':'
                          | """
    p[0] = p[1] if len(p) != 1 else None

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
        error(None, "unexpected end of file")
        
##########        

class BinPACState(parseutil.State):
    """Tracks state during parsing."""
    def __init__(self, filename, import_paths):
        super(BinPACState, self).__init__(filename, import_paths)
        self.module = None

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
    parseutil.initParser(parser, lex, state)
    
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
        return (parser.state.errors(), None, parser)

    return (0, ast, parser)    

def _importFile(parser, filename, location, _parse):
    if not checkImport(filename):
        return False
    
    (errors, ast, subparser) = _parse(fullpath, parser.state.importPaths())
    
    ## Do something with the parsed file here.
    
    return (errors == 0, ast)
    
    
    

        

