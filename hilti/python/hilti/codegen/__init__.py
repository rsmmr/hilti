# $Id$
"""Compiles an |ast| into a LLVM module."""

__all__ = ["codegen"]

import codegen
import protogen
import typeinfo
import system

import flow
import integer
import double
import module
import bool
import void
import string
import any
import metatype
import tuple
import ref
import struct
import channel
import bytes
import operators
import enum
import addr
import port
import overlay
import vector
import list
import net
import regexp
import debug
import bitset

def generateLLVM(ast, libpaths, debug=False, verify=True):
    """Compiles the |ast| into LLVM module.  The |ast| must be well-formed as
    verified by ~~checkAST, and it must have been canonified by ~~canonifyAST.
    
    ast: ~~Node - The root of the |ast| to turn into LLVM.
    libpaths: list of strings - List of paths to be searched for libhilti prototypes.
    verify: bool - If true, the correctness of the generated LLVM code will
    be verified via LLVM's internal validator.
    debug: bool - If true, debugging support is compiled in.
    
    Returns: tuple (bool, llvm.core.Module) - If the bool is True, code generation (and
    if *verify* is True, also verification) was successful. If so, the second
    element of the tuple is the resulting LLVM module.
    """
    return codegen.codegen.generateLLVM(ast, libpaths, verify)

def generateCPrototypes(ast, fname):
    """Generates C interface prototypes for the functions in an |ast|.
    The |ast| must be well-formed as verified by ~~checkAST, and it
    must have been canonified by ~~canonifyAST. The output is a C
    include file with function prototypes for all exported HILTI
    functions.

    ast: ~~Node - The root of the |ast| to turn into LLVM.
    
    fname: string - The name of the output file to write the prototypes into.
    """
    return protogen.protogen.generateCPrototypes(ast, fname)
    
    


