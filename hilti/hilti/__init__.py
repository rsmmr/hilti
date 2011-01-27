# $Id$

__all__ = []

# The short X.Y version.
version = "0.1"

# The full version, including alpha/beta/rc tags.
release = "0.1-alpha"

# The @type.hilti decorator registers types in this list.
_types = {}

# Load first to register types.
import instructions.__load

import parser
import resolver
import validator
import printer
import canonifier
import codegen as cg
import protogen as pg

import sys

def parseModule(filename, import_paths=["."]):
    """Parses a file into a module structure.

    filename: string - The name of the file to parse.
    import_paths: list of strings - Search paths for ``import`` statement.

    Returns: tuple (int, ~~Module) - The integer is the number of errors found
    during parsing. If there were no errors, ~~Node is the root of the parsed
    module.
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

    if not filename.endswith(".hlt"):
        filename += ".hlt"

    fullpath = util.findFileInPaths(filename, import_paths, True)
    if not fullpath:
        return False

    (errors, nmod) = parseModule(fullpath, import_paths)
    if not errors:
        mod.importIDs(nmod)
        return True
    else:
        return False

def resolveModule(mod):
    """Resolves all still unknown identifiers in a module. After
    parsing a module with ~~parseModule, the resolver must be called
    before any other functions operator on the module. Note that the
    resolver will not report any errors (such as identifiers it
    can't resolve) itself, but leave that to ~validateModule.

    mod: ~~Module - The module to validate.
    """
    return resolver.Resolver().resolve(mod)

def validateModule(mod):
    """ Validates the semantic correctness of a module. Inside the HILTI
    compiler, the validator is the primary component reporting semantic errors
    to the user. With the exception of ~~parserModule (which reports all
    syntax errors as well as some semantic errors directly when reading an
    input file), other components outsource their error checking to the
    validator and generally just double-check any properties they require with
    assertions.

    If this function detects any errors, suitable error messages are output
    via ~~util. This function should be called whenever a module can
    potentially violate any semantic constraints being relied upon by other
    components. In particular, an module should be checked when returned from
    either ~~parseModule and ~~canonifyModule (if the former does not return
    an error but the latter does, that indicates a bug in the canonifier).

    mod: ~~Module - The module to validate.

    Returns: int - The number of errors detected; zero means everything is
    fine.
    """
    return validator.Validator().validate(mod)

def canonifyModule(mod, debug=0):
    """Transforms a module into a canonified form suitable for code
    generation. The canonified form ensures a set of properties that simplify
    the code generation process, such as enforcing a fully linked block
    structure and replacing some generic instructions with more specific ones.
    Once a module has been canonified, it can be passed on to ~~codegenModule.

    Note: In the future, the canonifier will also be used to implement
    additional 'syntactic sugar' for the HILTI language, so that we can
    writing programs in HILTI more convenient without requiring additional
    complexity for the code generator.

    mod: ~~Module - The module to canonify. The module must be well-formed as
    verified by ~~validateModule. It will be changed in place.

    debug: int - If debug is larger than 0, the canonifier may insert
    additional code for helping with debugging.
    """
    return canonifier.Canonifier().canonify(mod, debug)

def codegen(mod, libpaths, debug=0, stack=0, trace=False, verify=True, profile=0):
    """Compiles a module into LLVM module.  The module must be well-formed as
    verified by ~~validateModule, and it must have been canonified by
    ~~canonifyModule.

    mod: ~~Node - The root of the module to turn into LLVM.

    libpaths: list of strings - List of paths to be searched for libhilti prototypes.

    verify: bool - If true, the correctness of the generated LLVM code will
    be verified via LLVM's internal validator.

    debug: int - Debugging level. With 1 or 2, debugging support or more
    debugging support is compiled in.

    stack: int - The default stack segment size. If zero, a default value will
    be chosen.

    trace: bool - If true, debugging information will be printed to trace
    where codegeneration currently is.

    profile: int - Profiling level. When > 0, profiling support is compiled
    in, with higher levels meaning more detailed profiling.

    Returns: tuple (bool, llvm.core.Module) - If the bool is True, code generation (and
    if *verify* is True, also verification) was successful. If so, the second
    element of the tuple is the resulting LLVM module.
    """

    if stack == 0:
        stack = 16384

    gen = cg.CodeGen()
    mod._cg = gen
    return gen.codegen(mod, libpaths, debug, stack, trace, verify, profile)

def protogen(mod, fname):
    """Generates C interface prototypes for the functions in a module. The
    module must be well-formed as verified by ~~validateModule, and it must
    have been canonified by ~~canonifyModule. The output is a C include file
    with prototypes for all exported HILTI identifiers.

    mod: ~~Node - The root of the mod to turn into LLVM.

    fname: string - The name of the output file to write the prototypes into.
    """
    return pg.ProtoGen().protogen(mod._cg, mod, fname)

def printModule(mod, output=sys.stdout, prefix=""):
    """Prints out a module as a HILTI source code.  The output will be a valid
    HILTI program that can be reparsed into an equivalent module.

    mod: ~~Node - The module to print. The module must be well-formed as
    verified by ~~validateModule.

    output: file - The file to write the output to.

    prefix: string - Each output line begin with the prefix.
    """
    return printer.Printer().printNode(mod, output, prefix)


