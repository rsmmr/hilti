# $Id$
#
# The parser.

builtin_id = id

import os.path
import warnings
import sys
import copy

import ply

import binpac.id as id
import binpac.type as type
import binpac.module as module
import binpac.expr as expr
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
    p.parser.state.blocks = [None]
    p.parser.state.units = [None]
    p.parser.state.typename = None
    p.parser.state.in_unit_param_list = False

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

def p_module_global_hook(p):
    """module_global : opt_debug ON hook_ident stmt_block"""
    p.parser.state.module.addExternalHook(None, p[3], p[4], p[1])

def p_module_global_import(p):
    """module_global : IMPORT MODULE_IDENT ';'"""
    if not _importFile(p.parser, p[2], _loc(p, 1)):
        raise SyntaxError

def p_var_init(p):
    """
    var_init : IDENT ':' type
             | IDENT '=' expr
             | IDENT ':' type '=' expr
    """
    if isinstance(p[3], type.Type):
        ty = p[3]

    else:
        ty = p[3].type()

    name = p[1]

    if len(p) > 5:
        e = p[5]

    elif p[2] == '=':
        e = p[3]

    else:
        e = None

    p[0] = (name, ty, e)

def p_global_decl(p):
    """global_decl : opt_linkage global_or_const var_init ';'"""
    (name, ty, e) = p[3]
    linkage = p[1]

    if p[2] == "const":
        i = id.Constant(name, ty, e, linkage=linkage, location=_loc(p, 4))
    else:
        i = id.Global(name, ty, e, linkage=linkage, location=_loc(p, 4))

    _currentScope(p).addID(i)

def p_global_or_const(p):
    """global_or_const : GLOBAL
                       | CONST
    """
    p[0] = p[1]

def p_type_decl(p):
    """type_decl : opt_linkage TYPE IDENT _set_typename '=' type ';'"""
    name = p[3]
    type = p[6]

    i = id.Type(name, type, linkage=p[1], location=_loc(p, 2))
    type.setName("%s" % name)

    _currentScope(p).addID(i)

    p.parser.state.typename = None

def p_set_typename(p):
    """_set_typename : """
    p.parser.state.typename = p[-1]

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
    if not p.parser.state.in_unit_param_list:
        p[0] = id.Parameter(p[1], p[3], location=_loc(p, 1))
    else:
        p[0] = id.UnitParameter(p[1], p[3], location=_loc(p, 1))

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

#def p_id_list(p):
#    """id_list : IDENT "," id_list
#               | IDENT"""
#    if len(p) == 2:
#        p[0] = [p[1]]
#    else:
#        p[0] = [p[1]] + p[3]

 # Simple types.

def p_typepac(p):
    """builtin_type : PACTYPE"""
    p[0] = p[1]

 # Container types.

def p_list(p):
    """builtin_type : LIST '<' unit_field_type '>'"""
    ((val, ty), args) = p[3]
    p[0] = type.List(ty, value=val, item_args=args, location=_loc(p, 1))

def p_vector(p):
    """builtin_type : VECTOR '<' unit_field_type '>'"""
    ((val, ty), args) = p[3]
    p[0] = type.Vector(ty, value=val, item_args=args, location=_loc(p, 1))

 # Iterators.
def p_iterator(p):
    """builtin_type : ITER '<' type '>'"""

    if not isinstance(p[3], type.Iterable):
        util.parser_error(p, "cannot iterate over type " % p[1])
        raise SyntaxError

    p[0] = p[3].iterType()

 # Tuples.
def p_tuple(p):
    """builtin_type : TUPLE '<' type_list '>'"""
    p[0] = type.Tuple(p[3])

def p_type_list(p):
    """type_list : type ',' type_list
                 | type"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]

 # More complex types.

   # Bitfield type. The type here must be an uint.
def p_bitfield_type(p):
    """builtin_type : BITFIELD '(' type ')' '{' bitfield_list '}'"""

    ty = p[3]

    if not isinstance(ty, type.UnsignedInteger):
        util.parser_error(p, "base type must be an unsigned integer" % ty)
        raise SyntaxError

    bits = {}
    for (name, lower, upper, attrs) in p[6]:
        bits[name] = (lower[0], upper[0], attrs)

    ty.setBits(bits)

    p[0] = ty

def p_bitfield_list(p):
    """bitfield_list : bitfield bitfield_list
                     | bitfield"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[2]

def p_bitfield(p):
    """bitfield : IDENT ':' CONSTANT DOTDOT CONSTANT opt_type_attr_list ';'
                | IDENT ':' CONSTANT opt_type_attr_list ';'"""

    f = (p[1], p[3], p[5], p[6]) if len(p) == 8 else (p[1], p[3], p[3], p[4])

    if not isinstance(f[1][1], type.Integer) or not isinstance(f[2][1], type.Integer):
        util.parser_error(p, "invalid bit specification")
        raise SyntaxError

    p[0] = f

   # Enum type.
def p_def_enum(p):
    """builtin_type : ENUM '{' enum_label_list '}'"""
    assert p.parser.state.typename
    p[0] = type.Enum(p[3], _currentScope(p), p.parser.state.typename)

def p_enum_label_list(p):
    """enum_label_list : enum_label "," enum_label_list
                       | enum_label"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]

def p_enum_label(p):
    """enum_label : IDENT
                  | IDENT '=' CONSTANT
    """

    if len(p) > 2 and not isinstance(p[3][0], int):
        util.parser_error(p, "enum value must be an integer")
        raise SyntaxError

    p[0] = (p[1], None) if len(p) == 2 else (p[1], p[3][0])

   # Unit type.
def p_type_unit(p):
    """builtin_type : UNIT opt_unit_param_list _instantiate_unit '{' _enter_unit_hook unit_item_list _leave_unit_hook  '}' """
    _currentUnit(p).setLocation(_loc(p, 1))
    p[0] = _currentUnit(p)
    _popUnit(p)

def p_instantiate_unit(p):
    """_instantiate_unit :"""
    unit = type.Unit(p.parser.state.module, args=p[-1])
    unit.setNamespace(p.parser.state.module.name(True))
    _pushUnit(p, unit)
    p.parser.state.in_switch = None

def p_enter_unit_hook(p):
    """_enter_unit_hook : """
    _pushScope(p, _currentUnit(p).scope())

def p_leave_unit_hook(p):
    """_leave_unit_hook : """
    _popScope(p)

def p_unit_param_list(p):
    """opt_unit_param_list : '(' _enter_unit_param_list opt_param_list _leave_unit_param_list ')'
                           | """
    p[0] = p[3] if len(p) > 2 else []

def p_enter_unit_param_list(p):
    """_enter_unit_param_list : """
    p.parser.state.in_unit_param_list = True

def p_leave_unit_param_list(p):
    """_leave_unit_param_list : """
    p.parser.state.in_unit_param_list = False

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
    _currentUnit(p).addField(p[1])

def p_unit_field_property(p):
    """unit_property : PROPERTY '=' expr ';'"""

    expr = p[3]

    try:
        _currentUnit(p).setProperty(p[1], expr)

    except ValueError, e:
        util.parser_error(p, "unknown property %s" % p[1])
        raise SyntaxError

def p_unit_var(p):
    """unit_var : IDENT ':' type ';'
                | IDENT ':' type opt_debug stmt_block
    """
    i = id.Variable(p[1], p[3])
    _currentUnit(p).addVariable(i)

    if p[4] != ';':
        hook = stmt.VarHook(_currentUnit(p), p[1], 0, stmts=p[5], debug=p[4])
        p.parser.state.module.addHook(hook)

def p_unit_hook(p):
    """unit_hook : opt_debug ON hook_ident stmt_block"""
    p.parser.state.module.addExternalHook(_currentUnit(p), p[3], p[4], p[1])

def p_opt_debug(p):
    """opt_debug : DEBUG
                 |
    """
    p[0] = len(p) > 1

def p_opt_hook_ident(p):
    """hook_ident : IDENT
                  | INIT
                  | DONE
    """
    p[0] = p[1]

def _addAttrs(p, t, attrs):
    for attr in attrs:
        t.addAttribute(attr[0], attr[1])

def p_unit_field(p):
    """unit_field : opt_unit_field_name unit_field_type _instantiate_field _enter_unit_field opt_type_attr_list opt_unit_field_cond opt_unit_field_sink _leave_unit_field ';'"""
    _addAttrs(p, p.parser.state.field.type(), p[5])
    p[0] = p.parser.state.field

def p_unit_field_with_hook(p):
    """unit_field : opt_unit_field_name unit_field_type _instantiate_field _enter_unit_field opt_type_attr_list opt_unit_field_cond opt_foreach opt_debug stmt_block _leave_unit_field"""
    if not p[7]:
        if p.parser.state.in_switch:
            hook = stmt.SubFieldHook(_currentUnit(p), p.parser.state.field, 0, stmts=p[9], debug=p[8])
        else:
            hook = stmt.FieldHook(_currentUnit(p), p.parser.state.field, 0, stmts=p[9], debug=p[8])

        p.parser.state.module.addHook(hook)

    else:
        hook = stmt.FieldForEachHook(_currentUnit(p), p.parser.state.field, 0, stmts=p[9], debug=p[8])
        p.parser.state.module.addHook(hook)

    _addAttrs(p, p.parser.state.field.type(), p[5])
    p[0] = p.parser.state.field

def p_opt_foreach(p):
    """opt_foreach : FOREACH
                   |
    """
    p[0] = len(p) > 1

def p_enter_unit_field(p):
    """_enter_unit_field : """
    _pushScope(p, p.parser.state.field.scope())

def p_leave_unit_field(p):
    """_leave_unit_field : """
    _popScope(p)

def p_instantiate_field(p):
    """_instantiate_field : """
    loc = location.Location(p.lexer.parser.state._filename, p.lexer.lineno)

    if not p.parser.state.in_switch:
        p.parser.state.field = unit.Field(p[-2], p[-1][0][0], p[-1][0][1], _currentUnit(p), args=p[-1][1], location=loc)
    else:
        p.parser.state.field = unit.SwitchFieldCase(p[-2], p[-1][0][0], p[-1][0][1], _currentUnit(p), args=p[-1][1], location=loc)

def p_unit_field_type_const(p):
    """unit_field_type : CONSTANT opt_unit_field_params"""
    (val, type) = p[1]
    e = expr.Ctor(val, type)
    p[0] = ((e, p[1].type()), p[2])

def p_unit_field_type_regexp(p):
    """unit_field_type : REGEXP opt_unit_field_params"""
    e = expr.Ctor(p[1], type.RegExp())
    p[0] = ((e, type.RegExp()), p[2])

def p_unit_field_type_ident(p):
    """unit_field_type : IDENT opt_unit_field_params"""
    # We resolve it later.
    val = (None, type.Unknown(p[1], location=_loc(p, 1)))
    p[0] = (val, p[2])

def p_unit_field_type_vector(p):
    """unit_field_type : unit_field_type '[' expr ']'"""
    ((val, ty), args) = p[1]
    length = p[3];
    vty = type.Vector(ty, value=val, item_args=args, length=length, location=_loc(p, 1))
    p[0] = ((None, vty), [])

def p_unit_field_type_builtin_type(p):
    """unit_field_type : builtin_type opt_unit_field_params"""
    p[0] = ((None, p[1]), p[2])

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

def p_opt_unit_field_sink(p):
    """opt_unit_field_sink : ARROW expr
                           | """
    if len(p) > 1:
        p.parser.state.field.setSink(p[2])

def p_unit_switch(p):
    """unit_switch : SWITCH '(' expr ')' '{' _instantiate_switch unit_switch_case_list '}' ';'"""
    _currentUnit(p).addField(p.parser.state.in_switch)
    p.parser.state.in_switch = None

def p_instantiate_switch(p):
    """_instantiate_switch : """
    loc = location.Location(p.lexer.parser.state._filename, p.lexer.lineno)
    p.parser.state.in_switch = unit.SwitchField(p[-3], _currentUnit(p), location=loc)

def p_unit_switch_case_list(p):
    """unit_switch_case_list : unit_switch_case unit_switch_case_list
                             | """
    if len(p) > 1:
        p.parser.state.in_switch.addCase(p[1])

def p_unit_switch_case(p):
    """unit_switch_case : expr_list ARROW unit_field
                        | '*'  ARROW unit_field
    """
    assert isinstance(p[3], unit.SwitchFieldCase)
    expr = p[1] if p[1] != "*" else None
    p[3].setExprs(expr)
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
    (val, type) = p[1]
    p[0] = expr.Ctor(val, type, location=_loc(p, 1))

def p_ctor_bytes(p):
    """ctor : BYTES"""
    p[0] = expr.Ctor(p[1], type.Bytes(), location=_loc(p, 1))

def p_ctor_regexp(p):
    """ctor : REGEXP"""
    p[0] = expr.Ctor(p[1], type.RegExp(), location=_loc(p, 1))

def p_ctor_list(p):
    """ctor : '[' opt_initializer_list ']'
            | LIST '<' type '>' '(' ')'"""
    if len(p) > 4:
        val = []
        ty = p[3]
    else:
        if not len(p[2]):
            util.parser_error(p, "list ctors cannot be empty (use list<T>() instead)")
            raise SyntaxError
        val = p[2]
        ty = p[2][0]

    p[0] = expr.Ctor(val, type.List(ty))

def p_ctor_vector(p):
    """ctor : VECTOR '(' opt_initializer_list ')'
            | VECTOR '<' type '>' '(' opt_initializer_list ')'"""
    if len(p) > 5:
        val = p[6]
        ty = p[3]
    else:
        if not len(p[2]):
            util.parser_error(p, "vector ctors cannot be empty (use vector<T>() instead)")
            raise SyntaxError
        val = p[3]
        ty = p[3][0]

    p[0] = expr.Ctor(val, type.Vector(ty))

def p_opt_initializer_list(p):
    """opt_initializer_list : initializer_list
                     | """
    p[0] = p[1] if len(p) > 1 else []

def p_initializer_list(p):
    """initializer_list : ctor_or_expr ',' initializer_list
                 | ctor_or_expr """
    p[0] = [p[1]] + p[3] if len(p) > 2 else [p[1]]

def p_ctor_or_expr(p):
    """ctor_or_expr : ctor
                    | expr"""
    p[0] = p[1]

def p_ctor_tuple(p):
    """ctor : '(' ctor ',' initializer_list ')'"""
    ctors = [p[2]] + p[4]
    types = [c.type() for c in ctors]
    p[0] = expr.Ctor(ctors, type.Tuple(types))

### Expressions

precedence = (
    ('left', '[', ']'),
    ('left', '(', ')'),
    ('left', 'PLUSEQUAL'),
    ('left', 'LOGICAL_AND', 'LOGICAL_OR'),
    ('left', 'EQUAL', 'UNEQUAL', 'LEQ', 'GEQ', '<', '>'),
    ('left', '+', '-'),
    ('left', '*', '/', "MOD"),
    ('left', 'SHIFT_LEFT', 'SHIFT_RIGHT'),
    ('left', '&', '|', '^'),
    ('left', '!'),
    ('left', '.', 'HASATTR'),
    ('right', 'DEREF', 'PLUSPLUS_PREFIX', 'MINUSMINUS_PREFIX'),
    ('left', 'PLUSPLUS' ),
    ('right', 'ELSE'),  # Force priority of ELSE in IF..ELSE rule. 
)

def p_expr_function_call(p):
    """expr : expr '(' opt_expr_list ')'"""
    p[0] = expr.Overloaded(Operator.Call, (p[1], p[3]), location=_loc(p, 1))

def p_expr_index(p):
    """expr : expr '[' expr ']'"""
    p[0] = expr.Overloaded(Operator.Index, (p[1], p[3]), location=_loc(p, 1))

def p_expr_logical_and(p):
    """expr : expr LOGICAL_AND expr"""
    p[0] = expr.Overloaded(Operator.LogicalAnd, (p[1], p[3]), location=_loc(p, 1))

def p_expr_logical_or(p):
    """expr : expr LOGICAL_OR expr"""
    p[0] = expr.Overloaded(Operator.LogicalOr, (p[1], p[3]), location=_loc(p, 1))

def p_expr_bitwise_and(p):
    """expr : expr '&' expr"""
    p[0] = expr.Overloaded(Operator.BitAnd, (p[1], p[3]), location=_loc(p, 1))

def p_expr_bitwise_or(p):
    """expr : expr '|' expr"""
    p[0] = expr.Overloaded(Operator.BitOr, (p[1], p[3]), location=_loc(p, 1))

def p_expr_bitwise_xor(p):
    """expr : expr '^' expr"""
    p[0] = expr.Overloaded(Operator.BitXor, (p[1], p[3]), location=_loc(p, 1))

def p_expr_shift_left(p):
    """expr : expr SHIFT_LEFT expr"""
    p[0] = expr.Overloaded(Operator.ShiftLeft, (p[1], p[3]), location=_loc(p, 1))

def p_expr_shift_right(p):
    """expr : expr SHIFT_RIGHT expr"""
    p[0] = expr.Overloaded(Operator.ShiftRight, (p[1], p[3]), location=_loc(p, 1))

def p_expr_power(p):
    """expr : expr POW expr"""
    p[0] = expr.Overloaded(Operator.Power, (p[1], p[3]), location=_loc(p, 1))

#def p_expr_constant(p):
#    """expr : CONSTANT"""
#    (val, type) = p[1]
#   p[0] = expr.Ctor(val, type, location=_loc(p, 1))

def p_expr_ctor(p):
    """expr : ctor"""
    p[0] = p[1]

def p_expr_cast(p):
    """expr : CAST '<' type '>' '(' expr ')'"""
    p[0] = expr.Cast(p[6], p[3], location=_loc(p, 1))

def p_expr_attribute(p):
    """expr : expr '.' IDENT"""
    attr = expr.Attribute(p[3])
    p[0] = expr.Overloaded(Operator.Attribute, (p[1], attr), location=_loc(p, 1))

def p_expr_has_attribute(p):
    """expr : expr HASATTR IDENT"""
    attr = expr.Attribute(p[3])
    p[0] = expr.Overloaded(Operator.HasAttribute, (p[1], attr), location=_loc(p, 1))

#def p_expr_type(p):
#    """expr : type"""
#    p[0] = expr.Type(p[1], _currentScope(p), location=_loc(p, 1))

def p_expr_name(p):
    """expr : IDENT"""
    p[0] = expr.Name(p[1], _currentScope(p), location=_loc(p, 1))

def p_expr_incr_postfix(p):
    """expr : expr PLUSPLUS"""
    p[0] = expr.Overloaded(Operator.IncrPostfix, (p[1], ), location=_loc(p, 1))

def p_expr_incr_prefix(p):
    """expr : PLUSPLUS expr %prec PLUSPLUS_PREFIX"""
    p[0] = expr.Overloaded(Operator.IncrPrefix, (p[2], ), location=_loc(p, 1))

def p_expr_decr_postfix(p):
    """expr : expr MINUSMINUS"""
    p[0] = expr.Overloaded(Operator.DecrPostfix, (p[1], ), location=_loc(p, 1))

def p_expr_decr_prefix(p):
    """expr : MINUSMINUS expr %prec MINUSMINUS_PREFIX"""
    p[0] = expr.Overloaded(Operator.DecrPrefix, (p[2], ), location=_loc(p, 1))

def p_expr_deref(p):
    """expr : '*' expr %prec DEREF"""
    p[0] = expr.Overloaded(Operator.Deref, (p[2], ), location=_loc(p, 1))

def p_expr_not(p):
    """expr : '!' expr"""
    p[0] = expr.Not(p[2], location=_loc(p, 1))

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

def p_expr_mod(p):
    """expr : expr MOD expr"""
    p[0] = expr.Overloaded(Operator.Mod, (p[1], p[3]), location=_loc(p, 1))

def p_expr_equal(p):
    """expr : expr EQUAL expr"""
    p[0] = expr.Overloaded(Operator.Equal, (p[1], p[3]), location=_loc(p, 1))

def p_expr_unequal(p):
    """expr : expr UNEQUAL expr"""
    eq = expr.Overloaded(Operator.Equal, (p[1], p[3]), location=_loc(p, 1))
    p[0] = expr.Not(eq, location=_loc(p, 1))

def p_expr_lequal(p):
    """expr : expr LEQ expr"""
    gt = expr.Overloaded(Operator.Greater, (p[1], p[3]), location=_loc(p, 1))
    p[0] = expr.Not(gt, location=_loc(p, 1))

def p_expr_gequal(p):
    """expr : expr GEQ expr"""
    lt = expr.Overloaded(Operator.Lower, (p[1], p[3]), location=_loc(p, 1))
    p[0] = expr.Not(lt, location=_loc(p, 1))

def p_expr_lower(p):
    """expr : expr '<' expr"""
    p[0] = expr.Overloaded(Operator.Lower, (p[1], p[3]), location=_loc(p, 1))

def p_expr_greater(p):
    """expr : expr '>' expr"""
    p[0] = expr.Overloaded(Operator.Greater, (p[1], p[3]), location=_loc(p, 1))

def p_expr_add_assign(p):
    """expr : expr PLUSEQUAL expr"""
    p[0] = expr.Overloaded(Operator.PlusAssign, (p[1], p[3]), location=_loc(p, 1))

def p_expr_index_assign(p):
    """expr : expr '[' expr ']' '=' expr"""
    p[0] = expr.Overloaded(Operator.IndexAssign, (p[1], p[3]. p[6]), location=_loc(p, 1))

def p_expr_method_call(p):
    """expr : expr '.' IDENT '(' opt_expr_list ')'"""
    attr = expr.Attribute(p[3])
    p[0] = expr.Overloaded(Operator.MethodCall, (p[1], attr, p[5]), location=_loc(p, 1))

def p_expr_size(p):
    """expr : '|' expr '|'"""
    p[0] = expr.Overloaded(Operator.Size, (p[2], ), location=_loc(p, 1))

def p_expr_paren(p):
    """expr : '(' expr ')'"""
    p[0] = p[2]

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

def p_expr_new(p):
    """expr : NEW IDENT
            | NEW IDENT '(' expr_list ')'
    """
    ident = expr.Name(p[2], _currentScope(p))
    p[0] = expr.Overloaded(Operator.New, (ident, p[4] if len(p) > 5 else []), location=_loc(p, 1))

### Statement blocks.

def p_stmt_block(p):
    """stmt_block : '{' _instantiate_block opt_local_var_list stmt_list '}'"""

    b = _popBlock(p)

    for local in p[3]:
        b.scope().addID(local)

    p[0] = b

def p_instantiate_block(p):
    """_instantiate_block :"""
    _pushBlock(p, stmt.Block(_currentScope(p), location=_loc(p, -1)))

def p_opt_local_var_list(p):
    """opt_local_var_list : local_var opt_local_var_list
                          |
    """
    p[0] = [p[1]] + p[2] if len(p) > 1 else []

def p_local_var(p):
    """local_var : LOCAL var_init ';'"""
    (name, ty, e) = p[2]
    p[0] = id.Local(name, ty, e, location=_loc(p,1))

### Statements

def p_stmt_expr(p):
    """stmt : expr ';'"""
    p[0] = stmt.Expression(p[1], location=_loc(p,1))

def p_stmt_stmt_block(p):
    """stmt : stmt_block"""
    p[0] = p[1]

def p_stmt_print(p):
    """stmt : PRINT expr_list ';'"""
    p[0] = stmt.Print(p[2], location=_loc(p,1))

def p_stmt_list(p):
    """stmt_list : stmt_list stmt
                 | """
    if len(p) > 1:
        _currentBlock(p).addStatement(p[2])

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

def _currentBlock(p):
    b = p.parser.state.blocks[-1]
    assert b
    return b

def _pushBlock(p, b):
    p.parser.state.blocks += [b]
    _pushScope(p, b.scope())

def _popBlock(p):
    b = _currentBlock(p)
    p.parser.state.blocks = p.parser.state.blocks[:-1]
    _popScope(p)
    return b

def _currentUnit(p):
    unit = p.parser.state.units[-1]
    assert unit
    return unit

def _pushUnit(p, unit):
    p.parser.state.units += [unit]

def _popUnit(p):
    p.parser.state.units = p.parser.state.units[:-1]

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
        lines = ""
        for line in open(filename):
            if line.startswith("#") and line.find("@TEST-START-") >= 0:
                print >>sys.stderr, "warning: truncating input at @TEST-START-* marker"
                break

            lines += line

    except IOError, e:
        util.parser_error(None, "cannot open input file: %s" % filename)
        return (1, None, None)

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

    return (errors, ast, parser)

def _importFile(parser, filename, location, internal_module=False):
    fullpath = util.checkImport(parser, filename, "pac2", location)

    if not fullpath:
        # Already imported.
        return True

    (errors, ast, subparser) = _parse(fullpath, parser.state.importPaths())

    if errors > 0:
        parser.state.increaseErrors(subparser.state.errors())
        return False

    parser.state.module.addImport(subparser.state.module, fullpath)

    return True
