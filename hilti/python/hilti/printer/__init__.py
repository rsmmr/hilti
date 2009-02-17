# $Id$
"""Converts an |ast| into a textual HILTI program. The output is supposed to
be a valid HILTI program that can be reparsed into an equivalent |ast|.

Todo: Currently, the printer does in fact not output valid HILTI code but only
a close approximation. That needs to be fixed and corresponding tests added to the
test-suite.
"""

__all__ = []

import sys

import printer
import module

def printAST(ast, output=sys.stdout):
    """Prints out an |ast| as a HILTI program. The |ast| must be well-formed
    as verified by ~~checkAST.
    
    ast: ~~Node - The root of the |ast| to print.
    output: file - The file to write the output to.
    """
    return printer.printer.printAST(ast, output)




