# $Id$
#
# The parser.

import os.path
import warnings
import sys

# ply.yacc triggers "DeprecationWarning: the md5 module is deprecated; use hashlib instead"
with warnings.catch_warnings():
    warnings.simplefilter("ignore", DeprecationWarning)
    import ply.yacc

from hilti.core import *

import resolver
import lexer
from lexer import tokens

# The current parser. Not nice that we need this, but in some case the error
# handler doesn't seem to have access to the parser otherwise. 
_Parser = None

# Creates a Location object from the parser object for the given symbol.
def loc(p, num):
    """Creates a location object for a symbol.
    
    p: parser object - The parser object as handed to any of parsing functions.
    num: int - The index of the symbol in *p* for from the location
    information is to be extracted.
    
    Returns: ~~Location - The location for the symbol.
    """
    
    assert p.parser.state.filename
    l = location.Location(p.parser.state.filename, p.lineno(num))
    
    if p.parser.state.internal_module:
        l.markInternal()

    return l

# Tracks state during parsing. 
class State:
    """Keeps state during parsing.
    
    filename: string - The name of the file we are parsing.
    """
    def __init__(self, filename):
        self.filename = filename
        self.errors = 0
        self.module = None
        self.internal_module = False
        self.function = None
        self.block = None
        self.import_paths = []
        self.imported_files = []
        self.comment = []

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
    p.parser.state.module = module.Module(p[-1], location=loc(p, -1))
    p.parser.state.module.setComment(p.parser.state.comment)
    
    # Implicitly import the internal libhilti functions.
    if p.parser.state.module.name() != "libhilti" and not p.parser.state.internal_module:
        if not _importFile(p.parser, "libhilti.hlt", loc(p, -1), internal_module=True):
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
                   | def_function"""
                   
    if p[1]:
        p[1].setComment(p.parser.state.comment)

def p_module_decl_error(p):
    """module_decl : error"""
    # Try to resynchronize here.
    pass

def p_def_global(p):
    """def_global : GLOBAL global_id NL"""
    p.parser.state.module.addID(p[2])
    return p[2]

def p_def_const(p):
    """def_const : CONST const_id '=' operand"""
    
    if not isinstance(p[4], instruction.ConstOperand):
        error(p, "value must be a constant")
        raise SyntaxError
    
    p.parser.state.module.addID(p[2], p[4])
    return p[2]

def p_def_local(p):
    """def_local : LOCAL local_id NL"""
    p.parser.state.function.addID(p[2])
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
    decl = type.TypeDeclType(t)
    i = id.ID(name, decl, id.Role.GLOBAL, location=location)
    p.parser.state.module.addID(i)
    type.registerHiltiType(t)
    return i
    
def p_def_struct(p):
    """def_struct_decl : STRUCT _begin_nolines IDENT '{' struct_id_list '}' _end_nolines"""
    _addTypeDecl(p, p[3], type.Struct(p[5]), location=loc(p, 1))
    
def p_def_enum(p):
    """def_enum_decl : ENUM _begin_nolines IDENT '{' id_list '}' _end_nolines"""
    t = type.Enum(p[5])
    nid = _addTypeDecl(p, p[3], t, location=loc(p, 1))
        
    # Register the individual labels.
    for (label, value) in t.labels().items():
        eid = id.ID(p[3] + "::" + label, t, id.Role.CONST, location=loc(p, 1))
        p.parser.state.module.addID(eid, value)
        
    return nid
        
def p_def_bitset(p):
    """def_bitset_decl : BITSET _begin_nolines IDENT '{' id_with_opt_int_init_list '}' _end_nolines"""
    t = type.Bitset(p[5])
    nid = _addTypeDecl(p, p[3], t, location=loc(p, 1))
        
    # Register the individual labels.
    for (label, value) in t.labels().items():
        eid = id.ID(p[3] + "::" + label, t, id.Role.CONST, location=loc(p, 1))
        p.parser.state.module.addID(eid, value)        

    return nid
        
def p_def_exception(p):
    """def_exception_decl : EXCEPTION IDENT opt_exception_arg opt_exception_base"""
    return _addTypeDecl(p, p[2], type.Exception(p[2], p[3], p[4]), location=loc(p, 1))
    
def p_opt_exception_arg(p):
    """opt_exception_arg : '(' type ')'
                             | 
    """
    p[0] = p[2] if len(p) != 1 else None
    
def p_opt_exception_base(p):
    """opt_exception_base : ':' IDENT
                             | 
    """
    p[0] = p[2] if len(p) != 1 else None
    
def p_def_overlay(p):
    """def_overlay_decl : OVERLAY _begin_nolines IDENT '{' overlay_field_list '}' _end_nolines"""
    t = type.Overlay()
    for f in p[5]:
        t.addField(f)
        
    return _addTypeDecl(p, p[3], t, location=loc(p, 1))

def p_def_overlay_field_list(p):    
    """overlay_field_list : overlay_field ',' overlay_field_list
                          | overlay_field"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]

def p_def_overlay_field(p):    
    """overlay_field : IDENT ':' type AT INTEGER INSTRUCTION WITH IDENT operand
                     | IDENT ':' type AFTER IDENT INSTRUCTION WITH IDENT operand
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
    if not _importFile(p.parser, p[2], loc(p, 1)):
        raise SyntaxError

def p_def_declare(p):
    """def_declare : DECLARE function_head"""
    p[2].setLinkage(function.Linkage.EXPORTED)

def p_def_function_head(p):
    """function_head : opt_linkage opt_cc result_type IDENT '(' param_list ')'"""
    ftype = type.Function(p[6], p[3])
    
    if p[2] == function.CallingConvention.HILTI:
        func = function.Function(ftype, None, location=loc(p, 4))
    elif p[2] in (function.CallingConvention.C, function.CallingConvention.C_HILTI):
        # FIXME: We need some way to declare C function which are not part of
        # a module. In function.Function, we already allow module==None in the
        # CC.C case but our syntax does not provide for that currently. 
        func = function.Function(ftype, None, cc=p[2], location=loc(p, 4))
    else:
        # Unknown calling convention
        assert False

    i = id.ID(p[4], func.type(), id.Role.GLOBAL, location=loc(p, 4))
    p.parser.state.module.addID(i, func)

    func.setID(i)
    func.setLinkage(p[1])

    p[0] = func
    
    p.parser.state.function = None
    p.parser.state.block = None

def p_def_opt_cc(p):
    """opt_cc : STRING
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
        
    error(p, "unknown calling convention \"%s\"" % p[1])
    raise SyntaxError
                   
def p_def_opt_linkage(p):
    """opt_linkage : EXPORT
                   | """

    if len(p) == 1:
        p[0] = function.Linkage.LOCAL
    else:
        p[0] = function.Linkage.EXPORTED

def p_def_function(p):
    """def_function : function_head _begin_nolines '{' _instantiate_function  _end_nolines instruction_list _begin_nolines '}' _end_nolines """
    
def p_instantiate_function(p):
    """_instantiate_function :"""
    func = p[-3]
    p.parser.state.function = func
    p.parser.state.block = block.Block(func, location=loc(p, 0))
    p.parser.state.block.setMayRemove(False)
    p.parser.state.function.addBlock(p.parser.state.block)

def p_instruction_list(p):
    """instruction_list : def_local instruction_list
                        | instruction instruction_list
                        | LABEL _set_block_name instruction instruction_list
                        | LABEL _set_block_name NL instruction instruction_list
                        | empty_line instruction_list
                        | """

def p_empty_line(p):
    """empty_line : NL
                  | comment_line NL
    """

def p_comment_line(p):
    """comment_line : COMMENTLINE"""
    p.parser.state.comment += [p[1]]

def p_set_block_name(p):
    """_set_block_name : """
    
    if len(p.parser.state.block.instructions()):
        # Start a new block.
        p.parser.state.block = block.Block(p.parser.state.function, name=p[-1], location=loc(p, 0))
        p.parser.state.function.addBlock(p.parser.state.block)
    else:
        # Current block still empty so just set its name.
        p.parser.state.block.setName(p[-1])

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
        location = loc(p, 3)
    else:
        name = p[1]
        op1 = p[2]
        op2 = p[3]
        op3 = p[4]
        target = None
        location = loc(p, 1)
        
    ins = instruction.createInstruction(name, op1, op2, op3, target, location=location)
    if not ins:
        error(p, "unknown instruction %s" % name)
        raise SyntaxError
    
    p.parser.state.block.addInstruction(ins)

def p_assignment(p):
    # Syntactic sugar for the assignment operator. 
    """instruction : operand '=' operand"""
    ins = instruction.createInstruction("assign", p[3], None, None, p[1], location=loc(p, 1))
    assert ins
    p.parser.state.block.addInstruction(ins)
    
def p_operand_ident(p):
    """operand : IDENT"""
    ident = id.ID(p[1], type.Unknown(), id.Role.UNKNOWN)
    p[0] = instruction.IDOperand(ident, location=loc(p, 1))

def p_operand_integer(p):
    """operand : INTEGER"""
    const = constant.Constant(p[1], type.Integer(0), location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))

def p_operand_double(p):
    """operand : DOUBLE"""
    const = constant.Constant(p[1], type.Double(), location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))

def p_operand_bool(p):
    """operand : BOOL"""
    const = constant.Constant(p[1], type.Bool(), location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))
    
def p_operand_null(p):
    """operand : NULL"""
    const = constant.Constant(None, type.Reference(type.Wildcard), location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))

def p_operand_string(p):
    """operand : STRING"""
    try:
        string = constant.Constant(util.expand_escapes(p[1], unicode=True), type.String(), location=loc(p, 1))
        p[0] = instruction.ConstOperand(string, location=loc(p, 1))
    except ValueError:
        error(p, "error in escape sequence %s" % p[1])
        raise SyntaxError
    
def p_operand_tuple(p):
    """operand : tuple"""
    p[0] = instruction.TupleOperand(p[1], location=loc(p, 1))

def p_operand_addr(p):
    """operand : ADDR"""
    const = constant.Constant(p[1], type.Addr(), location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))
    
def p_operand_net(p):
    """operand : NET"""
    const = constant.Constant(p[1], type.Net(), location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))

def p_operand_port(p):
    """operand : PORT"""
    const = constant.Constant(p[1], type.Port(), location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))
    
def p_operand_regexp(p):
    """operand : regexp_list"""
    const = constant.Constant(p[1], type.Reference([type.RegExp()]), location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))

def p_regexp_list(p):    
    """regexp_list : REGEXP '|' regexp_list
                   | REGEXP"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]
    
def p_operand_list_non_empty(p):
    """operand : '[' operand_list ']'"""
    
    ops = p[2]
    
    t = ops[0].type()
    for op in ops:
        if t != op.type():
            error(p, "list of inhomogenous types" % p[1])
            raise SyntaxError
    
    const = constant.Constant(ops, type.Reference([type.List([t])]), location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))
 
# FIXME: Can't derive type information.    
#def p_operand_list_empty(p):
#    """operand : '[' ']'"""
#    const = constant.Constant([], type.Reference([type.List()]), location=loc(p, 1))
#    p[0] = instruction.ConstOperand(const, location=loc(p, 1))

def p_operand_bytes(p):
    """operand : BYTES"""
    try:
        string = constant.Constant(util.expand_escapes(p[1], unicode=False), type.Reference([type.Bytes()]), location=loc(p, 1))
        p[0] = instruction.ConstOperand(string, location=loc(p, 1))
    except ValueError:
        error(p, "error in escape sequence %s" % p[1])
        raise SyntaxError
    
def p_operand_type(p):
    """operand : type"""
    p[0] = instruction.TypeOperand(p[1], location=loc(p, 1))

def p_operand_ununsed(p):
    """operand : """
    p[0] = None 

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
    """param_id : param_type IDENT"""
    p[0] = id.ID(p[2], p[1], id.Role.PARAM, location=loc(p, 2))

def p_local_id(p):
    """local_id : type IDENT
                | ANY IDENT"""
    p[0] = id.ID(p[2], p[1], id.Role.LOCAL, location=loc(p, 2))

def p_struct_id(p): 
    """struct_id : type IDENT
                 | type IDENT ATTR_DEFAULT '=' operand"""
                
    if len(p) == 3:
        p[0] = (id.ID(p[2], p[1], id.Role.LOCAL, location=loc(p, 2)), None)
    else:
        p[0] = (id.ID(p[2], p[1], id.Role.LOCAL, location=loc(p, 2)), p[5])
    
def p_global_id(p):
    """global_id : type IDENT
                 | ANY IDENT"""
    p[0] = id.ID(p[2], p[1], id.Role.GLOBAL, location=loc(p, 2))

def p_const_id(p):
    """const_id : type IDENT"""
    p[0] = id.ID(p[2], p[1], id.Role.CONST, location=loc(p, 2))

def p_def_opt_default_val(p):
    """opt_default_val : '=' operand
                    | """
    if len(p) == 3:
        p[0] = p[2]
    else:
        p[0] = None
    
def p_type_generic(p):
    """type : TYPE
            | TYPE '<' type_param_list '>'"""
    if len(p) == 2:
        (success, result) = type.getHiltiType(p[1], [])
    else:
        (success, result) = type.getHiltiType(p[1], p[3])

    if not success:
        error(p, result)
        raise SyntaxError
    
    p[0] = result

def p_type_tuple(p):
    """type : '(' type_list ')'
            | '(' '*' ')'"""
    if p[2] != "*":
        p[0] = type.Tuple(p[2])
    else:
        p[0] = type.Tuple(type.Wildcard)

def p_type_ident(p):
    """type : IDENT"""
    # Must be a type name.
    id = p.parser.state.module.lookupID(p[1])
    if not id:
        error(p, "unknown identifier %s" % p[1])
        raise SyntaxError
    
    if not isinstance(id.type(), type.TypeDeclType):
        error(p, "%s is not a type name" % p[1])
        raise SyntaxError
    
    p[0] = id.type().declType()
    
def p_type_list(p):    
    """type_list : type "," type
                 | type """
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]
    
def p_result_type(p):
    """result_type : type
                   | VOID
                   | ANY"""
                      
    if isinstance(p[1], str):
        if p[1] == "void":
            p[0] = type.Void()
        elif p[1] == "any":
            p[0] = type.Any()
    else:
        p[0] = p[1]

def p_param_type(p):
    """param_type : type
                  | METATYPE
                  | ANY"""
                      
    if isinstance(p[1], str):
        if p[1] == "any":
            p[0] = type.Any()
        elif p[1] == "type":
            p[0] = type.MetaType()
    else:
        p[0] = p[1]
        
def p_type_param_list(p):
    """type_param_list : type_param "," type_param_list
                       | type_param"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]

def p_type_param(p):
    """type_param : INTEGER
                  | DOUBLE 
                  | ATTRIBUTE
                  | type
                  | '*'
    """
    if p[1] == "*":
        p[0] = type.Wildcard
    else:
        p[0] = p[1]
    
def p_type_param_type_name(p):
    """type_param : IDENT
    """
    # Must be a type name.
    id = p.parser.state.module.lookupID(p[1])
    if not id:
        error(p, "unknown identifier %s" % p[1])
        raise SyntaxError
    
    if not isinstance(id.type(), type.TypeDeclType):
        error(p, "%s is not a type name" % p[1])
        raise SyntaxError
    
    p[0] = id.type().declType()
        
def p_struct_id_list(p):
    """struct_id_list : struct_id "," struct_id_list
                      | struct_id"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]

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
    """opt_int_init : '=' INTEGER
                    | """

    return p[2] if len(p) == 3 else None
            
def p_error(p):    
    if p:
        type = p.type.lower()
        value = p.value
        if type == value:
            error(None, "unexpected %s" % type, lineno=p.lineno)
        else:
            error(None, "unexpected %s '%s'" % (type, value), lineno=p.lineno)
    else:
        error(None, "unexpected end of file")

def error(p, msg, lineno=0):
    """Reports an parsing error.
    
    p: parser object - The parser object as handed to any of parsing
    functions; can be None if not available.
    
    msg: string - The error message to output.
    
    lineno: int - The line number to report the error in. If zero and *p* is
    given, the information is pulled from the first symbol in *p*.
    """
    
    if p and lineno <= 0:
        lineno = p.lineno(1)

    if p:
        parser = p.parser
    else:
        global _Parser
        parser = _Parser
        
    parser.state.errors += 1
    loc = location.Location(parser.state.filename, lineno)        
    util.error(msg, component="parser", context=loc, fatal=False)

# Recursively imports another file and makes all declared/exported IDs available 
# to the current module. 
def _importFile(parser, filename, location, internal_module=False):
    (root, ext) = os.path.splitext(filename)
    if not ext:
        ext = ".hlt"
        
    filename = root + ext
    
    fullpath = util.findFileInPaths(filename, parser.state.import_paths, lower_case_ok=True)
    if not fullpath:
        util.error("cannot find %s for import" % filename, context=location)

    imp_paths = [path for (mod, path) in parser.state.imported_files]
    if fullpath in imp_paths:
        # Already imported.
        return True

    parser.state.imported_files += [(filename, fullpath)]
    
    nhi = internal_module or parser.state.internal_module
    
    (errors, ast, subparser) = _parse(fullpath, parser.state.import_paths, internal_module=nhi)
    if errors > 0:
        return False

    substate = subparser.state

    for i in substate.module.IDs():
        
        if i.imported():
            # Don't import IDs recursively.
            continue
        
        t = i.type()

        # FIXME: Can we unify functions and other types here? Probably once we
        # have a linkage for all IDs, see below.
        if isinstance(t, type.Function):
            func = substate.module.lookupIDVal(i)
            assert func and isinstance(func.type(), type.Function)
            
            if func.linkage() == function.Linkage.EXPORTED:
                newid = id.ID(i.name(), i.type(), i.role(), scope=i.scope(), imported=True, location=func.location())
                parser.state.module.addID(newid, func)
                
            continue

        # FIXME: We should introduce linkage for all IDs so that we can copy
        # only "exported" ones over.
        if isinstance(t, type.TypeDeclType) or i.role() == id.Role.CONST:
            val = substate.module.lookupIDVal(i)
            newid = id.ID(i.name(), i.type(), i.role(), scope=i.scope(), imported=True, location=i.location())
            parser.state.module.addID(newid, val)
            continue
        
        # Cannot export types other than those above at the moment. 
        util.internal_error("can't handle IDs of type %s (role %d) in import" % (repr(t), i.role()))

    return True
        
def parse(filename, import_paths=["."], internal_module=False):
    """See ~~parse."""
    (errors, ast, parser) = _parse(filename, import_paths, internal_module)
    return (errors, ast)
        
def _parse(filename, import_paths=["."], internal_module=False):
    parser = ply.yacc.yacc(debug=0, write_tables=0)
    parser.state = State(filename)
    parser.state.import_paths = import_paths
    parser.state.internal_module = internal_module

    filename = os.path.expanduser(filename)
    
    try:
        lines = open(filename).read()
    except IOError, e:
        util.error("cannot open file %s: %s" % (filename, e))

    global _Parser
    oldparser = _Parser
    _Parser = parser
        
    try:
        ast = parser.parse(lines, lexer=lexer.buildLexer(), debug=0)
    except ply.lex.LexError, e:
        # Already reported.
        ast = None

    _Parser = oldparser
        
    if parser.state.errors > 0:
        return (parser.state.errors, None, parser)
        
    resolver_errors = resolver.resolver.resolveOperands(ast)
    
    if resolver_errors > 0:
        return (resolver_errors, None, parser)

    parser.state.module._imported_modules = parser.state.imported_files
    
    return (0, ast, parser)    
