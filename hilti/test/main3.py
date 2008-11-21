#! /usr/bin/env python2.6

import parser.lexer
import parser.parser

import printer

lexer = parser.lexer.lexer

import sys

data = open("test2.bir").read()

lexer.input(data)

while 1:
    tok = lexer.token()
    if not tok: 
        break 
    print tok
    
if lexer.errors > 0:
    print "%d error%s, exiting." % (lexer.errors, "s" if lexer.errors > 1 else "")
    

