# $Id$
#
# The parser.

builtin_id = id

import os.path
import sys

import block
import constant
import function
import id
import instruction
import lexer
import module
import operand
import type
import util

import instructions.foreach
import instructions.trycatch
import instructions.hook as hook

from lexer import tokens

# ply.yacc triggers "DeprecationWarning: the md5 module is deprecated; use hashlib instead"
import warnings
with warnings.catch_warnings():
    warnings.simplefilter("ignore", DeprecationWarning)
    import ply.yacc

def p_module(p):
    """module : _eat_newlines MODULE IDENT _instantiate_module _clear_comment NL module_decl_list"""
    p[0] = p.parser.state.module

def p_clear_comment(p):
    """_clear_comment : """
    p.parser.state.comment = []

def p_eat_newlines(p):
    """_eat_newlines : NL _eat_newlines
                     | comment_line NL _eat_newlines
                     | """
    pass

def p_begin_nolines(p):
    """_begin_nolines : """
    p.lexer.push_state('nolines')

def p_end_nolines(p):
    """_end_nolines : """
    p.lexer.pop_state()

def p_instantiate_module(p):
    """_instantiate_module :"""
    p.parser.state.module = module.Module(p[-1], location=_loc(p, -1))
    p.parser.state.module.setComment(p.parser.state.comment)
    _pushScope(p, p.parser.state.module.scope())

    # Implicitly import the internal libhilti functions.
    if p.parser.state.module.name() != "libhilti" and not p.parser.state.internal_module:
        if not _importFile(p.parser, "libhilti.hlt", _loc(p, -1), internal_module=True):
            raise SyntaxError

def p_module_decl_list(p):
    """module_decl_list :   module_decl _clear_comment module_decl_list
                        |   empty_line _clear_comment module_decl_list
                        |   """
    pass

def p_module_decl(p):
    """module_decl : def_global
                   | def_const
                   | def_type
                   | def_declare
                   | def_import
                   | def_function
                   | def_export
                   | def_hook_function
                   """

    if p[1]:
        p[1].setComment(p.parser.state.comment)

def p_module_decl_error(p):
    """module_decl : error"""
    # Try to resynchronize here.
    pass

def p_def_global(p):
    """def_global : GLOBAL global_id NL
                  | GLOBAL global_id '=' operand NL"""

    op = p[4] if len(p) > 4 else None

    if op and not (isinstance(p[4], operand.Constant) or isinstance(p[4], operand.Ctor) or isinstance(p[4], operand.ID)):
        util.parser_error(p, "value must be an initializer X")
        raise SyntaxError

    (type, name) = p[2]

    if op:
        i = id.Global(name, type, op, location=_loc(p, 1))
    else:
        i = id.Global(name, type, None, location=_loc(p, 1))

    p.parser.state.module.scope().add(i)

    return i

def p_def_const(p):
    """def_const : CONST const_id '=' operand"""

    if not p[4].constant():
        util.parser_error(p, "value must be a constant")
        raise SyntaxError

    (type, name) = p[2]

    if not p[4].canCoerceTo(type):
        util.parser_error(p, "cannot initialize constant of type %s with value of type %s" % (type, p[4].type()))
        raise SyntaxError

    i = id.Constant(name, p[4], location=_loc(p, 2))
    p.parser.state.module.scope().add(i)

    return p[2]

def p_def_local(p):
    """def_local : LOCAL local_id NL"""
    p.parser.state.function.scope().add(p[2])
    return p[2]

def p_def_type(p):
    """def_type : def_struct_decl
                | def_enum_decl
                | def_overlay_decl
                | def_bitset_decl
                | def_exception_decl
                """
    return p[1]

def _addTypeDecl(p, name, t, location):
    i = id.Type(name, t, linkage=id.Linkage.EXPORTED, location=location)
    p.parser.state.module.scope().add(i)
    return i

def p_def_struct(p):
    """def_struct_decl : TYPE IDENT '=' STRUCT _begin_nolines '{' opt_struct_id_list '}' _end_nolines"""
    _addTypeDecl(p, p[2], type.Struct(p[7]), location=_loc(p, 1))

def p_def_enum(p):
    """def_enum_decl : TYPE IDENT '=' ENUM _begin_nolines '{' enum_label_list '}' _end_nolines"""

    labels = {}
    last = 0

    for (ident, value) in p[7]:
        if value == None:
            value = last + 1

        labels[ident] = value
        last = value

    t = type.Enum(labels)
    nid = _addTypeDecl(p, p[2], t, location=_loc(p, 1))

    # Add  the labels to the module's scope.
    for (label, value) in t.labels().items():
        c = constant.Constant(label, t)
        eid = id.Constant(p[2] + "::" + label, operand.Constant(c), linkage=id.Linkage.EXPORTED, location=_loc(p, 1))
        p.parser.state.module.scope().add(eid)

    # Add the Undef label.
    undef = id.Constant(p[2] + "::Undef", operand.Constant(t.undef()), linkage=id.Linkage.EXPORTED, location=_loc(p, 1))
    p.parser.state.module.scope().add(undef)

    return nid

def p_enum_label_list(p):
    """enum_label_list : enum_label "," enum_label_list
                       | enum_label"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]

def p_enum_label(p):
    """enum_label : IDENT
                  | IDENT '=' CINTEGER
    """
    p[0] = (p[1], None) if len(p) == 2 else (p[1], p[3])

def p_def_bitset(p):
    """def_bitset_decl : TYPE IDENT '=' BITSET _begin_nolines '{' id_with_opt_int_init_list '}' _end_nolines"""
    t = type.Bitset(p[7])
    nid = _addTypeDecl(p, p[2], t, location=_loc(p, 1))

    # Register the individual labels.
    for (label, value) in t.labels().items():
        c = constant.Constant(label, t)
        eid = id.Constant(p[2] + "::" + label, operand.Constant(c), linkage=id.Linkage.EXPORTED, location=_loc(p, 1))
        p.parser.state.module.scope().add(eid)

    return nid

def p_def_exception(p):
    """def_exception_decl : TYPE IDENT '=' EXCEPTION '<' opt_type '>' opt_exception_base opt_internal"""
    t = type.Exception(p[6], p[8], location=_loc(p, 1))
    if p[9]:
        t.setInternalName(p[9])

    return _addTypeDecl(p, p[2], t, location=_loc(p, 1))

def p_opt_internal(p):
    """opt_internal : INTERNAL '(' CSTRING ')'
                    | """
    p[0] = p[3] if len(p) > 1 else None

def p_def_exception_no_type(p):
    """def_exception_decl : TYPE IDENT '=' EXCEPTION opt_exception_base"""
    return _addTypeDecl(p, p[2], type.Exception(None, p[5], location=_loc(p, 1)), location=_loc(p, 1))

def p_opt_exception_base(p):
    """opt_exception_base : ':' type
                             |
    """
    p[0] = p[2] if len(p) != 1 else None

def p_def_overlay(p):
    """def_overlay_decl : TYPE IDENT '=' OVERLAY _begin_nolines '{' overlay_field_list '}' _end_nolines"""
    t = type.Overlay()
    for f in p[7]:
        t.addField(f)

    return _addTypeDecl(p, p[2], t, location=_loc(p, 1))

def p_def_overlay_field_list(p):
    """overlay_field_list : overlay_field ',' overlay_field_list
                          | overlay_field"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]

def p_def_overlay_field(p):
    """overlay_field : IDENT ':' type AT CINTEGER INSTRUCTION WITH operand operand
                     | IDENT ':' type AFTER IDENT INSTRUCTION WITH operand operand
    """
    name = p[1]
    t = p[3]
    start = p[5]
    fmt = p[8]
    arg = p[9]

    if p[6] != 'unpack':
        raise SyntaxError

    p[0] = type.Overlay.Field(name, start, t, fmt, arg)

def p_def_import(p):
    """def_import : IMPORT IDENT"""
    if not _importFile(p.parser, p[2], _loc(p, 1)):
        raise SyntaxError

def p_def_declare_func(p):
    """def_declare : DECLARE function_head"""
    p[2].id().setLinkage(id.Linkage.EXPORTED)

def _declare_hook(p, name, args, result, location):
    hook = p.parser.state.module.lookupHook(name)

    if not hook:

        if name.find("::") >= 0:
            # Can't have a namespace if it's not declared.
            util.parser_error(p, "hook was not declared")
            raise SyntaxError

        htype = type.Hook(args, result, location=location)
        hook = id.Hook(name, htype, namespace=p.parser.state.module.name(), location=location)
        p.parser.state.module.addHook(hook)

    return hook

def p_def_declare_hook(p):
    """def_declare : DECLARE HOOK type IDENT '(' param_list ')'"""
    _declare_hook(p, p[4], p[6], p[3], _loc(p, 4))

def p_def_export_ident(p):
    """def_export : EXPORT IDENT"""
    ident = id.Unknown(p[2], _currentScope(p), location=_loc(p, 1))
    p.parser.state.module.exportIdent(ident)

def p_def_export_type(p):
    """def_export : EXPORT type"""
    ident = id.Type("<exported type>", p[2], location=_loc(p, 1))
    p.parser.state.module.exportIdent(ident)

def p_def_function_head(p):
    """function_head : opt_linkage opt_cc type IDENT '(' param_list ')' opt_hook_attrs"""
    ftype = type.Function(p[6], p[3], location=_loc(p, 4))
    namespace = p.parser.state.module.name()
    ident = p[4]

    if p[2] == function.CallingConvention.HILTI:
        if not p.parser.state.in_hook:
            func = function.Function(ftype, None, p.parser.state.module.scope(), location=_loc(p, 4))
        else:
            func = hook.HookFunction(ftype, None, p.parser.state.module.scope(), location=_loc(p, 4))

    elif p[2] in (function.CallingConvention.C, function.CallingConvention.C_HILTI):
        # FIXME: We need some way to declare C function which are not part of
        # a module. In function.Function, we already allow module==None in the
        # CC.C case but our syntax does not provide for that currently.
        func = function.Function(ftype, None, p.parser.state.module.scope(), cc=p[2], location=_loc(p, 4))
    else:
        # Unknown calling convention
        assert False

    for (key, val) in p[8]:
        if not p.parser.state.in_hook:
            util.parser_error(p, "hook attributes not allowed for function")
            raise SyntaxError

        if key == "ATTR_PRIORITY":
            func.setPriority(val)

        elif key == "ATTR_GROUP":
            func.setGroup(val)

        else:
            util.internal_error("unexpected attribute in p_def_opt_hook_attrs: %s" % key)

    i = id.Function(ident, ftype, func, namespace=namespace, location=_loc(p, 4))

    if not p.parser.state.in_hook:
        p.parser.state.module.scope().add(i)

    func.setID(i)
    func.id().setLinkage(p[1])

    p[0] = func

    p.parser.state.function = None

def p_def_opt_cc(p):
    """opt_cc : CSTRING
              | """

    if len(p) == 1:
        p[0] = function.CallingConvention.HILTI
        return

    if p[1] == "HILTI":
        p[0] =  function.CallingConvention.HILTI
        return

    if p[1] == "C":
        p[0] = function.CallingConvention.C
        return

    if p[1] == "C-HILTI":
        p[0] = function.CallingConvention.C_HILTI
        return

    util.parser_error(p, "unknown calling convention \"%s\"" % p[1])
    raise SyntaxError

def p_def_opt_linkage(p):
    """opt_linkage : INIT
                   | """

    if len(p) == 1:
        p[0] = id.Linkage.LOCAL
    else:
        if p[1] == "init":
            p[0] = id.Linkage.INIT
        else:
            util.internal_error("unexpected state in p_def_opt_linkage")

def p_def_opt_hook_attrs(p):
    """opt_hook_attrs : ATTR_PRIORITY '=' CINTEGER opt_hook_attrs
                      | ATTR_GROUP '=' CINTEGER opt_hook_attrs
                      |
    """
    if len(p) == 1:
        p[0] = []
    else:
        p[0] = [(p[1], p[3])] + p[4]

def p_def_function(p):
    """def_function : function_head _begin_nolines '{' _instantiate_function _end_nolines instruction_list _begin_nolines _pop_scope '}' _finish_function _end_nolines """

def p_instantiate_function(p):
    """_instantiate_function :"""
    func = p[-3]
    p.parser.state.function = func

    _pushBlockList(p)
    _pushScope(p, p.parser.state.function.scope())

    b = block.Block(func, location=_loc(p, 0))
    b.setMayRemove(False)
    _addBlock(p, b)

def p_finish_function(p):
    """_finish_function :"""
    blocks = _popBlockList(p)
    for b in blocks:
        p.parser.state.function.addBlock(b)

def p_def_hook_function(p):
    """def_hook_function : HOOK _begin_hook_func def_function _end_hook_func"""
    func = p.parser.state.function
    hook = _declare_hook(p, func.name(), func.type().args(), func.type().resultType(), _loc(p, 0))
    hook.addFunction(func)

def p_begin_hook_func(p):
    """_begin_hook_func :"""
    p.parser.state.in_hook = True

def p_end_hook_func(p):
    """_end_hook_func :"""
    p.parser.state.in_hook = False

def p_pop_scope(p):
    """_pop_scope :"""
    _popScope(p)

def p_instruction_list(p):
    """instruction_list : def_local instruction_list
                        | instruction instruction_list
                        | CLABEL _set_block_name instruction_list
                        | CLABEL _set_block_name NL instruction_list
                        | empty_line instruction_list
                        | """

def p_empty_line(p):
    """empty_line : NL
                  | comment_line NL
    """

def p_empty_lines(p):
    """empty_lines : NL
                  | comment_line NL
                  | empty_lines NL
    """

def p_comment_line(p):
    """comment_line : COMMENTLINE"""
    p.parser.state.comment += [p[1]]

def p_set_block_name(p):
    """_set_block_name : """

    name = p[-1]

    if len(p.parser.state.block.instructions()) or p.parser.state.block._name_set:
        if name and p.parser.state.function.scope().lookup(name):
            util.parser_error(p, "block name %s already defined" % name, lineno=p.lexer.lineno)
            raise SyntaxError

        # Start a new block.
        b = block.Block(p.parser.state.function, name=name, location=_loc(p, 0))
        _addBlock(p, b)

        if name:
            p.parser.state.block._name_set = True

    else:
        # Current block still empty so just set its name.
        if name and p.parser.state.function.scope().lookup(name) and name != p.parser.state.block.name():
            util.parser_error(p, "block name %s already defined" % name, lineno=p.lexer.lineno)
            raise SyntaxError

        p.parser.state.block.setName(name)
        p.parser.state.block._name_set = True

def p_instruction(p):
    """instruction : operand '=' INSTRUCTION operand operand operand NL
                   |             INSTRUCTION operand operand operand NL
    """

    if p[2] == "=":
        name = p[3]
        op1 = p[4]
        op2 = p[5]
        op3 = p[6]
        target = p[1]
        location = _loc(p, 3)
    else:
        name = p[1]
        op1 = p[2]
        op2 = p[3]
        op3 = p[4]
        target = None
        location = _loc(p, 1)

    ins = instruction.createInstruction(name, op1, op2, op3, target, location=location)
    if not ins:
        util.parser_error(p, "unknown instruction %s" % name)
        raise SyntaxError

    p.parser.state.block.addInstruction(ins)

def p_assignment(p):
    # Syntactic sugar for the assignment operator.
    """instruction : operand '=' operand"""
    ins = instruction.createInstruction("assign", p[3], None, None, p[1], location=_loc(p, 1))
    assert ins
    p.parser.state.block.addInstruction(ins)

def p_foreach(p):
    """instruction : FOR IDENT IN operand DO NL _push_blocklist instruction_list DONE NL"""
    blocks = _popBlockList(p)
    ins = instructions.foreach.ForEach(p[2], p[4], blocks, location=_loc(p, 1))
    p.parser.state.block.addInstruction(ins)

def p_trycatch(p):
    """instruction : TRY '{' NL _push_blocklist instruction_list '}' empty_lines catch_list"""
    try_blocks = _popBlockList(p)
    catches = p[8]

    ins = instructions.trycatch.TryCatch(try_blocks, catches, location=_loc(p, 1))
    p.parser.state.block.addInstruction(ins)

def p_catch_list(p):
    """catch_list : catch catch_list
                  | catch
    """
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[2]

def p_catch(p):
    """catch : CATCH opt_type '{' NL _push_blocklist instruction_list '}' empty_lines """
    excpt = p[2] if p[2] else type.Exception.root()
    p[0] = (excpt, _popBlockList(p))

def p_push_blocklist(p):
    """_push_blocklist : """
    _pushBlockList(p)

    b = block.Block(p.parser.state.function, location=_loc(p, 0))
    b.setMayRemove(False)
    _addBlock(p, b)

def p_operand_ident(p):
    """operand : IDENT"""
    ident = id.Unknown(p[1], _currentScope(p), location=_loc(p, 1))
    p[0] = operand.ID(ident, location=_loc(p, 1))

def p_operand_integer(p):
    """operand : CINTEGER"""
    const = constant.Constant(p[1], type.Integer(64), location=_loc(p, 1))
    p[0] = operand.Constant(const, location=_loc(p, 1))

def p_operand_double(p):
    """operand : CDOUBLE"""
    const = constant.Constant(p[1], type.Double(), location=_loc(p, 1))
    p[0] = operand.Constant(const, location=_loc(p, 1))

def p_operand_bool(p):
    """operand : CBOOL"""
    const = constant.Constant(p[1], type.Bool(), location=_loc(p, 1))
    p[0] = operand.Constant(const, location=_loc(p, 1))

def p_operand_null(p):
    """operand : NULL"""
    const = constant.Constant(None, type.Reference(None), location=_loc(p, 1))
    p[0] = operand.Constant(const, location=_loc(p, 1))

def p_operand_string(p):
    """operand : CSTRING"""
    try:
        string = constant.Constant(util.expand_escapes(p[1], unicode=True), type.String(), location=_loc(p, 1))
        p[0] = operand.Constant(string, location=_loc(p, 1))
    except ValueError:
        util.parser_error(p, "error in escape sequence %s" % p[1])
        raise SyntaxError

def p_operand_tuple(p):
    """operand : tuple"""
    types = [op.type() for op in p[1]]
    const = constant.Constant(p[1], type.Tuple(types), location=_loc(p, 1))
    p[0] = operand.Constant(const, location=_loc(p, 1))

def p_operand_addr(p):
    """operand : CADDRESS"""
    const = constant.Constant(p[1], type.Addr(), location=_loc(p, 1))
    p[0] = operand.Constant(const, location=_loc(p, 1))

def p_operand_net(p):
    """operand : CNET"""
    const = constant.Constant(p[1], type.Net(), location=_loc(p, 1))
    p[0] = operand.Constant(const, location=_loc(p, 1))

def p_operand_port(p):
    """operand : CPORT"""
    const = constant.Constant(p[1], type.Port(), location=_loc(p, 1))
    p[0] = operand.Constant(const, location=_loc(p, 1))

def p_operand_regexp(p):
    """operand : regexp_list"""
    p[0] = operand.Ctor(p[1], type.Reference(type.RegExp()), location=_loc(p, 1))

def p_regexp_list(p):
    """regexp_list : CREGEXP '|' regexp_list
                   | CREGEXP"""
    if len(p) == 2:
        patterns = [p[1]]
        attrs = []
        p[0] = (patterns, attrs)
    else:
        patterns = [p[1]] + p[3][0]
        attrs = [] + p[3][1]
        p[0] = (patterns, attrs)

def p_operand_bytes(p):
    """operand : CBYTES"""
    try:
        string = util.expand_escapes(p[1], unicode=False)
        p[0] = operand.Ctor(string, type.Reference(type.Bytes()), location=_loc(p, 1))
    except ValueError:
        util.parser_error(p, "error in escape sequence %s" % p[1])
        raise SyntaxError

def p_operand_type(p):
    """operand : type"""
    p[0] = operand.Type(p[1], location=_loc(p, 1))

def p_operand_ununsed(p):
    """operand : """
    p[0] = None

def p_operand_ctor(p):
    """operand : type '(' opt_operand_list ')'"""

    t = p[1]
    ops = p[3]

    if isinstance(p[1], type.List):
        for op in ops:
            if not op.canCoerceTo(t.itemType()):
                util.parser_error(p, "list element %s is of wrong type" % op)
                raise SyntaxError

        p[0] = operand.Ctor(ops, type.Reference(t), location=_loc(p, 1))
        return

    util.parser_error(p, "type %s does not support constructor expression" % t)
    raise SyntaxError

def p_tuple_empty(p):
    """tuple : '(' ')'"""
    p[0] = []

def p_tuple_non_empty(p):
    """tuple : '(' operand_list ')'"""
    p[0] = p[2]

def p_operand_list(p):
    """operand_list : operand ',' operand_list
                    | operand"""
    if len(p) == 4:
        p[0] = [p[1]] + p[3]
    else:
        p[0] = [p[1]]

def p_opt_operand_list(p):
    """opt_operand_list : operand_list"""
    if len(p[1]) == 1 and p[1][0] == None:
        p[0] = []
    else:
        p[0] = p[1]

def p_param_list(p):
    """param_list : param_id opt_default_val ',' param_list
                  | param_id opt_default_val
                  | """

    if len(p) == 1:
        p[0] = []
    elif len(p) == 3:
        p[0] = [(p[1], p[2])]
    else:
        p[0] = [(p[1], p[2])] + p[4]

def p_param_id(p):
    """param_id : type IDENT"""
    p[0] = id.Parameter(p[2], p[1], location=_loc(p, 2))

def p_local_id(p):
    """local_id : type IDENT
                | type IDENT '=' operand"""

    op = p[4] if len(p) > 4 else None
    p[0] = id.Local(p[2], p[1], op, location=_loc(p, 2))

def p_struct_id(p):
    """struct_id : type IDENT
                 | type IDENT ATTR_DEFAULT '=' operand"""

    if len(p) == 3:
        p[0] = (id.Local(p[2], p[1], location=_loc(p, 2)), None)
    else:
        p[0] = (id.Local(p[2], p[1], location=_loc(p, 2)), p[5])

def p_global_id(p):
    """global_id : type IDENT"""
    p[0] = (p[1], p[2])

def p_const_id(p):
    """const_id : type IDENT"""
    p[0] = (p[1], p[2])

def p_def_opt_default_val(p):
    """opt_default_val : '=' operand
                    | """
    if len(p) == 3:
        p[0] = p[2]
    else:
        p[0] = None

# Builtin types.

def p_type_string(p):
    """type : STRING"""
    p[0] = type.String()

def p_type_bool(p):
    """type : BOOL"""
    p[0] = type.Bool()

def p_type_int_width(p):
    """type : INT '<' CINTEGER '>'"""
    p[0] = type.Integer(p[3], location=_loc(p, 1))

def p_type_int_fixed(p):
    """type : INT8
            | INT16
            | INT32
            | INT64
    """
    p[0] = type.Integer(int(p[1][3:]))

def p_type_void(p):
    """type : VOID"""
    p[0] = type.Void()

def p_type_type(p):
    """type : TYPE"""
    p[0] = type.MetaType()

def p_type_any(p):
    """type : ANY"""
    p[0] = type.Any()

def p_type_int(p):
    """type : BYTES"""
    p[0] = type.Bytes()

def p_type_double(p):
    """type : DOUBLE"""
    p[0] = type.Double()

def p_type_addr(p):
    """type : ADDR"""
    p[0] = type.Addr()

def p_type_port(p):
    """type : PORT"""
    p[0] = type.Port()

def p_type_net(p):
    """type : NET"""
    p[0] = type.Net()

def p_type_caddr(p):
    """type : CADDR"""
    p[0] = type.CAddr()

def p_type_timer(p):
    """type : TIMER"""
    p[0] = type.Timer()

def p_type_timer_mgr(p):
    """type : TIMER_MGR"""
    p[0] = type.TimerMgr()

def p_type_file(p):
    """type : FILE"""
    p[0] = type.File()


#

def p_type_tuple(p):
    """type : TUPLE '<' type_list '>'
            | TUPLE '<' '*' '>'"""
    if p[3] != "*":
        p[0] = type.Tuple(p[3])
    else:
        p[0] = type.Tuple([])

def p_type_ref(p):
    """type : REF '<' type '>'
            | REF '<' '*' '>'"""
    if p[2] != "*":
        p[0] = type.Reference(p[3])
    else:
        p[0] = type.Reference(None)

def p_type_list(p):
    """type : LIST '<' type '>'
            | LIST '<' '*' '>'"""
    if p[2] != "*":
        p[0] = type.List(p[3])
    else:
        p[0] = type.List(None)

def p_type_vector(p):
    """type : VECTOR '<' type '>'
            | VECTOR '<' '*' '>'"""
    if p[2] != "*":
        p[0] = type.Vector(p[3])
    else:
        p[0] = type.Vector(None)

def p_type_map(p):
    """type : MAP '<' type ',' type '>'
            | MAP '<' '*' '>'"""
    if p[3] != "*":
        p[0] = type.Map(p[3], p[5])
    else:
        p[0] = type.Map(None, None)

def p_type_set(p):
    """type : SET '<' type '>'
            | SET '<' '*' '>'"""
    if p[3] != "*":
        p[0] = type.Set(p[3])
    else:
        p[0] = type.Set(None)

def p_type_pktsrc(p):
    """type : IOSRC '<' IDENT '>'
            | IOSRC '<' '*' '>'
    """
    if p[3] != "*":
        p[0] = type.IOSrc(p[3])
    else:
        p[0] = type.IOSrc(None)

def p_type_channel(p):
    """type : CHANNEL '<' type '>'
            | CHANNEL '<' '*' '>'
    """
    if p[3] != "*":
        p[0] = type.Channel(p[3])
    else:
        p[0] = type.Channel(None)

def p_type_iterator(p):
    """type : ITERATOR '<' type '>'
    """
    t = p[3]
    if not isinstance(t, type.Iterable):
        util.parser_error(p, "type %s is not iterable" % t)
        raise SyntaxError

    p[0] = t.iterType()

def p_type_regexp(p):
    """type : REGEXP '<' ATTR_NOSUB '>'
            | REGEXP '<' '>'
            | REGEXP"""
    if len(p) <= 4:
        p[0] = type.RegExp([])
    else:
        p[0] = type.RegExp([p[3]])

def p_type_callable(p):
    """type : CALLABLE '<' type '>'"""
    p[0] = type.Callable(p[3])

def p_type_ident(p):
    """type : IDENT"""
    # Must be a type name.
    i = p.parser.state.module.scope().lookup(p[1])
    if not i:
        # Resolver will look this up later.
        p[0] = type.Unknown(p[1], location=_loc(p, 1))
        return

    if not isinstance(i, id.Type):
        util.parser_error(p, "%s is not a type name" % p[1])
        raise SyntaxError

    p[0] = i.type()

def p_opt_type(p):
    """opt_type : type
                | """
    p[0] = p[1] if len(p) > 1 else None

def p_typelist(p):
    """type_list : type "," type_list
                 | type """
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]

def p_struct_id_list(p):
    """struct_id_list : struct_id "," struct_id_list
                      | struct_id"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]

def p_opt_struct_id_list(p):
    """opt_struct_id_list : struct_id_list
                          | """
    p[0] = p[1] if len(p) == 2 else []

def p_id_list(p):
    """id_list : IDENT "," id_list
                | IDENT"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]

def p_id_with_opt_int_init_list(p):
    """id_with_opt_int_init_list : IDENT opt_int_init "," id_with_opt_int_init_list
                                 | IDENT opt_int_init """
    if len(p) == 3:
        p[0] = [(p[1], p[2])]
    else:
        p[0] = [(p[1], p[2])] + p[4]

def p_opt_id_init(p):
    """opt_int_init : '=' CINTEGER
                    | """

    return p[2] if len(p) == 3 else None

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

###

class HILTIState(util.State):
    """Tracks state during parsing."""
    def __init__(self, filename, import_paths, internal_module):
        super(HILTIState, self).__init__(filename, import_paths)
        # Note that we don't predefine all possible attributes here; we add
        # them as needed during parsing.
        self.module = None
        self.internal_module = internal_module
        self.comment = []
        self.scopes = []
        self.blocklists = []
        self.in_hook = False

_loc = util.loc

def parse(filename, import_paths=["."], internal_module=False):
    """See ~~parse."""
    (errors, ast, parser) = _parse(filename, import_paths, internal_module)
    return (errors, parser.state.module)

def _parse(filename, import_paths=["."], internal_module=False):
    parser = ply.yacc.yacc(optimize=1, debug=0, write_tables=0, picklefile="/tmp/hiltic_yacc_cache.dat")
    lex = ply.lex.lex(optimize=1, debug=0, module=lexer, lextab=None)
    state = HILTIState(filename, import_paths, internal_module)
    util.initParser(parser, lex, state)

    filename = os.path.expanduser(filename)

    lines = ""
    try:
        for line in open(filename):
            if line.startswith("#") and line.find("@TEST-START-NEXT") >= 0:
                print >>sys.stderr, "warning: truncating input at @TEST-START-NEXT marker"
                break

            lines += line
    except IOError, e:
        util.parser_error(None, "cannot open file %s: %s" % (filename, e))
        return (1, None, parser)

    try:
        ast = parser.parse(lines, lexer=lex, debug=0)
    except ply.lex.LexError, e:
        # Already reported.
        ast = None

    return (parser.state.errors(), ast, parser)

def _importFile(parser, filename, location, internal_module=False):
    fullpath = util.checkImport(parser, filename, "hlt", location)

    if not fullpath:
        # Already imported.
        return True

    nhi = internal_module or parser.state.internal_module

    (errors, ast, subparser) = _parse(fullpath, parser.state.importPaths(), internal_module=nhi)

    if errors > 0:
        parser.state.increaseErrors(subparser.state.errors())
        return False

    parser.state.module.importIDs(subparser.state.module)

    return True

def _currentScope(p):
    scope = p.parser.state.scopes[-1]
    assert scope
    return scope

def _pushScope(p, scope):
    p.parser.state.scopes += [scope]

def _popScope(p):
    p.parser.state.scopes = p.parser.state.scopes[:-1]

def _pushBlockList(p):
    p.parser.state.blocklists += [[]]

def _popBlockList(p):
    bl = p.parser.state.blocklists[-1]
    p.parser.state.blocklists = p.parser.state.blocklists[:-1]

    try:
        p.parser.state.block = p.parser.state.blocklists[-1][-1]
    except IndexError:
        p.parser.state.block = None

    return bl

def _addBlock(p, b):
    p.parser.state.block = b
    p.parser.state.blocklists[-1] += [b]

