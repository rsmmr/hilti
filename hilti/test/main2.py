#! /usr/bin/env python2.6

import parser.lexer
import parser.parser

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
    
if m:
    print 
    printer.printer.dispatch(m)

