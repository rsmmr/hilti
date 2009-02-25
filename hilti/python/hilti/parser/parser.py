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

# Creates a Location object from the parser object for the given symbol.
def loc(p, num):
    """Creates a location object for a symbol.
    
    p: parser object - The parser object as handed to any of parsing functions.
    num: int - The index of the symbol in *p* for from the location
    information is to be extracted.
    
    Returns: ~~Location - The location for the symbol.
    """
    
    assert p.parser.current.filename
    return location.Location(p.parser.current.filename, p.lineno(num))

# Tracks state during parsing. 
class State:
    """Keeps state during parsing.
    
    filename: string - The name of the file we are parsing.
    """
    def __init__(self, filename):
        self.filename = filename
        self.errors = 0
        self.module = None
        self.function = None
        self.block = None
        self.import_paths = []

def p_module(p):
    """module : _eat_newlines MODULE IDENT _instantiate_module NL module_decl_list"""
    p[0] = p.parser.current.module

def p_eat_newlines(p):
    """_eat_newlines : NL _eat_newlines
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
    p.parser.current.module = module.Module(p[-1], location=loc(p, -1))
    
def p_module_decl_list(p):
    """module_decl_list :   module_decl module_decl_list
                        |   empty_line module_decl_list
                        |   """
    pass
    
def p_module_decl(p):
    """module_decl : def_global
                   | def_type
                   | def_declare
                   | def_import
                   | def_function"""
    pass
                   

def p_module_decl_error(p):
    """module_decl : error"""
    # Try to resynchronize here.
    pass

def p_def_global(p):
    """def_global : GLOBAL global_id NL"""
    p.parser.current.module.addID(id)

def p_def_local(p):
    """def_local : LOCAL local_id NL"""
    p.parser.current.function.addID(p[2])
    
def p_def_type(p):
    """def_type : def_struct"""
    pass
    
def p_def_struct(p):
    """def_struct : STRUCT _begin_nolines IDENT '{' local_id_list '}' _end_nolines"""
    stype = type.Struct(p[5])
    struct = type.StructDecl(stype)
    sid = id.ID(p[3], struct, id.Role.GLOBAL, location=loc(p, 1))
    p.parser.current.module.addID(sid)

def p_def_import(p):
    """def_import : IMPORT IDENT"""
    if not _importFile(p[2], loc(p, 1)):
        raise ply.yacc.SyntaxError

def p_def_declare(p):
    """def_declare : DECLARE function_head"""
    p[2].setLinkage(function.Linkage.EXPORTED)

def p_def_function_head(p):
    """function_head : opt_cc type_with_void IDENT '(' param_list ')'"""
    ftype = type.Function(p[5], p[2])
    
    if p[1] == function.CallingConvention.HILTI:
        func = function.Function(p[3], ftype, p.parser.current.module, location=loc(p, 3))
    elif p[1] == function.CallingConvention.C:
        # FIXME: We need some way to declare C function which are not part of
        # a module. In function.Function, we already allow module==None in the
        # CC.C case but our syntax does not provide for that currently. 
        func = function.Function(p[3], ftype, p.parser.current.module, cc=function.CallingConvention.C, location=loc(p, 3))
    else:
        # Unknown calling convention
        assert False
        
    p.parser.current.module.addID(id.ID(func.name(), func.type(), id.Role.GLOBAL, location=loc(p, 3)), func)
    p[0] = func
    
    p.parser.current.function = None
    p.parser.current.block = None
     
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
        
    error(p, "unknown calling convention \"%s\"" % p[1])
    raise ply.yacc.SyntaxError
                   
    
def p_def_function(p):
    """def_function : function_head _begin_nolines '{' _instantiate_function  _end_nolines instruction_list _begin_nolines '}' _end_nolines """
    
def p_instantiate_function(p):
    """_instantiate_function :"""
    func = p[-3]
    p.parser.current.function = func
    p.parser.current.block = block.Block(func, location=loc(p, 0))
    p.parser.current.block.setMayRemove(False)
    p.parser.current.function.addBlock(p.parser.current.block)

def p_instruction_list(p):
    """instruction_list : def_local instruction_list
                        | instruction instruction_list
                        | LABEL _set_block_name instruction instruction_list
                        | LABEL _set_block_name NL instruction instruction_list
                        | empty_line instruction_list
                        | """

def p_empty_line(p):
    """empty_line : NL"""
    pass

def p_set_block_name(p):
    """_set_block_name : """
    
    if len(p.parser.current.block.instructions()):
        # Start a new block.
        p.parser.current.block = block.Block(p.parser.current.function, name=p[-1], location=loc(p, 0))
        p.parser.current.function.addBlock(p.parser.current.block)
    else:
        # Current block still empty so just set its name.
        p.parser.current.block.setName(p[-1])

def p_instruction(p):
    """instruction : operand '=' INSTRUCTION operand operand operand NL
                   |             INSTRUCTION operand operand operand NL"""
    
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
        raise ply.yacc.SyntaxError
    
    p.parser.current.block.addInstruction(ins)

def p_operand_ident(p):
    """operand : IDENT"""
    
    ident = id.ID(p[1], type.Unknown(), id.Role.UNKNOWN)
    p[0] = instruction.IDOperand(ident, location=loc(p, 1))

def p_operand_number(p):
    """operand : NUMBER"""
    const = constant.Constant(p[1], type.Integer(0), location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))

def p_operand_bool(p):
    """operand : BOOL"""
    const = constant.Constant(p[1], type.Bool(), location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))

def p_operand_string(p):
    """operand : STRING"""
    try:
        string = const.Constant(util.expand_escapes(p[1]), type.String(), location=loc(p, 1))
        p[0] = instruction.ConstOperand(string, location=loc(p, 1))
    except ValueError:
        error(p, "error in escape sequence %s" % p[1])
        raise ply.yacc.SyntaxError

def p_operand_tuple(p):
    """operand : tuple"""
    p[0] = instruction.TupleOperand(p[1], location=loc(p, 1))

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
    """param_list : param_id ',' param_list
                  | param_id 
                  | """
                  
    if len(p) == 1:
        p[0] = []
    elif len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]
    
def p_param_id(p):
    """param_id : type IDENT"""
    p[0] = id.ID(p[2], p[1], id.Role.PARAM, location=loc(p, 1))

def p_local_id(p):
    """local_id : type IDENT"""
    p[0] = id.ID(p[2], p[1], id.Role.LOCAL, location=loc(p, 1))

def p_global_id(p):
    """global_id : type IDENT"""
    p[0] = id.ID(p[2], p[1], id.Role.GLOBAL, location=loc(p, 1))

def p_type(p):
    """type : TYPE
            | TYPE '<' type_param_list '>'"""
    if len(p) == 2:
        (success, result) = type.getHiltiType(p[1], [])
        
    else:
        (success, result) = type.getHiltiType(p[1], p[3])

    if not success:
        error(p, result)
        raise ply.yacc.SyntaxError

    # FIXME: This is not great location for this check ...
    if isinstance(result, type.Integer) and result.width() == 0:
        error(p, "integer type requires non-zero width")
        raise ply.yacc.SyntaxError
    
    p[0] = result

def p_type_with_void(p):
    """type_with_void : type
                      | VOID"""
                      
    if p[1] == "void":
        p[0] = type.Void()
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
    """type_param : NUMBER 
                  | IDENT"""
    p[0] = p[1]
        
def p_local_id_list(p):
    """local_id_list : local_id "," local_id_list
               | local_id"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]
        
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
    
    global Parser
    if p and lineno <= 0:
        lineno = p.lineno(1)
    
    Parser.current.errors += 1
    loc = location.Location(Parser.current.filename, lineno)        
    util.error(msg, component="parser", context=loc, fatal=False)

# Find file in dirs and returns full pathname or None if not found.
def _findFile(filename, dirs):
    
    for dir in dirs:
        fullpath = os.path.realpath(os.path.join(dir, filename))
        if os.path.exists(fullpath) and os.path.isfile(fullpath):
            return fullpath
        
    return None
    
# Recursively imports another file and makes all declared/exported IDs available 
# to the current module. 
def _importFile(filename, location):

    global Parser
    oldparser = Parser

    (root, ext) = os.path.splitext(filename)
    if not ext:
        ext = ".hlt"
        
    filename = root + ext
    
    fullpath = _findFile(filename, oldparser.current.import_paths)
    if not fullpath:
        util.error("cannot find %s for import" % filename, context=location)
        
    (errors, ast) = parse(fullpath, oldparser.current.import_paths)
    if errors > 0:
        return (errors, None)
    
    substate = Parser.current
    Parser = oldparser

    for i in substate.module.IDs():
        t = i.type()
        
        if isinstance(t, type.Function):
            func = substate.module.lookupIDVal(i.name())
            assert func and isinstance(func.type(), type.Function)
            
            if func.linkage() == function.Linkage.EXPORTED:
                Parser.current.module.addID(id.ID(func.name(), func.type(), id.Role.GLOBAL, location=func.location()), func)
            
            continue
        
        # Cannot export types other than functions at the moment. 
        assert False
        
def parse(filename, import_paths=["."]):
    """See ~~parse."""
    global Parser
    Parser = ply.yacc.yacc(debug=0, write_tables=0)
    
    Parser.current = State(filename)
    Parser.current.import_paths = import_paths

    filename = os.path.expanduser(filename)
    
    try:
        lines = open(filename).read()
    except IOError, e:
        util.error("cannot open file %s: %s" % (filename, e))

    try:
        ast = Parser.parse(lines, lexer=lexer.buildLexer(), debug=0)
    except ply.lex.LexError, e:
        # Already reported.
        ast = None

    if Parser.current.errors > 0:
        return (Parser.current.errors, None)
        
    resolver_errors = resolver.resolver.resolveOperands(ast)
    
    if resolver_errors > 0:
        return (resolver_errors, None)
    
    return (0, ast)
        
    
    
    
        

    
    

    

    
    
    
