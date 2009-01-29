# $Id$
#
# The parser.

import os.path
import sys
import warnings

# ply.yacc triggers "DeprecationWarning: the md5 module is deprecated; use hashlib instead"
with warnings.catch_warnings():
    warnings.simplefilter("ignore", DeprecationWarning)
    import ply.yacc

from hilti.core import *

import lexer
from lexer import tokens

# Creates a Location object from the parser object for the given symbol.
def loc(p, num):
    assert p.parser.current.filename
    return location.Location(p.parser.current.filename, p.lineno(num))

# Tracks state during parsing. 
class State:
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
    """def_global : GLOBAL id NL"""
    p.parser.current.module.addGlobal(id)

def p_def_local(p):
    """def_local : LOCAL id NL"""
    p.parser.current.function.addID(p[2])
    
def p_def_type(p):
    """def_type : def_struct"""
    pass
    
def p_def_struct(p):
    """def_struct : STRUCT _begin_nolines IDENT '{' id_list '}' _end_nolines"""
    stype = type.StructType(p[5])
    struct = type.StructDeclType(stype)
    sid = id.ID(p[3], struct, location=loc(p, 1))
    p.parser.current.module.addGlobal(sid)

def p_def_import(p):
    """def_import : IMPORT IDENT"""
    if not _importFile(p[2], loc(p, 1)):
        raise ply.yacc.SyntaxError

def p_def_declare(p):
    """def_declare : DECLARE function_head"""
    p[2].setLinkage(function.Function.LINK_EXPORTED)
    p[2].setImported(True) 

def p_def_function_head(p):
    """function_head : opt_cc type IDENT '(' param_list ')'"""
    ftype = type.FunctionType(p[5], p[2])
    func = function.Function(p[3], ftype, p.parser.current.module, location=loc(p, 0))
    func.setCallingConvention(p[1])
    p.parser.current.module.addGlobal(id.ID(func.name(), func.type(), location=loc(p, 0)), func)
    p[0] = func
    
    p.parser.current.function = None
    p.parser.current.block = None
     
def p_def_opt_cc(p):
    """opt_cc : STRING
              | """

    if len(p) == 1:
        p[0] = function.Function.CC_HILTI
        return 
    
    if p[1] == "HILTI":
        p[0] =  function.Function.CC_HILTI
        return
    
    if p[1] == "C":
        p[0] = function.Function.CC_C
        return
        
    error(p, "unknown calling convention \"%s\"" % p[1])
    raise ply.yacc.SyntaxError
                   
    
def p_def_function(p):
    """def_function : function_head _begin_nolines '{' _instantiate_function  _end_nolines instruction_list _begin_nolines '}' _end_nolines """
    
def p_instantiate_function(p):
    """_instantiate_function :"""
    func = p[-3]
    func.setImported(False) 
    p.parser.current.function = func
    p.parser.current.block = block.Block(func, location=loc(p, 0))
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
    else:
        name = p[1]
        op1 = p[2]
        op2 = p[3]
        op3 = p[4]
        target = None
        
    ins = instruction.createInstruction(name, op1, op2, op3, target, location=loc(p, 3))
    if not ins:
        error(p, "unknown instruction %s" % name)
        raise ply.yacc.SyntaxError
    
    p.parser.current.block.addInstruction(ins)

def p_operand_ident(p):
    """operand : IDENT"""
    
    local = True
    ident = p.parser.current.function.scope().lookup(p[1])
    if not ident:
        ident = p.parser.current.function.type().getID(p[1])
        if not ident:
            local = False
            ident = p.parser.current.module.lookupID(p[1])
        
            if not ident:
                if p[1].startswith("@"):
                    l = loc(p, 1)
                    p[0] = instruction.IDOperand(id.ID(p[1], type.Label, location=l), local, location=l)
                    return
                
                error(p, "unknown identifier %s" % p[1])
                raise ply.yacc.SyntaxError
    
    p[0] = instruction.IDOperand(ident, local, location=loc(p, 1))

def p_operand_number(p):
    """operand : NUMBER"""
    const = constant.Constant(p[1], type.Integer, location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))

def p_operand_bool(p):
    """operand : BOOL"""
    const = constant.Constant(p[1], type.Bool, location=loc(p, 1))
    p[0] = instruction.ConstOperand(const, location=loc(p, 1))

def p_operand_string(p):
    """operand : STRING"""
    string = const.Constant(p[1], type.String, location=loc(p, 1))
    p[0] = instruction.ConstOperand(string, location=loc(p, 1))

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
    """param_list : id ',' param_list
                  | id 
                  | """
                  
    if len(p) == 1:
        p[0] = []
    elif len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]
    
def p_id(p):
    """id : type IDENT"""
    p[0] = id.ID(p[2], p[1], location=loc(p, 1))

def p_type(p):
    """type : TYPE
            | TYPE ':' NUMBER"""
    if len(p) == 2:
        p[0] = p[1]
    else:
        if p[1] != type.Integer:
            error(p, "only integer type allows width specifications, not %s" % p[1])
            raise ply.yacc.SyntaxError

        p[0] = type.IntegerType(p[3])
    
def p_id_list(p):
    """id_list : id "," id_list
    
               | id"""
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[3]
        
def p_error(p):    
    if p:
        error(None, "unexpected %s '%s'" % (p.type.lower(), p.value), lineno=p.lineno)
    else:
        error(None, "unexpected end of file")

def error(p, msg, lineno=0):
    global Parser
    if p and lineno < 0:
        lineno = p.lineno(1)
    
    Parser.current.errors += 1
    loc = location.Location(Parser.current.filename, lineno)        
    util.error(msg, context=loc, fatal=False)

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

    for i in substate.module.IDs().values():
        t = i.type()
        
        if isinstance(t, type.FunctionType):
            func = substate.module.lookupGlobal(i.name())
            assert func and isinstance(func.type(), type.FunctionType)
            
            if func.linkage() == function.Function.LINK_EXPORTED:
                Parser.current.module.addGlobal(id.ID(func.name(), func.type(), location=func.location()), func)
            
            continue
        
        # Cannot export types other than functions at the moment. 
        assert False
        
def parse(filename, import_paths=["."]):
    """Returns tuple (errs, ast) where *errs* is the number errors found and *ast* is the parsed input.""" 
    
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
    
    return (Parser.current.errors, ast)
    
    
    
        

    
    

    

    
    
    
