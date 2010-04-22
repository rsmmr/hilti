# $Id$
#
# The parser.

import os.path
import warnings
import sys

import ply

import binpac.id as id
import binpac.type as type
import binpac.module as module
import binpac.expr as expr
import binpac.constant as constant
import binpac.operator as operator
import binpac.location as location
import binpac.stmt as stmt
import binpac.scope as scope
import binpac.util as util
import binpac.pactypes.unit as unit
import binpac.pactypes.function as function

# FIXME: Why does this not work?
#import binpac.lexer as lexer
import lexer
import resolver

tokens = lexer.tokens
Operator = operator.Operator

def parsePAC(filename, import_paths=["."]):
    """Parses a file into an |ast|.
    
    filename: string - The name of the file to parse.
    import_paths: list of strings - Search paths for ``import``
    statement.
    
    Returns: tuple (int, ~~Node) - The integer is the number of
    errors found
    during parsing. If there were no errors, ~~Node is the root of
    the parsed
    |ast|.
    """
    (errors, ast, p) = _parse(filename, import_paths)
    return (errors, ast)

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

def p_module_global_function(p):
    """module_global : function_decl"""
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

    if p[2] == "const":
        if e:
            if not e.isConst():
                util.parser_error(p, "expression must be constant")
                raise SyntaxError    
        const = e.constant() if e else None
        i = id.Constant(name, const, linkage=linkage, location=_loc(p, 2))
    else:
        i = id.Global(name, ty, e, linkage=linkage, location=_loc(p, 2))
        
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
    p[0] = id.Parameter(p[1], p[3], location=_loc(p, 1))

### Functions

def p_function_decl(p):
    """function_decl : extern_func
                     | func_def"""
    pass

def _getFunc(p, name, ftype):
    i = _currentScope(p).lookupID(name)
    if not i:
        ty = type.OverloadedFunction(ftype.resultType(), location=_loc(p, 1))
        func = function.OverloadedFunction(ty, None, location=_loc(p, 1))
        i = id.Function(name, func)
        func.setID(i)
        _currentScope(p).addID(i)
        
    util.check_class(i, id.Function, "_getFunc")
    return i

def p_extern_func(p):
    """extern_func : EXTERN func_head ';'"""
    (name, ftype) = p[2]
    i = _getFunc(p, name, ftype)

    hfunc = function.HILTIFunction(ftype, name, location=_loc(p, 1))
    i.function().addFunction(hfunc)

def p_func_def(p):
    """func_def : func_head stmt_block"""    
    (name, ftype) = p[1]
    i = _getFunc(p, name, ftype)

    pfunc = function.PacFunction(name, ftype, p[2], location=_loc(p, 1))
    i.function().addFunction(pfunc)
    
def p_func_head(p):
    """func_head : type IDENT '(' opt_param_list ')'"""
    name = p[2]
    ftype = type.Function(p[4], p[1], location=_loc(p, 1))
    p[0] = (name, ftype)
    
### Types 

def p_type(p):
    """type : builtin_type opt_type_attr_list
            | type_ident opt_type_attr_list
    """
    _addAttrs(p, p[1], p[2])
    p[0] = p[1]
    
def p_type_ident(p):
    """type_ident : IDENT"""
    
    i = _currentScope(p).lookupID(p[1])
    if not i:
        # We try to resolve it later.
        p[0] = type.Unknown(p[1], location=_loc(p, 1))
        return
    
    if not isinstance(i, id.Type):
        util.parser_error(p, "%s is not a type " % p[1])
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
    """builtin_type : UNIT opt_unit_param_list _instantiate_unit '{' _enter_unit_hook unit_item_list _leave_unit_hook  '}' """
    p.parser.state.unit.setLocation(_loc(p, 1))
    p[0] = p.parser.state.unit

def p_instantiate_unit(p):
    """_instantiate_unit :"""
    p.parser.state.unit = type.Unit(_currentScope(p), params=p[-1])
    p.parser.state.in_switch = None 

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

def p_unit_item_list(p):
    """unit_item_list :  unit_item unit_item_list
                       | unit_item"""
    pass

def p_unit_item(p):
    """unit_item : VAR unit_var
                 |     unit_field_top_level
                 |     unit_switch
                 |     unit_hook
                 |     unit_property
    """

def p_unit_field_top_level(p):
    """unit_field_top_level : unit_field"""
    p.parser.state.unit.addField(p[1])
    
def p_unit_field_property(p):
    """unit_property : PROPERTY expr ';'"""
    
    expr = p[2]
    if not expr.isConst():
        util.parser_error(p, "expression must be constant")
        raise SyntaxError    
    
    try:
        p.parser.state.unit.setProperty(p[1], expr.constant())
    except ValueError, e:
        util.parser_error(p, "unknown property %s" % p[1])
        raise SyntaxError    
    except TypeError, e:
        util.parser_error(p, "expression must be of type %s" % e)
        raise SyntaxError    

def p_unit_var(p):
    """unit_var : IDENT ':' type ';'
                | IDENT ':' type stmt_block
    """
    i = id.Variable(p[1], p[3])
    p.parser.state.unit.addVariable(i)
    
    if p[4] != ';':
        hook = stmt.UnitHook(p.parser.state.unit, None, 0, stmts=p[4].statements())
        i.addHook(hook)

def p_unit_hook(p):
    """unit_hook : ON IDENT stmt_block"""
    hook = stmt.UnitHook(p.parser.state.unit, None, 0, stmts=p[3].statements())
    p.parser.state.unit.addHook(p[2], hook, 0)
    
def _addAttrs(p, t, attrs):
    for attr in attrs:
        try:
            t.addAttribute(attr[0], attr[1])
        except type.ParseableType.AttributeMismatch, e:
            util.parser_error(p, "invalid attribute &%s: %s" % (attr[0], e))
            raise SyntaxError    

def p_unit_field(p):
    """unit_field : opt_unit_field_name unit_field_type _instantiate_field _enter_unit_field opt_type_attr_list opt_unit_field_cond _leave_unit_field ';'"""
    _addAttrs(p, p.parser.state.field.type(), p[5])
    p[0] = p.parser.state.field

def p_unit_field_with_hook(p):
    """unit_field : opt_unit_field_name unit_field_type _instantiate_field _enter_unit_field opt_type_attr_list opt_unit_field_cond stmt_block _leave_unit_field"""
    hook = stmt.UnitHook(p.parser.state.unit, p.parser.state.field, 0, stmts=p[7].statements())
    _addAttrs(p, p.parser.state.field.type(), p[5])
    
    p.parser.state.field.addHook(hook)
    p[0] = p.parser.state.field

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
    
def p_instantiate_field(p):
    """_instantiate_field : """
    loc = location.Location(p.lexer.parser.state._filename, p.lexer.lineno)
    
    if not p.parser.state.in_switch:
        p.parser.state.field = unit.Field(p[-2], p[-1][0][0], p[-1][0][1], p.parser.state.unit, params=p[-1][1], location=loc)
    else:
        p.parser.state.field = unit.SwitchFieldCase(p[-2], p[-1][0][0], p[-1][0][1], p.parser.state.unit, params=p[-1][1], location=loc)
    
def p_unit_field_type_const(p):
    """unit_field_type : CONSTANT     
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
        util.parser_error(p, "identifier must be type or constant")
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
    
def p_opt_unit_field_cond(p):
    """opt_unit_field_cond : IF '(' expr ')'
                           | """
    if len(p) > 1:
        p.parser.state.field.setCondition(p[3])

def p_unit_switch(p):
    """unit_switch : SWITCH '(' expr ')' '{' _instantiate_switch unit_switch_case_list '}'"""
    p.parser.state.unit.addField(p.parser.state.in_switch)
    p.parser.state.in_switch = None
    
def p_instantiate_switch(p):
    """_instantiate_switch : """
    loc = location.Location(p.lexer.parser.state._filename, p.lexer.lineno)
    p.parser.state.in_switch = unit.SwitchField(p[-3], p.parser.state.unit, location=loc)

def p_unit_switch_case_list(p):
    """unit_switch_case_list : unit_switch_case unit_switch_case_list
                             | """
    if len(p) > 1:
        p.parser.state.in_switch.addCase(p[1])
        
def p_unit_switch_case(p):
    """unit_switch_case : expr ARROW unit_field
                        | '*'  ARROW unit_field
    """
    assert isinstance(p[3], unit.SwitchFieldCase)
    expr = p[1] if p[1] != "*" else None
    p[3].setExpr(expr)
    p[0] = p[3]
    
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

### Ctor expressions.

def p_ctor_constant(p):
    """ctor : CONSTANT"""
    p[0] = expr.Constant(p[1], location=_loc(p, 1))

def p_ctor_bytes(p):
    """ctor : BYTES"""
    p[0] = expr.Ctor(p[1], type.Bytes(), location=_loc(p, 1))
    
def p_ctor_regexp(p):
    """ctor : REGEXP"""
    p[0] = expr.Ctor(p[1], type.RegExp(), location=_loc(p, 1))

def p_ctor_list(p):
    """ctor : '[' opt_ctor_list_list ']'
            | LIST '<' type '>' '(' ')'"""
    if len(p) > 4:
        val = []    
        ty = p[3]
    else:
        if not len(p[2]):
            util.parser_error(p, "list ctors cannot be empty (use list<T>() instead)")
            raise SyntaxError    
        val = p[2]
        ty = p[2][0].type()
        
    p[0] = expr.Ctor(val, type.List(ty))

def p_opt_ctor_list_list(p):
    """opt_ctor_list_list : ctor_list_list
                          | """
    p[0] = p[1] if len(p) > 1 else []
    
def p_ctor_list_list(p):
    """ctor_list_list : ctor ',' ctor_list_list
                      | ctor """
    p[0] = [p[1]] + p[3] if len(p) > 2 else [p[1]]
    
### Expressions

def p_expr_constant(p):
    """expr : CONSTANT"""
    p[0] = expr.Constant(p[1], location=_loc(p, 1))
    
def p_expr_ctor(p):
    """expr : ctor"""
    p[0] = p[1]

def p_expr_attribute(p):
    """expr : expr '.' IDENT"""
    const = constant.Constant(p[3], type.Identifier())
    ident = expr.Constant(const, location=_loc(p, 1))
    # FIXME: Need to unify type of ident here with expr_method_call()
    p[0] = expr.Overloaded(Operator.Attribute, (p[1], ident), location=_loc(p, 1))
    
def p_expr_has_attribute(p):
    """expr : expr HASATTR IDENT"""
    const = constant.Constant(p[3], type.Identifier())
    ident = expr.Constant(const, location=_loc(p, 1))
    # FIXME: Need to unify type of ident here with expr_method_call()
    p[0] = expr.Overloaded(Operator.HasAttribute, (p[1], ident), location=_loc(p, 1))    
    
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

def p_expr_and(p):
    """expr : expr AND expr"""
    p[0] = expr.Overloaded(Operator.And, (p[1], p[3]), location=_loc(p, 1))

def p_expr_function_call(p):
    """expr : expr '(' opt_expr_list ')'"""
    p[0] = expr.Overloaded(Operator.Call, (p[1], p[3]), location=_loc(p, 1))

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

def p_opt_expr(p):
    """opt_expr : expr
                |
    """
    p[0] = p[1] if len(p) > 1 else None
    
### Statement blocks.

def p_stmt_block(p):
    """stmt_block : '{' _instantiate_block opt_local_var_list stmt_list _leave_block '}'"""
    
    for local in p[3]:
        p.parser.state.block.scope().addID(local)
    
    p[0] = p.parser.state.block
    
def p_instantiate_block(p):
    """_instantiate_block :"""
    p.parser.state.block = stmt.Block(_currentScope(p), location=_loc(p, -1))
    _pushScope(p, p.parser.state.block.scope())
    
def p_leave_block(p):
    """_leave_block :"""
    _popScope(p)

def p_opt_local_var_list(p):
    """opt_local_var_list : local_var opt_local_var_list
                          |
    """
    p[0] = [p[1]] + p[2] if len(p) > 1 else []

def p_local_var(p):
    """local_var : LOCAL IDENT ':' type opt_init_expr ';'"""
    p[0] = id.Local(p[2], p[4], p[5], location=_loc(p,1))

def p_opt_init_expr(p):
    """opt_init_expr : '=' expr
                     | 
    """
    p[0] = p[2] if len(p) > 1 else None
    
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

def p_stmt_return(p):
    """stmt : RETURN opt_expr ';'"""
    p[0] = stmt.Return(p[2], location=_loc(p,1))
    
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
            util.parser_error(p, "unexpected %s" % type, lineno=p.lineno)
        else:
            util.parser_error(p, "unexpected %s '%s'" % (type, value), lineno=p.lineno)
    else:
        util.parser_error(None, "unexpected end of file")
        
##########        

class BinPACState(util.State):
    """Tracks state during parsing."""
    def __init__(self, filename, import_paths):
        super(BinPACState, self).__init__(filename, import_paths)
        self.module = None

_loc = util.loc

# Same arguments as parser.parse().
def _parse(filename, import_paths=["."]):
    state = BinPACState(filename, import_paths)
    lex = ply.lex.lex(debug=0, module=lexer)
    parser = ply.yacc.yacc(debug=0, write_tables=0)
    
    util.initParser(parser, lex, state)
    
    filename = os.path.expanduser(filename)
    
    try:
        lines = open(filename).read()
    except IOError, e:
        util.parser_error("cannot open file %s: %s" % (filename, e))

    try:
        ast = parser.parse(lines, lexer=lex, debug=0)
    except ply.lex.LexError, e:
        # Already reported.
        print e
        ast = None

    if parser.state.errors() > 0:
        return (parser.state.errors(), None, parser)

    assert ast

    errors = resolver.Resolver().resolve(ast)
    
    ast.simplify()
    
    return (errors, ast, parser)    

def _importFile(parser, filename, location, _parse):
    fullpath = util.checkImport(parser, filename, "hlt", location)
        
    if not fullpath:
        return False
    
    (errors, ast, subparser) = _parse(fullpath, parser.state.importPaths())
    
    ## Do something with the parsed file here.
    
    return (errors == 0, ast)
