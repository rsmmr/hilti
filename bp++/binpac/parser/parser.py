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
import binpac.core.constant as constant
import binpac.core.operator as operator
import binpac.core.location as location
import binpac.core.stmt as stmt
import binpac.support.util as util
import binpac.support.parseutil as parseutil
import binpac.pactypes.unit as unit

# FIXME: Why does this not work?
#import binpac.parser.lexer as lexer
import lexer

tokens = lexer.tokens
Operator = operator.Operator

########## BinPAC++ Grammar.
       
### Top-level constructs.

def p_module(p):
    """module : MODULE IDENT _instantiate_module ';' module_global_list"""
    p[0] = p.parser.state.module
    _popScope(p)
    
def p_instantiate_module(p):
    """_instantiate_module :"""
    p.parser.state.module = module.Module(p[-1], location=_loc(p, -1))
    p.parser.state.scopes = [None]
    _pushScope(p, p.parser.state.module.scope())
    
def p_module_global_list(p):
    """module_global_list :   module_global module_global_list
                          |   """
    pass
    
def p_module_global_type(p):
    """module_global : type_decl"""
    pass

def p_module_global_global(p):
    """module_global : global_decl"""
    pass

def p_module_global_stmt(p):
    """module_global : stmt"""
    p.parser.state.module.addStatement(p[1])

def p_global_decl(p):
    """global_decl : opt_linkage GLOBAL IDENT ':' type ';'
                   | opt_linkage GLOBAL IDENT '=' expr ';'
                   | opt_linkage GLOBAL IDENT ':' type '=' expr ';'
    """

    if isinstance(p[5], type.Type):
        ty = p[5]
        
    else:
        ty = p[5].type() 
    
    linkage = p[1]
    name = p[3]
    value = p[7] if len(p) > 7 else None
    
    i = id.Global(name, ty, value, linkage=linkage, location=_loc(p, 2))
    _currentScope(p).addID(i)
    
def p_type_decl(p):
    """type_decl : opt_linkage TYPE IDENT '=' type ';'"""
    i = id.Type(p[3], p[5], linkage=p[1], location=_loc(p, 2))
    _currentScope(p).addID(i)

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

### Container types.

def p_list(p):
    """type : LIST '<' type '>'"""
    p[0] = type.List(p[3], location=_loc(p, 1))
    
### More complex types.

# Unit type.

def p_type_unit(p):
    """type : UNIT _instantiate_unit '{' unit_field_list '}'"""
    p[0] = p.parser.state.unit
    
def p_instantiate_unit(p):
    """_instantiate_unit :"""
    p.parser.state.unit = type.Unit(location=_loc(p, -1))

def p_unit_field_list(p):
    """unit_field_list : unit_field unit_field_list
                       | unit_field"""
    pass
    
def p_unit_field(p):
    """unit_field : opt_unit_field_name unit_field_type _instantiate_field ';'"""
    p.parser.state.unit.addField(p.parser.state.field)

def p_unit_field_with_hook(p):
    """unit_field : opt_unit_field_name unit_field_type _instantiate_field _enter_field_hook stmt_block _leave_field_hook"""
    p.parser.state.field.addHook(p[5], 0)
    p.parser.state.unit.addField(p.parser.state.field)
    
def p_instantiate_field(p):
    """_instantiate_field : """
    p.parser.state.field = unit.Field(p[-2], p[-1][0], p[-1][1], _currentScope(p), p.parser.state.unit, location=_loc(p, -1))
    
def p_enter_field_hook(p):
    """_enter_field_hook : """
    _pushScope(p, p.parser.state.field.scope())
    
def p_leave_field_hook(p):
    """_leave_field_hook : """
    _popScope(p)
    
def p_unit_field_type_const(p):
    """unit_field_type : constant"""
    p[0] = (p[1], p[1].type())
    
def p_unit_field_type_type(p):
    """unit_field_type : type_with_attrs"""
    p[0] = (None, p[1])
    
def p_opt_unit_field_name(p):
    """opt_unit_field_name : IDENT ':'
                          | """
    p[0] = p[1] if len(p) != 1 else None

### Type attributes.

def p_type_with_attrs(p):
    """type_with_attrs : type opt_type_attr_list"""
    for attr in p[2]:
        try:
            p[1].addAttribute(attr[0], attr[1])
        except type.ParseableType.AttributeMismatch, e:
            parseutil.error(p, "invalid attribute &%s: %s" % (attr[0], e))
            raise SyntaxError    
        
    p[0] = p[1]
        
def p_opt_type_attr_list(p):
    """opt_type_attr_list : attr opt_type_attr_list
                          | """
    p[0] = [p[1]] + p[2] if len(p) > 1 else []

def p_attr(p):
    """attr : ATTRIBUTE '=' expr 
            | ATTRIBUTE
    """
    p[0] = (p[1], p[3]) if len(p) > 2 else (p[1], None)

### Constants

def p_constant_const(p):
    """constant : CONSTANT"""
    p[0] = p[1]
    
def p_constant_list(p):
    """constant : '[' opt_const_list ']'
                | LIST '<' type '>' '(' ')'"""
    if len(p) > 4:
        val = []    
        ty = p[3]
    else:
        if not len(p[2]):
            parseutil.error(p, "list constants cannot be empty (use list<T>() instead)")
            raise SyntaxError    
        val = p[2]
        ty = p[2][0].type()
        
    p[0] = constant.Constant(val, type.List(ty))

def p_const_list(p):
    """const_list : constant ',' const_list
                  | constant """
    p[0] = [p[1]] + p[3] if len(p) > 2 else [p[1]]
    
def p_opt_const_list(p):
    """opt_const_list : const_list
                      | """
    p[0] = p[1] if len(p) > 1 else []
    
### Expressions

def p_expr_constant(p):
    """expr : constant"""
    p[0] = expr.Constant(p[1], location=_loc(p, 1))

def p_expr_attribute(p):
    """expr : expr '.' IDENT"""
    const = constant.Constant(p[3], type.Identifier())
    ident = expr.Constant(const, location=_loc(p, 1))
    p[0] = expr.Overloaded(Operator.Attribute, (p[1], ident), location=_loc(p, 1))
    
def p_expr_name(p):
    """expr : IDENT"""
    p[0] = expr.Name(p[1], _currentScope(p), location=_loc(p, 1))
    
def p_expr_add(p):
    """expr : expr '+' expr"""
    p[0] = expr.Overloaded(Operator.Plus, (p[1], p[3]), location=_loc(p, 1))
    
def p_expr_sub(p):
    """expr : expr '-' expr"""
    p[0] = expr.Overloaded(Operator.Minus, (p[1], p[3]), location=_loc(p, 1))
    
def p_expr_mult(p):
    """expr : expr '*' expr"""
    p[0] = expr.Overloaded(Operator.Mult, (p[1], p[3]), location=_loc(p, 1))
    
def p_expr_div(p):
    """expr : expr '/' expr"""
    p[0] = expr.Overloaded(Operator.Div, (p[1], p[3]), location=_loc(p, 1))

def p_expr_method_call(p):
    """expr : expr '.' IDENT '(' expr_list ')'"""
    p[0] = expr.Overloaded(Operator.MethodCall, (p[1], p[3], p[5]), location=_loc(p, 1))
    
def p_expr_list(p):
    """expr_list : expr ',' expr_list
                 | expr
    """
    p[0] = [p[1]] + p[3] if len(p) > 2 else [p[1]]
    
### Statement blocks.

def p_stmt_block(p):
    """stmt_block : '{' _instantiate_block stmt_list _leave_block '}'"""
    p[0] = p.parser.state.block
    
def p_instantiate_block(p):
    """_instantiate_block :"""
    p.parser.state.block = stmt.Block(_currentScope(p), location=_loc(p, -1))
    _pushScope(p, p.parser.state.block.scope())
    
def p_leave_block(p):
    """_leave_block :"""
    _popScope(p)
    
### Statements

def p_stmt_print(p):
    """stmt : PRINT expr_list ';'"""
    p[0] = stmt.Print(p[2], location=_loc(p,1))
    
def p_stmt_expr(p):
    """stmt : expr ';'"""
    p[0] = stmt.Expression(p[1], location=_loc(p,1))

def p_stmt_list(p):
    """stmt_list : stmt stmt_list
                 | """
    if len(p) > 1:
        p.parser.state.block.addStatement(p[1])

### Scope management.
def _currentScope(p):
    scope = p.parser.state.scopes[-1]
    assert scope
    return scope
    
def _pushScope(p, scope):
    p.parser.state.scopes += [scope]
    
def _popScope(p):
    p.parser.state.scopes = p.parser.state.scopes[:-1]
        
### Error handling.

def p_error(p):    
    if p:
        type = p.type.lower()
        value = p.value
        if type == value:
            parseutil.error(p, "unexpected %s" % type, lineno=p.lineno)
        else:
            parseutil.error(p, "unexpected %s '%s'" % (type, value), lineno=p.lineno)
    else:
        parseutil.error(None, "unexpected end of file")
        
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
        print e
        ast = None

    if parser.state.errors() > 0:
        return (parser.state.errors(), None, parser)

    assert ast
    
    return (0, ast, parser)    

def _importFile(parser, filename, location, _parse):
    if not checkImport(filename):
        return False
    
    (errors, ast, subparser) = _parse(fullpath, parser.state.importPaths())
    
    ## Do something with the parsed file here.
    
    return (errors == 0, ast)
