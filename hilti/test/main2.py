#! /usr/bin/env python2.6

import sys

from hilti import *

#lexer = parser.lexer.lexer
#lexer = parser.lexer

m = parser.parser.parse("test.bir")

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
    
if m:
#    print 
#    printer.printer.dispatch(m)
#    
#    print "--------------"
    core.checker.checker.dispatch(m)
    print "---- Original ----------"
    core.printer.printer.dispatch(m)
    
    print "---- Canonified ----------"
    codegen.canonify.canonify.dispatch(m)
    core.printer.printer.dispatch(m)
    core.checker.checker.reset()
    core.checker.checker.dispatch(m)
##    print "---- CPS ----------"
##    codegen.cps.cps.dispatch(m)
##    printer.printer.dispatch(m)
##    checker.checker.reset()
##    checker.checker.dispatch(m)
    
    m = codegen.codegen.codegen.dispatch(m)
    print "--------------"
    print codegen.codegen.codegen.llvmModule(fatalerrors=False)

