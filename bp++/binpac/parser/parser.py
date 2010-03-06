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
import binpac.core.scope as scope
import binpac.support.util as util
import binpac.support.parseutil as parseutil
import binpac.pactypes.unit as unit

# FIXME: Why does this not work?
#import binpac.parser.lexer as lexer
import lexer
import resolver

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
    """global_decl : opt_linkage global_or_const IDENT ':' type ';'
                   | opt_linkage global_or_const IDENT '=' expr ';'
                   | opt_linkage global_or_const IDENT ':' type '=' expr ';'
    """

    if isinstance(p[5], type.Type):
        ty = p[5]
        
    else:
        ty = p[5].type() 
    
    linkage = p[1]
    name = p[3]

    if len(p) > 7:
        e = p[7]
        
    elif p[4] == '=':
        e = p[5]

    else:
        e = None

    if e:
        if not e.isConst():
            parseutil.error(p, "expression must be constant")
            raise SyntaxError    
        
    const = e.constant() if e else None
    
    if p[2] == "const":
        i = id.Constant(name, const, linkage=linkage, location=_loc(p, 2))
    else:
        i = id.Global(name, ty, const, linkage=linkage, location=_loc(p, 2))
        
    _currentScope(p).addID(i)

def p_global_or_const(p):
    """global_or_const : GLOBAL
                       | CONST
    """
    p[0] = p[1]                   
    
def p_type_decl(p):
    """type_decl : opt_linkage TYPE IDENT '=' type ';'"""
    name = p[3]
    type = p[5]
    
    i = id.Type(name, type, linkage=p[1], location=_loc(p, 2))
    type.setName(name)
    
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

def p_param_list(p):
    """param_list : param ',' opt_param_list
                  | param
    """
    p[0] = [p[1]] + p[3] if len(p) > 2 else [p[1]]
       
def p_opt_param_list(p):
    """opt_param_list : param_list
                      | 
    """
    p[0] = p[1] if len(p) > 1 else []
    
def p_param(p):
    """param : IDENT ':' type"""
    p[0] = id.ID(p[1], p[3], location=_loc(p, 1))

### Types 

def p_type(p):
    """type : builtin_type
            | type_ident
    """
    p[0] = p[1]
    
def p_type_ident(p):
    """type_ident : IDENT"""
    
    i = _currentScope(p).lookupID(p[1])
    if not i:
        # We try to resolve it later.
        p[0] = type.Unknown(p[1], location=_loc(p, 1))
        return
    
    if not isinstance(i, id.Type):
        parseutil.error(p, "%s is not a type " % p[1])
        raise SyntaxError    
        
    p[0] = i.type()
    
 # Simple types.

def p_type_pac(p):
    """builtin_type : PACTYPE"""
    p[0] = p[1]

    
 # Container types.

def p_list(p):
    """builtin_type : LIST '<' type '>'"""
    p[0] = type.List(p[3], location=_loc(p, 1))
    
 # More complex types.

   # Unit type.
def p_type_unit(p):
    """builtin_type : UNIT opt_unit_param_list _instantiate_unit '{' _enter_unit_hook unit_field_list _leave_unit_hook  '}' """
    p.parser.state.unit.setLocation(_loc(p, 1))
    p[0] = p.parser.state.unit

def p_instantiate_unit(p):
    """_instantiate_unit :"""
    p.parser.state.unit = type.Unit(_currentScope(p), params=p[-1])

def p_enter_unit_hook(p):
    """_enter_unit_hook : """
    _pushScope(p, p.parser.state.unit.scope())
    
def p_leave_unit_hook(p):
    """_leave_unit_hook : """
    _popScope(p)
        
def p_unit_param_list(p):
    """opt_unit_param_list : '(' opt_param_list ')'
                           | """
    p[0] = p[2] if len(p) > 2 else []

def p_unit_field_list(p):
    """unit_field_list : unit_field unit_field_list
                       | unit_field"""
    pass

def _addAttrs(p, attrs):
    for attr in attrs:
        try:
            p.parser.state.field.type().addAttribute(attr[0], attr[1])
        except type.ParseableType.AttributeMismatch, e:
            parseutil.error(p, "invalid attribute &%s: %s" % (attr[0], e))
            raise SyntaxError    

def p_unit_field(p):
    """unit_field : opt_unit_field_name unit_field_type _instantiate_field _enter_unit_field opt_type_attr_list _leave_unit_field ';'"""
    _addAttrs(p, p[5])
    p.parser.state.unit.addField(p.parser.state.field)

def p_unit_field_with_hook(p):
    """unit_field : opt_unit_field_name unit_field_type _instantiate_field _enter_unit_field opt_type_attr_list stmt_block _leave_unit_field"""
    hook = stmt.FieldHook(p.parser.state.field, 0, stmts=p[6].statements())
    _addAttrs(p, p[5])
    
    p.parser.state.field.addHook(hook)
    p.parser.state.unit.addField(p.parser.state.field)

def p_enter_unit_field(p):
    """_enter_unit_field : """
    # If the field has a control hook, we push it's scope.
    hook = p.parser.state.field.controlHook()
    if hook:
        _pushScope(p, hook.scope())
    else:
        # Push a dummy scope so that we can pop in any case.
        _pushScope(p, scope.Scope(None, _currentScope(p)))

def p_leave_unit_field(p):
    """_leave_unit_field : """
    _popScope(p)
    
def p_unit_field_property(p):
    """unit_field : PROPERTY expr ';'"""
    
    expr = p[2]
    if not expr.isConst():
        parseutil.error(p, "expression must be constant")
        raise SyntaxError    
    
    try:
        p.parser.state.unit.setProperty(p[1], expr.constant())
    except ValueError, e:
        parseutil.error(p, "unknown property %s" % p[1])
        raise SyntaxError    
    except TypeError, e:
        parseutil.error(p, "expression must be of type %s" % e)
        raise SyntaxError    
    
def p_instantiate_field(p):
    """_instantiate_field : """
    loc = location.Location(p.lexer.parser.state._filename, p.lexer.lineno)
    p.parser.state.field = unit.Field(p[-2], p[-1][0][0], p[-1][0][1], p.parser.state.unit, params=p[-1][1], location=loc)
    
def p_unit_field_type_const(p):
    """unit_field_type : constant     
                       | builtin_type opt_unit_field_params 
                       | IDENT        opt_unit_field_params 
    """
    i = p[1]

    if isinstance(i, str):
        i = _currentScope(p).lookupID(i)
        if not i:
            # We try to resolve it later.
            val = (None, type.Unknown(p[1], location=_loc(p, 1)))
            p[0] = (val, p[2])
            return
    
    if isinstance(i, type.Type):
        val = (None, i)

    elif isinstance(i, constant.Constant):
        val = (i, i.type())
        
    elif isinstance(i, id.Constant):
        const = i.value()
        val = (const, const.type())
    
    elif isinstance(i, id.Type):
        val = (None, i.type())

    else:
        parseutil.error(p, "identifier must be type or constant")
        raise SyntaxError    

    p[0] = (val, p[2] if len(p) > 2 else [])
    
def p_opt_unit_field_params(p):
    """opt_unit_field_params : '(' opt_expr_list ')'
                             | """
    p[0] = p[2] if len(p) > 2  else []

def p_opt_unit_field_name(p):
    """opt_unit_field_name : IDENT ':'
                          | ':'"""
    p[0] = p[1] if len(p) > 2 else None

### Type attributes.

def p_opt_type_attr_list(p):
    """opt_type_attr_list : attr opt_type_attr_list
                          | """
    p[0] = [p[1]] + p[2] if len(p) > 1 else []

def p_attr(p):
    """attr : ATTRIBUTE '=' expr 
            | ATTRIBUTE '(' expr ')'
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
    # FIXME: Need to unify type of ident here with expr_method_call()
    p[0] = expr.Overloaded(Operator.Attribute, (p[1], ident), location=_loc(p, 1))
    
def p_expr_name(p):
    """expr : IDENT"""
    p[0] = expr.Name(p[1], _currentScope(p), location=_loc(p, 1))

def p_expr_not(p):
    """expr : '!' expr"""
    p[0] = expr.Overloaded(Operator.Not, (p[2], ), location=_loc(p, 1))
    
def p_expr_assign(p):
    """expr : expr '=' expr"""
    p[0] = expr.Assign(p[1], p[3], location=_loc(p, 1))
    
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
    
def p_expr_equal(p):
    """expr : expr EQUAL expr"""
    p[0] = expr.Overloaded(Operator.Equal, (p[1], p[3]), location=_loc(p, 1))
    
def p_expr_unequal(p):
    """expr : expr UNEQUAL expr"""
    eq = expr.Overloaded(Operator.Equal, (p[1], p[3]), location=_loc(p, 1))
    p[0] = expr.Overloaded(Operator.Not, (eq, ), location=_loc(p, 1))
    
def p_expr_method_call(p):
    """expr : expr '.' IDENT '(' expr_list ')'"""
    p[0] = expr.Overloaded(Operator.MethodCall, (p[1], p[3], p[5]), location=_loc(p, 1))

def p_expr_size(p):
    """expr : '|' expr '|'"""
    p[0] = expr.Overloaded(Operator.Size, (p[2], ), location=_loc(p, 1))

def p_expr_paren(p):
    """expr : '(' expr ')'"""
    p[0] = p[1]
    
def p_expr_list(p):
    """expr_list : expr ',' expr_list
                 | expr
    """
    p[0] = [p[1]] + p[3] if len(p) > 2 else [p[1]]

def p_opt_expr_list(p):
    """opt_expr_list : expr_list
                     | 
    """
    p[0] = p[1] if len(p) > 1 else []
    
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
    """stmt_list : stmt_list stmt 
                 | """
    if len(p) > 1:
        p.parser.state.block.addStatement(p[2])

def p_stmt_if_else(p):
    """stmt : IF '(' expr ')' stmt
            | IF '(' expr ')' stmt ELSE stmt
    """
    cond = p[3]
    yes = p[5]
    no = p[7] if len(p) > 7 else None
    
    p[0] = stmt.IfElse(cond, yes, no, location=_loc(p,1))
        
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

_loc = parseutil.loc

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

    assert ast

    errors = resolver.Resolver().resolve(ast)
    
    ast.simplify()
    
    return (errors, ast, parser)    

def _importFile(parser, filename, location, _parse):
    fullpath = parseutil.checkImport(parser, filename, "hlt", location)
        
    if not fullpath:
        return False
    
    (errors, ast, subparser) = _parse(fullpath, parser.state.importPaths())
    
    ## Do something with the parsed file here.
    
    return (errors == 0, ast)
