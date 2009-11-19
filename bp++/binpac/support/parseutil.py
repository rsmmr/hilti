# $Id$
#
# A set of helpers to be used by a Ply-based parser. 

import sys
import location
import util

import ply.lex

class State(object):
    """Keeps state during parsing. Can be derived from to add custom state.
    
    filename: string - The name of the file we are parsing.
    """
    def __init__(self, filename, import_paths):
        self._filename = filename
        self._import_paths = import_paths
        self._errors = 0
        self._imported_files = []

    def errors(self):
        """Returns the number of errors reported so far. 
        
        Returns: int - Number of errors
        """
        return self._errors
        
    def importPaths(self):
        """Returns the import paths registered with this state.
        
        Returns: list of string - The paths.
        """
        return self._import_paths

def error(p, msg, lineno=0):
    """Reports an parsing error.
    
    p: ply.yacc.YaccProduction - The current production as handed to parsing
    functions by Ply; can be None if not available.
    
    msg: string - The error message to output.
    
    lineno: int - The line number to report the error in. If zero and *p* is
    given, the information is pulled from the first symbol in *p*.
    """

    assert(p)
    
    if lineno <= 0:
        lineno = p.lineno(1)

    if isinstance(p, ply.lex.LexToken):
        parser = p.lexer.parser
    else:
        parser = p.parser
        
    parser.state._errors += 1
    loc = location.Location(parser.state._filename, lineno)        
    util.error(msg, component="parser", context=loc, fatal=False)

def initParser(parser, lexer, state):
    """Initializes a parser object's state. The state will then be assessible
    via the parser's +state+ attribute.
    
    parser: ~~ply.yacc.yacc - The parser to initialize.
    lexer: ply.lex.Lexer - The lexer which is used with the parser.
    state: ~~State - The state object to be associated with the parser.
    
    Returns: ply.yacc.yacc - The new parser.
    """
    lexer.parser = parser
    parser.state = state
    
def checkImport(parser, filename, def_ext, location):
    """Checks whether we can import a file. If it can't find the file within
    the parser's import paths, the function aborts with an error message. If
    it has already been called earlier with the same file, it returns None.
    Otherwise, it returns the full path for the file to be imported.
    
    parser: ~~ply.yacc.yacc - The parser that will be used for importing the
    file.
    
    filename: string - The name of the file to be imported; if no path is
    given, it will be searched within the parser's import paths.
    
    def_ext: string - If *filename* doesn't have an extension, this one is
    added before it's searched.
    
    location: ~~Location - A location object to be used in an error message if
    the file is not found. 
    
    Returns: string or None - As described above.
    """
    (root, ext) = os.path.splitext(filename)
    if not ext:
        ext = ".pac"
        
    filename = root + ext
    
    fullpath = util.findFileInPaths(filename, parser.state._import_paths, lower_case_ok=True)
    if not fullpath:
        util.error("cannot find %s for import" % filename, context=location)

    if fullpath in parser.state._imported_files:
        return None
    
    parser.state._imported_files += [fullpath]
    return fullpath
