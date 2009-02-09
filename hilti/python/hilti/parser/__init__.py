# $Id$

__all__ = []

import parser
import lexer

def parse(filename, import_paths=["."]):
    return parser.parse(filename, import_paths)
