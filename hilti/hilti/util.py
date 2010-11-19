# $Id$
"""Provides a set of general utility functions used across the compiler."""

import os
import os.path
import sys
import glob

import ply.lex

import location

def _print(prefix1, prefix2, msg, context):

    if prefix1:
        prefix = "%s %s" % (prefix1, prefix2)

    else:
        prefix = prefix2

    if context:
        print >>sys.stderr, "%s:" % context,

    if len(prefix.strip()) > 0:
        print >>sys.stderr, "%s," % prefix,
    else:
        print >>sys.stderr, "%s" % prefix,

    print >>sys.stderr, "%s" % msg

def error(msg, component=None, context=None, fatal=True, indent=False):
    """Reports an error message to the user.

    msg: string - The message to report.

    component: string - Optional string that will prefix the message
    to indicate which component reported the problem.

    context: string - Optional string that will be inserted into the
    message at a point suitable to report location information; most
    commonly *context* will be the relevant ~~Location object (which
    converts into a string).

    fatal: bool - If True, the execution of the process is
    terminated after the error message has been printed.

    indent: bool - If true, the text is assumed to contain more details
    about the previous error and will be printed indented.

    """
    if not indent:
        _print(component, "error", msg, context)
    else:
        _print(component, "    ", msg, context)

    if fatal:
        sys.exit(1)

def warning(msg, component=None, context=None):
    """Reports an warning message to the user.

    msg: string - The message to report.

    component: string - Optional string that will prefix the message
    to indicate which component reported the problem.

    context: string - Optional string that will be inserted into the
    message at a point suitable to report location information; most
    commonly *context* will be the relevant ~~Location object (which
    converts into a string).
    """
    _print(component, "warning", msg, context)

def debug(msg, component=None, context=None):
    """Reports an debug message to the user if debug output is enabled.

    msg: string - The message to report.

    component: string - Optional string that will prefix the message
    to indicate which component reported the problem.

    context: string - Optional string that will be inserted into the
    message at a point suitable to report location information; most
    commonly *context* will be the relevant ~~Location object (which
    converts into a string).
    """
    _print(component, "debug", msg, context)

def internal_error(msg, component=None, context=None):
    """Reports an internal message to the user and aborts
    execution.

    msg: string - The message to report.

    component: string - Optional string that will prefix the message
    to indicate which component reported the problem.
    """
    _print(component, "internal error", msg, context)
    assert False

def check_class(val, cls, orig):
    """Checks whether a value is an instance of an expected class.
    If not, an internal error is generated and execution aborted.

    val: any - The value to test.

    cls: class or list of classes - The class(es) of which *val*
    should be an instance. If a list, *val* must be an instance of
    at least one of them.
    """
    if not isinstance(cls, tuple) and not isinstance(cls, list):
        cls = [cls]

    for c in cls:
        if isinstance(val, c):
            return

    internal_error("%s: must have %s but got %s" % (orig, " or ".join([str(c) for c in cls]), repr(val)))

def expand_escapes(str, unicode=True):
    r"""Expands escape sequences. The following escape sequences are supported:

    ============   ============================
    Escape         Result
    ============   ============================
    \\ \\          Backslash
    \\n            Line feed
    \\r            Carriage return
    \\t            Tabulator
    \\uXXXX        16-bit Unicode codepoint (u)
    \\UXXXXXXXX    32-bit Unicode codepoint (u)
    \\xXX          8-bit hex value
    ============   ============================

    str: string - The string to expand.

    unicode: bool - If true, a Unicode string is returned and the
    escape sequences marked as ``(u)`` are supported. If false, a
    raw byte string is returned and the escape sequences marked as
    ``(r)`` are supported.

    Returns: unicode - A unicode string with escape sequences expanded.

    Raises: ValueError - Raised when an illegal sequence was found.
    """

    if unicode:
        result = u""
    else:
        result = ""
    i = 0

    while i < len(str):

        c = str[i]
        i += 1

        try:
            if c == "\\":
                c = str[i]
                i += 1

                if c == "\\":
                    result += "\\"
                elif c == "n":
                    result += "\n"
                elif c == "r":
                    result += "\r"
                elif c == "t":
                    result += "\t"
                elif c == "\"":
                    result += "\""
                elif c == "u" and unicode:
                    result += unichr(int(str[i:i+4], 16))
                    i += 4
                elif c == "U" and unicode:
                    result += unichr(int(str[i:i+8], 16))
                    i += 8
                elif c == "x":
                    result += unichr(int(str[i:i+2], 16))
                    i += 2
                else:
                    raise ValueError

            else:
                result += c

        except IndexError:
            raise ValueError

    return result

def findFileInPaths(filename, dirs, lower_case_ok=False):
    """Searches a file in a list of directories.

    filename: string - The name of the file to search.
    dirs: list of strings - The directories to search in.
    lower_case_ok: bool - If true, finding a lower-case version of the file
    name is ok too.

    Returns: string or None - The full path of the file if found, or None if
    not.
    """

    if lower_case_ok:
        files = (filename, filename.lower())
    else:
        files = (filename, )

    for dir in dirs:
        for name in files:
            fullpath = os.path.realpath(os.path.join(dir, name))
            if os.path.exists(fullpath) and os.path.isfile(fullpath):
                return fullpath

    return None

def importByPattern(modname, pattern):
    """Imports all submodules of a module which match a given pattern. The
    imported submodules will be accessible under the module's namespace, i.e.,
    +<modname>.<submodname>.*+. They are also visible from other parties
    importing +<modname>+ in the usual way.

    modname: string - The name of the module, which must have already been
    imported. Typically, this will be +__name__+ for the current module.

    pattern: string - A glob-style pattern for the module's filename (no path).
    """

    mod = sys.modules[modname]
    dir = os.path.dirname(mod.__file__)

    for fullpath in glob.glob(os.path.join(dir, pattern)):

        (dir, name) = os.path.split(fullpath)

        if name  == "__init__.py":
            continue

        if name.endswith(".py"):
            name = name [:-3]

        mod.__dict__[name] = __import__("%s" % name, mod.__dict__, {}, [], -1)

### A set of helpers to be used by a Ply-based parser.

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

    def increaseErrors(self, n):
        """Increases the error count manually.

        n: int - The number of errors to add to the current count.
        """
        self._errors += n

    def filename(self):
        """Returns the name of the file we are parsing.

        Returns: string - The file name.
        """
        return self._filename

    def importPaths(self):
        """Returns the import paths registered with this state.

        Returns: list of string - The paths.
        """
        return self._import_paths

_location_mod = location

def parser_error(p, msg, lineno=0, location=None):
    """Reports an parsing error.

    p: ply.yacc.YaccProduction - The current production as handed to parsing
    functions by Ply; can be None if not available.

    msg: string - The error message to output.

    lineno: int - The line number to report the error in. If zero and *p* is
    given, the information is pulled from the first symbol in *p*.

    loc: Location - Location information which will override that from
    *p* and *lineno*.
    """

    if p:
        if lineno <= 0:
            lineno = p.lineno(1)

        if isinstance(p, ply.lex.LexToken):
            parser = p.lexer.parser
        else:
            parser = p.parser

        state = parser.state
    else:
        assert _state
        state = _state

    state._errors += 1

    if not location:
        location = _location_mod.Location(state._filename, lineno)

    error(msg, component="parser", context=location, fatal=False)

# This global is a bit unfortunate as it prevents us from having multiple
# parsers in parallel. However, if PLY reports an end-of-file error, it
# doesn't give us a parser object and so we can't access our state in state
# case otherwise.
_state = None

def initParser(parser, lexer, state):
    """Initializes a parser object's state. The state will then be assessible
    via the parser's +state+ attribute.

    parser: ~~ply.yacc.yacc - The parser to initialize.
    lexer: ply.lex.Lexer - The lexer which is used with the parser.
    state: ~~State - The state object to be associated with the parser.

    Returns: ply.yacc.yacc - The new parser.
    """
    global _state
    _state = state
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
    (root, ext) = os.path.splitext(filename.lower())
    if not ext:
        ext = "." + def_ext

    filename = root + ext

    fullpath = findFileInPaths(filename, parser.state._import_paths, lower_case_ok=True)
    if not fullpath:
        error("cannot find %s for import" % filename, context=location)

    if fullpath in parser.state._imported_files:
        return None

    parser.state._imported_files += [fullpath]
    return fullpath

def loc(p, num):
    """Returns a location object for a parser symbol.

    p: ply.yacc.YaccProduction - The current production as handed to parsing
    functions by Ply.

    num: The index of the symbol to generate the location information for.

    Returns: ~~Location - The location.
    """

    assert p and p.parser.state.filename()

    try:
        if p[num].location():
            return p[num].location()
    except AttributeError:
        pass

    return location.Location(p.parser.state.filename(), p.lineno(num))

def unicode_escape(val):
    """Turns a unicode object into a string literal that can be used as a constant in
    a HILTI program.

    val: unicode - The value to encode.

    Returns: string -  The literal with a special characters escaped.

    Note: This works similar to Python's ``unicode_escape`` codec but also
    escapes quotes, which the Python version doesn't. See
    http://bugs.python.org/issue7615.
    """
    val = val.encode("unicode_escape")
    return val.replace('"', r'\x22')

