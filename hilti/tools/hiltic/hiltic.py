#! /usr/bin/env python2.6
#
# Command-line compiler for HILTI programs.

Version = 0.1

import sys
import os.path
import optparse
import subprocess

import llvm.core
Target = llvm.core.getHostTriple()

try:
    # Get hilti-config information.
    p = subprocess.Popen(["hilti-config", "--pythonpath"], stdout=subprocess.PIPE).communicate()[0].strip()
    sys.path = p.split(":") + sys.path
    addl_flags = subprocess.Popen(["hilti-config", "--hilticflags"], stdout=subprocess.PIPE).communicate()[0].strip()
except:
    addl_flags = ""

import hilti

# Collect -I arguments.
def import_path_callback(option, opt, value, parser):
    try:
        parser.values.import_paths += [value]
    except AttributeError:
        parser.values.import_paths = [value]

version_str = "hiltic %s\ntarget: %s" % (Version, Target)
optparser = optparse.OptionParser(usage="%prog [options] <input-file>", version=version_str)
optparser.add_option("-b", "--bitcode", action="store_true", dest="bitcode", default=False,
                     help="Output LLVM bitcode")
optparser.add_option("-l", "--ll", action="store_true", dest="ll", default=False,
                     help="Output human-readable LLVM .ll code (per default to stdout)")
optparser.add_option("-L", "--ll-no-verify", action="store_true", dest="ll_noverify", default=False,
                     help="Output human-readable LLVM .ll code but do not verify correctness (per default to stdout)")
optparser.add_option("-p", "--hilti-plain", action="store_true", dest="hilti_plain", default=False,
                     help="Output HILTI code as it is parsed (per default to stdout)")
optparser.add_option("-c", "--hilti-canonfied", action="store_true", dest="hilti_canon", default=False,
                     help="Output HILTI code after canonification (per default to stdout)")
optparser.add_option("-C", "--hilti-canonfied-no-verify", action="store_true", dest="hilti_canon_noverify", default=False,
                     help="Output HILTI code after canonification but do not verify correctness (per default to stdout)")
optparser.add_option("-o", "--output", action="store", type="string", dest="output", default=None,
                     help="Store output in file", metavar="FILE")
optparser.add_option("-I", "--import-path", action="callback", callback=import_path_callback, type="string",
                     help="Add DIR to search path for imports", metavar="DIR")
optparser.add_option("-P", "--prototypes", action="store_true", dest="protos", default=False,
                     help="Generate C prototypes")
optparser.add_option("-d", "--debug", action="count", dest="debug", default=0,
                     help="Compile with debugging support. Multiple times increases level.")
optparser.add_option("-D", "--debug-codegen", action="store_true", dest="debug_cg", default=False,
                     help="Print debugging information during code generation phase")
optparser.add_option("-S", "--stack-size", action="store", dest="stack", default=16384,
                     help="Default stack segment size in bytes (default: 16384)")
optparser.add_option("-F", "--profile", action="count", dest="profile", default=0,
                     help="Enable profiling support; each time this option is given, the profiling level is increased by one")
options = None

if addl_flags:
    (options, args) = optparser.parse_args(args=addl_flags.split())

(options, args) = optparser.parse_args(values=options)

if options.ll_noverify:
    options.ll = True

if options.hilti_canon_noverify:
    options.hilti_canon = True

if not "import_paths" in options.__dict__:
    options.import_paths = []

# Always search in current directory.
options.import_paths += ["."]

protos_only = False

if len(args) < 1:
    optparser.error("no input file specified")

if len(args) > 1:
    optparser.error("more than one input file specified")

if not (options.bitcode or options.ll or options.hilti_plain or options.hilti_canon):
    if options.protos:
        protos_only = True

    else:
        optparser.error("no output type specified")

input = args[0]
dest = options.output
prototypes = None

(root, ext) = os.path.splitext(input)

if not dest:
    if options.bitcode:
        dest = root + ".bc"

    if options.ll:
        dest = root + ".ll"

    if options.ll or options.ll_noverify or options.hilti_plain or options.hilti_canon:
        dest = "/dev/stdout"

if options.protos:
    if protos_only:
        prototypes = dest if dest else "/dev/stdout"
    else:
        prototypes = root + ".h"

if dest:
    try:
        output = open(dest, "w")
    except IOError, e:
        print >>sys.stderr, "cannot open %s: %s" % (dest, e)

# Parse input.
(errors, mod) = hilti.parseModule(input, options.import_paths)

if errors > 0:
    print >>sys.stderr, "\n%d error%s found" % (errors, "s" if errors > 1 else "")
    sys.exit(1)

# Resolve unknown symbols.
hilti.resolveModule(mod)

if errors == 0:
    # Verify semantic correctness.
    errors = hilti.validateModule(mod)

if errors > 0:
    print >>sys.stderr, "\n%d error%s found" % (errors, "s" if errors > 1 else "")
    sys.exit(1)

if options.hilti_plain:
    # Output HILTI code as it is parsed.
    hilti.printModule(mod, output)
    sys.exit(0)

# Canonify the code.
if not hilti.canonifyModule(mod, debug=options.debug):
    print >>sys.stderr, "error during canonicalization"
    sys.exit(1)

# Make sure it's still valid code.
if not options.hilti_canon_noverify:
   errs = hilti.validateModule(mod)
else:
   errs = 0

if errs > 0:
    print >>sys.stderr, "%d error(s) after canonicalization" % errs
    sys.exit(1)

if options.hilti_canon:
    # Output canonified HILTI code.
    hilti.printModule(mod, output)
    sys.exit(0)

# Generate code.
(success, llvm_module) = hilti.codegen(mod, options.import_paths, debug=options.debug, stack=options.stack, trace=options.debug_cg, verify=(not options.ll_noverify), profile=options.profile)

if not success:
    print >>sys.stderr, "error during code generation"
    sys.exit(1)

# Generate C prototypes.
if prototypes:
    hilti.protogen(mod, prototypes)

if options.ll:
    # Output human-readable code.
    print >>output, llvm_module
    sys.exit(0)

if options.bitcode:
    # Output bitcode.
    llvm_module.to_bitcode(output)
    sys.exit(0)







