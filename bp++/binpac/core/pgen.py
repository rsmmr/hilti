# $Id$
#
# Turns a grammar into a recursive descent HILTI parser. 

import hilti.core
import hilti.printer

import grammar

_LookAheadType = hilti.core.type.Integer(32)
_LookAheadNone = hilti.core.instruction.ConstOperand(hilti.core.constant.Constant(0, _LookAheadType))

def _flattenToStr(list):
    return " ".join([str(e) for e in list])

class ParserGen:
    def __init__(self, cg):
        """Generates a parser from a grammar.
        
        cg: ~~CodeGen - The code generator to use.
        
        hilti_import_paths: list of string - Paths to search for HILTI modules.
        """
        self._cg = cg
        self._mbuilder = cg.moduleBuilder()
        
        paths = cg.importPaths()
        hilti.parser.importModule(self._mbuilder.module(), "hilti", paths)
        hilti.parser.importModule(self._mbuilder.module(), "binpac", paths)
        hilti.parser.importModule(self._mbuilder.module(), "binpacintern", paths)

        self._builders = {}
        self._rhs_names = {}
        
        self._current_grammar = None
        
    def compile(self, grammar):
        """Adds the parser for a grammar to the HILTI module. 
        
        This method can be called multiple times to generate a set of parsers
        for different grammars, all within the same module.
        
        grammar: Grammar - The grammar to generate the parser from.  The
        grammar must pass ~~Grammar.check.
        
        Returns: hilti.core.Type.Reference - A reference type for 
        struct generated for parsing this grammar. 
        """

        self._current_grammar = grammar
        self._functionHostApplication()
        self._functionInit()
        
        return self._typeParseObjectRef()

    ### Internal class to represent arguments of generated HILTI parsing function. 
    
    class _Args:
        def __init__(self, fbuilder=None, args=None):
            self.cur = fbuilder.idOp(args[0])
            self.obj = fbuilder.idOp(args[1])
            self.lahead = fbuilder.idOp(args[2])
                
        def tupleOp(self):
            return hilti.core.instruction.TupleOperand([self.cur, self.obj, self.lahead])
        
    ### Methods generating parsing functions. 

    def _functionInit(self):
        """Generates the init function registering the parser with the BinPAC
        runtime."""
        grammar = self._current_grammar
        
        ftype = hilti.core.type.Function([], hilti.core.type.Void())
        name = self._name("init")
        fbuilder = hilti.core.builder.FunctionBuilder(self._mbuilder, name, ftype)
        fbuilder.function().setLinkage(hilti.core.function.Linkage.INIT)
        builder = hilti.core.builder.BlockBuilder(None, fbuilder)

        parser = fbuilder.addLocal("parser", fbuilder.typeByID("BinPACIntern::Parser"))
        funcs = fbuilder.addLocal("funcs", hilti.core.type.Tuple([hilti.core.type.CAddr()] * 2))
        f = fbuilder.addLocal("f", hilti.core.type.CAddr())
        
        builder.caddr_function(funcs, builder.idOp(self._name("parse")))
        
        builder.struct_set(parser, builder.constOp("name"), builder.constOp(grammar.name()))
        builder.struct_set(parser, builder.constOp("description"), builder.constOp("No description"))
        builder.tuple_index(f, funcs, builder.constOp(0))
        builder.struct_set(parser, builder.constOp("parse_func"), f)
        builder.tuple_index(f, funcs, builder.constOp(1))
        builder.struct_set(parser, builder.constOp("resume_func"), f)
        builder.call(None, builder.idOp("BinPACIntern::register_parser"), builder.tupleOp([builder.idOp("parser")]))
        builder.return_void()
    
    def _functionHostApplication(self):
        """Generates the C function visible to the host application for
        parsing the given grammar. See ``binpac.h`` for the exact semantics of
        the generated function."""

        grammar = self._current_grammar
        
        bytes_iter = hilti.core.type.IteratorBytes(hilti.core.type.Bytes())
        cur = hilti.core.id.ID("cur", bytes_iter, hilti.core.id.Role.PARAM)
        result = self._typeParseObjectRef()

        args = [cur]
        ftype = hilti.core.type.Function(args, result)
        name = self._name("parse")
        fbuilder = hilti.core.builder.FunctionBuilder(self._mbuilder, name, ftype)
        fbuilder.function().setLinkage(hilti.core.function.Linkage.EXPORTED)
        builder = hilti.core.builder.BlockBuilder(None, fbuilder)

        pobj = fbuilder.addLocal("pobj", self._typeParseObjectRef())
        presult = fbuilder.addLocal("presult", hilti.core.type.Tuple([bytes_iter, _LookAheadType]))
        lahead = fbuilder.addLocal("lahead", _LookAheadType)
        builder.assign(lahead, _LookAheadNone)
        
        builder.new(pobj, builder.typeOp(self._typeParseObject()))
        
        args = ParserGen._Args(fbuilder, ["cur", "pobj", "lahead"])
        
        self._parseProduction(builder, grammar.startSymbol(), args)
        
        builder.return_result(pobj)
        
    ### Methods generating parsing code for Productions.

    def _parseProduction(self, builder, prod, args):
        if isinstance(prod, grammar.Literal):
            return self._parseLiteral(builder, prod, args)
            
        elif isinstance(prod, grammar.Variable):
            return self._parseVariable(builder, prod, args)
            
        elif isinstance(prod, grammar.Epsilon):
            # Nothing to do.
            return builder
        
        elif isinstance(prod, grammar.Sequence):
            return self._parseSequence(builder, prod, args)

        elif isinstance(prod, grammar.LookAhead):
            return self._parseLookAhead(builder, prod, args)
        
        elif isinstance(prod, grammar.Boolean):
            util.internal_error("grammar.Boolean not yet supported")
            
        elif isinstance(prod, grammar.Switch):
            util.internal_error("grammar.Switch not yet supported")

        else:
            util.internal_error("unexpected non-terminal type %s" % repr(prod))
        
    def _parseLiteral(self, builder, lit, args):
        """Generates code to parse a literal."""
        builder.setNextComment("Parsing literal \"%s\"" % lit.literal().value())
        
        fbuilder = builder.functionBuilder()
        
        # See if we have a look-ahead symbol. 
        cond = fbuilder.addLocal("cond", hilti.core.type.Bool(), reuse=True)
        cond = builder.equal(cond, args.lahead, _LookAheadNone)
        (no_lahead, have_lahead, done) = builder.makeIfElse(cond, tag="no-lahead")

        # If we do not have a look-ahead symbol pending, search for our literal.
        match = self._matchToken(no_lahead, "literal", str(lit.id()), [lit.literal().value()], args.cur)
        symbol = fbuilder.addLocal("lahead", hilti.core.type.Integer(32), reuse=True)
        no_lahead.tuple_index(args.lahead, match, no_lahead.constOp(0))
        
        found_lit = hilti.core.builder.BlockBuilder("found-sym", fbuilder)
        found_lit.tuple_index(args.cur, match, found_lit.constOp(1))
        found_lit.jump(done.labelOp())

        values = [no_lahead.constOp(lit.id())]
        branches = [found_lit]
        (default, values, branches) = self._addMatchTokenErrorCases(fbuilder, values, branches, no_lahead)
        no_lahead.makeSwitch(symbol, values, default=default, branches=branches, cont=done, tag="next-sym")
        
        # If we have a look-ahead symbol, its value must match what we expect.
        have_lahead.setComment("Look-ahead symbol pending, check.")
        have_lahead.equal(cond, have_lahead.constOp(lit.id()), args.lahead)
        wrong_lahead = fbuilder.cacheBuilder("wrong-lahead", lambda b: self._parseError(b, "unexpected look-ahead symbol pending"))
        (match, _, _) = have_lahead.makeIfElse(cond, no=wrong_lahead, cont=done, tag="check-lahead")

        # If it matches, consume it (i.e., clear the look-ahead).
        match.setComment("Correct look-ahead symbol pending, will be consumed.")
        match.jump(done.labelOp())

        # Consume look-ahead.
        done.assign(args.lahead, _LookAheadNone)
        return done
        
    def _parseVariable(self, builder, var, args):
        """Generates code to parse a variable."""
        builder.setNextComment("Parsing variable %s" % var)
        ### TODO: Missing.
        return builder

    def _parseSequence(self, builder, prod, args):
        def _makeFunction():
            (fbuilder, builder, args) = self._createParseFunction("sequence", prod)
            builder.setNextComment("Parse function for production \"%s -> %s\"" % (prod.symbol(), prod))
            
            # Initialize cache entry already here so that we can work
            # recursively.
            self._mbuilder.setCacheEntry(prod.symbol(), fbuilder.function())
            
            for p in prod.sequence():
                builder = self._parseProduction(builder, p, args)
                
            builder.return_result(builder.tupleOp([args.cur, args.lahead]))
            return fbuilder.function()
        
        func = self._mbuilder.cache(prod.symbol(), _makeFunction)
        
        builder.setNextComment("Parsing non-terminal %s" % prod.symbol())

        bytes_iter = hilti.core.type.IteratorBytes(hilti.core.type.Bytes())
        result = builder.functionBuilder().addLocal("presult", hilti.core.type.Tuple([bytes_iter, _LookAheadType]), reuse=True)
        builder.call(result, builder.idOp(func.name()), args.tupleOp())
        builder.tuple_index(args.cur, result, builder.constOp(0))
        builder.tuple_index(args.lahead, result, builder.constOp(1))
        
        return builder        
    
    def _parseLookAhead(self, builder, prod, args):
        def _makeFunction():
            (fbuilder, builder, args) = self._createParseFunction("lahead", prod)
            builder.setNextComment("Parse function for production \"%s -> %s\"" % (prod.symbol(), prod))

            # Initialize cache entry already here so that we can work
            # recursively.
            self._mbuilder.setCacheEntry(prod.symbol(), fbuilder.function())
            
            ### See if we have look-ahead symbol pending.
            cond = fbuilder.addLocal("cond", hilti.core.type.Bool(), reuse=True)
            builder.equal(cond, args.lahead, _LookAheadNone)
            (no_lahead, builder) = builder.makeIf(cond, tag="no-lahead")

            ### If no, search for the next pattern.

            literals = prod.lookAheads()
            alts = prod.alternatives()
            
            # Build a regular expression for all the possible symbols. 
            all = literals[0] | literals[1]
            syms = ["%s{#%d}" % (lit.literal().value(), lit.id()) for lit in all]
            match = self._matchToken(no_lahead, "regexp", prod.symbol(), syms, args.cur)
            no_lahead.tuple_index(args.lahead, match, builder.constOp(0))

            ### Now branch according the look-ahead.

            done = hilti.core.builder.BlockBuilder("done", fbuilder)
            
            # Built the cases.
            def _makeBranch(i, alts, literals):
                branch = hilti.core.builder.BlockBuilder("case-%d" % i, fbuilder)
                branch.setComment("For look-ahead set {%s}" % ", ".join(['"%s"' % l.literal().value() for l in literals[i]]))
                nbranch = self._parseProduction(branch, alts[i], args)
                nbranch.jump(done.labelOp())
                return branch
            
            values = []
            branches = []
            cases = [_makeBranch(0, alts, literals), _makeBranch(1, alts, literals)]

            for i in (0, 1):
                case = cases[i]
                for lit in literals[i]:
                    values += [builder.constOp(lit.id())]
                    branches += [case]
                
            (default, values, branches) = self._addMatchTokenErrorCases(fbuilder, values, branches, no_lahead)
            builder.makeSwitch(args.lahead, values, default=default, branches=branches, cont=done, tag="lahead-next-sym")
            
            # Done, return the result.
            done.return_result(done.tupleOp([args.cur, args.lahead]))
            
            return fbuilder.function()        
        
        func = self._mbuilder.cache(prod.symbol(), _makeFunction)

        builder.setNextComment("Parsing non-terminal %s" % prod.symbol())
        
        bytes_iter = hilti.core.type.IteratorBytes(hilti.core.type.Bytes())
        result = builder.functionBuilder().addLocal("presult", hilti.core.type.Tuple([bytes_iter, _LookAheadType]), reuse=True)
        builder.call(result, builder.idOp(func.name()), args.tupleOp())
        builder.tuple_index(args.cur, result, builder.constOp(0))
        builder.tuple_index(args.lahead, result, builder.constOp(1)
        
        return builder        

    ### Methods generating helper code. 
    
    def _parseError(self, builder, msg):
        """Generates code to raise an exception."""
        builder.makeRaiseException("BinPAC::ParseError", builder.constOp(msg))
        
    def _yieldAndTryAgain(self, builder, cont):
        """Generates code that yields and then jumps to a previous block to
        repeat whatever it was doing."""
        builder.yield_()
        builder.jump(cont.labelOp())

    def _matchToken(self, builder, ntag1, ntag2, patterns, cur):
        """Generates standard code around a ``regexp.match`` token
        instruction.
        """
        fbuilder = builder.functionBuilder()
        
        match_rtype = hilti.core.type.Tuple([hilti.core.type.Integer(32), cur.type()])
        match = fbuilder.addLocal("match", match_rtype, reuse=True)
        cond = fbuilder.addLocal("cond", hilti.core.type.Bool(), reuse=True)
        name = self._name(ntag1, ntag2)
        
        def _makePatternConstant():
            cty = hilti.core.type.Reference([hilti.core.type.RegExp()])
            return fbuilder.moduleBuilder().addConstant(name, cty, patterns)
        
        pattern = fbuilder.moduleBuilder().cache(name, _makePatternConstant)
        builder.regexp_match_token(match, pattern, cur)
        return match

    ### Methods for manipulating the parser object.
    
    def _typeParseObject(self):
        """Returns the struct type for the parsed grammar."""
        
        def _makeType():
            # TODO: Not yet implemented. 
            fields = [hilti.core.id.ID("dummy", hilti.core.type.Integer(16), hilti.core.id.Role.LOCAL)]
            structty = hilti.core.type.Struct([(f, None) for f in fields])
            self._mbuilder.addTypeDecl(self._name("object"), structty)
            return structty
        
        return self._mbuilder.cache("struct_%s" % self._current_grammar.name(), _makeType)

    def _typeParseObjectRef(self):
        """Returns a reference to the struct type for the parsed grammar."""
        return hilti.core.type.Reference([self._typeParseObject()])
        
    ### Helper methods.

    def _name(self, tag1, tag2 = None):
        """Combines two tags to an canonicalized ID name."""
        name = "%s_%s" % (self._current_grammar.name(), tag1)
        if tag2:
            name += "_%s" % tag2
        
        # Makes sure the name contains only valid characters. 
        chars = [c if (c.isalnum() or c in "_") else "_0x%x_" % ord(c) for c in name]
        return "".join(chars).lower()
        
    def _createParseFunction(self, prefix, prod):
        """Creates a HILTI function with the standard parse function
        signature."""
        bytes_iter = hilti.core.type.IteratorBytes(hilti.core.type.Bytes())
        
        arg1 = hilti.core.id.ID("cur", bytes_iter, hilti.core.id.Role.PARAM)
        arg2 = hilti.core.id.ID("obj", self._typeParseObjectRef(), hilti.core.id.Role.PARAM)
        arg3 = hilti.core.id.ID("lahead", _LookAheadType, hilti.core.id.Role.PARAM)
        result = hilti.core.type.Tuple([bytes_iter, _LookAheadType]) 

        args = [arg1, arg2, arg3]
        ftype = hilti.core.type.Function(args, result)
        name = self._name(prefix, prod.symbol())
        fbuilder = hilti.core.builder.FunctionBuilder(self._mbuilder, name, ftype)
        bbuilder = hilti.core.builder.BlockBuilder(None, fbuilder)

        return (fbuilder, bbuilder, ParserGen._Args(fbuilder, args))
        
    def _addMatchTokenErrorCases(self, fbuilder, values, branches, repeat):
        """Build the standard error branches for the switch-statement
        following a ``match_token`` instruction."""
        
        # Not found.
        values += [fbuilder.constOp(0)]
        branches += [fbuilder.cacheBuilder("not-found", lambda b: self._parseError(b, "look-ahead symbol not found"))]
        
        # Not enough input.
        values += [fbuilder.constOp(-1)]
        branches += [fbuilder.cacheBuilder("need-input", lambda b: self._yieldAndTryAgain(b, repeat))]
        
        # Unknown case.
        default = fbuilder.cacheBuilder("unexpected-sym", lambda b: b.makeInternalError("unexpected look-ahead symbol returned"))
        
        return (default, values, branches)

    
    
if __name__ == "__main__":
    from binpac.core.grammar import *
    from binpac.core import type
    from binpac.support import constant

    hs = Literal(None, constant.Constant("#", type.Bytes()))
    pl = Literal(None, constant.Constant("(", type.Bytes()))
    pr = Literal(None, constant.Constant(")", type.Bytes()))
    no = Literal(None, constant.Constant("!", type.Bytes()))
    qu = Literal(None, constant.Constant("?", type.Bytes()))
    st = Variable(None, type.Bytes())

    F = Sequence(None, [no, st], symbol="Fact") 
    Q = Sequence(None, [qu, st], symbol="Question")
    
    S = LookAhead(None, None, None, symbol="Session")
    SS = Sequence(None, [pl, S, pr, S], symbol="SS")
    Fs = LookAhead(None, None, None, symbol="Facts")
    FsQ = Sequence(None, [Fs, Q], symbol="FsQ")
    FFs = Sequence(None, [F,Fs], symbol="FFs")
    
    S.setAlternatives(FsQ, SS)
    Fs.setAlternatives(FFs, Epsilon())
    
    root = Sequence(None, [S,hs], symbol="Start")
    g2 = Grammar("Grammar2", root)

    g2.printTables(False)
    
    error = g2.check()
    if error:
        print error

    module = hilti.core.module.Module("ParserTest")
    mbuilder = hilti.core.builder.ModuleBuilder(module)
    pgen = ParserGen(mbuilder, [".", "../../../hilti/libhilti", "../../libbinpac"])
    pgen.compile(g2)

    hilti.printer.printAST(module)
