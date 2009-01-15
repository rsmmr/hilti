# $Id$
#
# The parser.

import sys

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

def p_module(p):
    """module : MODULE _instantiate_module IDENT NL module_decl_list"""
    p[0] = p.parser.current.module

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
                   | def_function"""
    pass
                   

def p_module_decl_error(p):
    """module_decl : error"""
    # Try to resynchronize here.
    pass

def p_def_global(p):
    """def_global : GLOBAL id NL"""
    p.parser.current.module.addID(p[2])

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
    p.parser.current.module.addID(sid)

def p_def_function(p):
    """def_function : type IDENT '(' param_list ')' _begin_nolines '{' _instantiate_function  _end_nolines instruction_list _begin_nolines '}' _end_nolines """
    p.parser.current.function = None
    p.parser.current.block = None
    
def p_instantiate_function(p):
    """_instantiate_function :"""
    ftype = type.FunctionType(p[-4], p[-7])
    func = function.Function(p[-6], ftype, location=loc(p, 0))
    p.parser.current.function = func
    p.parser.current.block = block.Block(location=loc(p, 0))
    
    p.parser.current.function.addBlock(p.parser.current.block)
    p.parser.current.module.addFunction(p.parser.current.function)

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
        p.parser.current.block = block.Block(name=p[-1], location=loc(p, 0))
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

        
    ins = instruction.createInstruction(name, op1, op2, op3, target, location=loc(p, 1))
    if not ins:
        error(p, "unknown instruction %s" % name)
        raise ply.yacc.SyntaxError
    
    p.parser.current.block.addInstruction(ins)

def p_operand_ident(p):
    """operand : IDENT"""
    
    local = True
    ident = p.parser.current.function.scope().lookup(p[1])
    if not ident:
        local = False
        ident = p.parser.current.module.scope().lookup(p[1])
        
        if not ident:
            error(p, "unknown identifier %s" % p[1])
            raise ply.yacc.SyntaxError
    
    p[0] = instruction.IDOperand(ident, local, location=loc(p, 1))

def p_operand_number(p):
    """operand : NUMBER"""
    # FIXME: Which type for integer constants?
    const = constant.Constant(p[1], type.Int32, location=loc(p, 1))
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

def p_tuple(p):
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
                  | id """
    if len(p) == 2:
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
    
    if lineno > 0:
        context = "%s:%s" % (Parser.current.filename, lineno)
    else:
        context = parser.current.filename

    Parser.current.errors += 1
    util.error(msg, context=context, fatal=False)
        
def parse(filename):
    """Returns tuple (errs, ast) where *errs* is the number errors found and *ast* is the parsed input.""" 
    
    global Parser
    Parser = ply.yacc.yacc(debug=0, write_tables=0)
    
    Parser.current = State(filename)
    lines = open(filename).read()

    try:
        ast = Parser.parse(lines, lexer=lexer.lexer, debug=0)
    except ply.lex.LexError, e:
        # Already reported.
        pass
    
    return (Parser.current.errors, ast)
    
    
    
        

    
    

    

    
    
    
