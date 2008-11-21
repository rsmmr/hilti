# $Id$
#
# The parser.

import module
import id
import location
import util
import type
import block
import constant
import function
import instruction

import sys

import lexer

import ply.yacc

from lexer import tokens

# Creates a Location object from the parser object for the given symbol.
def loc(p, num):
    assert p.parser.current.filename
    return location.Location(p.parser.current.filename, p.lineno(num))

# Tracks state during parsing. 
class State:
    def __init__(self, filename):
        self.filename = filename
        self.module = None
        self.function = None
        self.block = None

def p_module(p):
    """module : MODULE _instantiate_module IDENT module_decl_list"""
    p[0] = p.parser.current.module
    
def p_instantiate_module(p):
    """_instantiate_module :"""
    p.parser.current.module = module.Module(p[-1], location=loc(p, -1))
    
def p_module_decl_list(p):
    """module_decl_list :   module_decl module_decl_list
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
    """def_global : GLOBAL id"""
    p.parser.current.module.addID(p[2])

def p_def_local(p):
    """def_local : LOCAL id"""
    p.parser.current.function.addID(p[2])
    
def p_def_type(p):
    """def_type : def_struct"""
    pass
    
def p_def_struct(p):
    """def_struct : STRUCT IDENT '{' id_list '}'"""
    stype = type.StructType(p[4])
    struct = type.StructDeclType(stype)
    sid = id.ID(p[2], struct, location=loc(p, 1))
    p.parser.current.module.addID(sid)

def p_def_function(p):
    """def_function : TYPE IDENT '(' param_list ')' '{' _instantiate_function instruction_list '}' """
    p.parser.current.function = None
    p.parser.current.block = None
    
def p_instantiate_function(p):
    """_instantiate_function :"""
    ftype = type.FunctionType(p[-3], p[-6])
    func = function.Function(p[-5], ftype, location=loc(p, 0))
    p.parser.current.function = func
    p.parser.current.block = block.Block(location=loc(p, 0))
    
    p.parser.current.function.addBlock(p.parser.current.block)
    p.parser.current.module.addFunction(p.parser.current.function)

def p_instruction_list(p):
    """instruction_list : def_local instruction_list
                        | instruction instruction_list
                        | """

def p_instruction(p):
    """instruction : IDENT '=' INSTRUCTION operand operand"""
    """            |           INSTRUCTION operand operand"""
    
    if len(p) == 6:
        name = p[3]
        op1 = p[4]
        op2 = p[5]
        target = p[1]
    else:
        name = p[1]
        op1 = p[2]
        op2 = p[3]
        target = None
    
    ins = instruction.createInstruction(name, op1, op2, target, location=loc(p, 1))
    p.parser.current.block.addInstruction(ins)

def p_operand_ident(p):
    """operand : IDENT"""
    
    ident = p.parser.current.function.scope().lookup(p[1])
    if not ident:
        indent = p.parser.current.module.scope().lookup(p[1])
        
        if not ident:
            error(p.parser, "unknown identifier %s" % p[1])
            raise ply.yacc.SyntaxError
    
    p[0] = instruction.IDOperand(ident, location=loc(p, 1))

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
    """id : TYPE IDENT"""
    p[0] = id.ID(p[2], p[1], location=loc(p, 1))

def p_id_list(p):
    """id_list : id id_list
               | """
    if len(p) == 1:
        p[0] = []
    else:
        p[0] = [p[1]] + p[2]

def error(parser, msg, lineno=0):
    
    if lineno > 0:
        context = "%s:%s" % (parser.current.filename, p.lineno)
    else:
        context = parser.current.filename
    
    util.error(msg, context=context, fatal=False)
        
def p_error(p):    
    if p:
        error(Parser, "unexpected %s '%s'" % (p.type.lower(), p.value), p.lineno)
    else:
        error(Parser, "unexpected end of file")
    
def parse(filename):
    global Parser
    Parser = ply.yacc.yacc(debug=0, write_tables=0)
    Parser.current = State(filename)

    lines = open(filename).read()
    return Parser.parse(lines, lexer=lexer.lexer)
        

    
    

    

    
    
    
