#! /usr/bin/env python2.6

import sys

from hilti import *

import hilti.codegen

#lexer = parser.lexer.lexer
#lexer = parser.lexer

(errs, ast) = parser.parser.parse("test.bir")

#data = sys.stdin.read()
#lexer.input(data)
#
#while 1:
#    tok = lexer.token()
#    if not tok: 
#        break 
#    print tok
    
#if lexer.errors > 0:
#    print "%d error%s, exiting." % (lexer.errors, "s" if lexer.errors > 1 else "")
#    sys.exit(1)
    
if not errs:
#    print 
#    printer.printer.dispatch(m)
#    
#    print "--------------"
    errs = core.checker.checkAST(ast)
    if errs:
        sys.exit()
    
    print "---- Original ----------"
    core.printer.printAST(ast)
    
    print "---- Canonified ----------"
    codegen.canonify.canonifyAST(ast)
    core.printer.printAST(ast)
    errs = core.checker.checkAST(ast)
    if errs:
        sys.exit()

    mod = codegen.codegen.generate(ast, False)
    
    print "--------------"
    print mod 

