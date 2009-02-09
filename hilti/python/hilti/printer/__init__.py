# $Id$

__all__ = []

import sys

import printer
import module

def printAST(ast, output=sys.stdout):
    return printer.printer.printAST(ast, output)




