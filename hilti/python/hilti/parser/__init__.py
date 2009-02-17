# $Id$
"""Parses a HILTI program into an |ast|.""" 

__all__ = []

import parser
import lexer

def parse(filename, import_paths=["."]):
    """Parses a file into an |ast|.
    
    filename: string - The name of the file to parse.
    import_paths: list of strings - Search paths for ``import`` statement.
    
    Returns: tuple (int, ~~Node) - The integer is the number of errors found
    during parsing. If there were no errors, ~~Node is the root of the parsed
    |ast|.
    """
    return parser.parse(filename, import_paths)
