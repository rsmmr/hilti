# $Id$
"""Parses a HILTI program into an |ast|.""" 

__all__ = []

import parser
import lexer
import resolver

def parse(filename, import_paths=["."]):
    """Parses a file into an |ast|.
    
    filename: string - The name of the file to parse.
    import_paths: list of strings - Search paths for ``import`` statement.
    
    Returns: tuple (int, ~~Module) - The integer is the number of errors found
    during parsing. If there were no errors, ~~Node is the root of the parsed
    |ast|.
    """
    return parser.parse(filename, import_paths)

def importModule(mod, filename, import_paths=["."]):
    """Imports an external module from a file into an existing module. If
    errors are encountered, error messages are printed to stderr and the
    module is not imported.

    filename: string - The name of the file to parse.
    import_paths: list of strings - Search paths for ``import`` statement.
        
    Returns: bool - True if no errors were encountered.
    """
    fullpath = parser._findFile(filename, import_paths)
    
    (errors, nmod) = parse(fullpath, import_paths)
    if not errors:
        mod.importIDs(nmod)
        mod._imported_modules += [(filename, fullpath)]
        return True
    else:
        return False
