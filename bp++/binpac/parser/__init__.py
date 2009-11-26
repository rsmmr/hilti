# $Id$
"""Parses a BinPAC++ program into an |ast|.""" 

import binpac.parser.parser

def parse(filename, import_paths=["."]):
    """Parses a file into an |ast|.
    
    filename: string - The name of the file to parse.
    import_paths: list of strings - Search paths for ``import`` statement.
    
    Returns: tuple (int, ~~Node) - The integer is the number of errors found
    during parsing. If there were no errors, ~~Node is the root of the parsed
    |ast|.
    """
    (errors, ast, p) = binpac.parser.parser._parse(filename, import_paths)
    return (errors, ast)
