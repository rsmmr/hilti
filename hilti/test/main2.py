#! /usr/bin/env python2.6

import parser.lexer
import parser.parser
import checker
import codegen.codegen
import codegen.canonify
#import codegen.cps

import printer

lexer = parser.lexer.lexer

import sys

m = parser.parser.parse("test.bir")

#data = sys.stdin.read()
#lexer.input(data)
#
#while 1:
#    tok = lexer.token()
#    if not tok: 
#        break 
#    print tok
    
if lexer.errors > 0:
    print "%d error%s, exiting." % (lexer.errors, "s" if lexer.errors > 1 else "")
    sys.exit(1)
    
if m:
#    print 
#    printer.printer.dispatch(m)
#    
#    print "--------------"
    checker.checker.dispatch(m)
    print "---- Original ----------"
    printer.printer.dispatch(m)
    
    print "---- Canonified ----------"
    codegen.canonify.canonify.dispatch(m)
    printer.printer.dispatch(m)
    checker.checker.reset()
    checker.checker.dispatch(m)
##    print "---- CPS ----------"
##    codegen.cps.cps.dispatch(m)
##    printer.printer.dispatch(m)
##    checker.checker.reset()
##    checker.checker.dispatch(m)
    
    m = codegen.codegen.codegen.dispatch(m)
    print "--------------"
    print codegen.codegen.codegen.llvmModule(fatalerrors=False)

