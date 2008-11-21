# $Id$
#
# The parser.

import module
import id
import location
import util
import type

import sys

import lexer

import ply.yacc

from lexer import tokens

# Creates a Location object from the parser object for the given symbol.
def loc(p, num):
    assert p.parser.current.filename
    return location.Location(p.parser.current.filename, p.lineno(num))

# Track state during parsing. 
class State:
    def __init__(self, filename):
        self.filename = filename
        self.module = None

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
    """def_function : """
    pass # XXX

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
    
def p_error(p):    
    p.lexer.errors += 1
    
    if p:
        context = "%s:%s" % (Current.filename, p.lineno)
        util.error("unexpexted %s '%s'" % (p.type.lower(), p.value), context=context, fatal=False)
    else:
        util.error("unexpexted end of file", context=Current.filename, fatal=False)
    
def parse(filename):
    global Current
    Current = State(filename)
    
    parser = ply.yacc.yacc()
    parser.current = Current 

    lines = open(filename).read()
    return parser.parse(lines, lexer=lexer.lexer, debug=0)
        

    
    

    

    
    
    
