# $Id$
#
# Turns a grammar into a recursive descent HILTI parser. 

import hilti.core.module
import hilti.core.builder
import hilti.printer

from core import grammar

_LookAheadType = hilti.core.type.Integer(32)
_LookAheadNone = hilti.core.instruction.ConstOperand(hilti.core.constant.Constant(0, _LookAheadType))

def _flattenToStr(list):
    return " ".join([str(e) for e in list])

class ParserGen:
    def __init__(self, module, hilti_import_paths=["."]):
        """Generates a parser from a grammar.
        
        module: hilti.core.module - The HILTI module to which the generated
        code will be added.
        
        hilti_import_paths: list of string - Paths to search for HILTI modules.
        """
        self._mbuilder = hilti.core.builder.ModuleBuilder(module)
        
        hilti.parser.importModule(self._mbuilder.module(), "hilti", hilti_import_paths)
        hilti.parser.importModule(self._mbuilder.module(), "binpac", hilti_import_paths)
        hilti.parser.importModule(self._mbuilder.module(), "binpac-intern", hilti_import_paths)

        self._builders = {}
        self._rhs_names = {}
        
        self._current_grammar = None
        self._current_ptab = None
        
    def compile(self, grammar):
        """Adds the parser for a grammar to the HILTI module. 
        
        This method can be called multiple times to generate a set of parsers
        for different grammars, all within the same module.
        
        grammar: Grammar - The grammar to generate the parser from.  The
        grammar must pass ~~Grammar.check.
        """

        self._current_grammar = grammar
        self._current_ptab = grammar.parseTable()
        
        self._functionHostApplication()
        self._functionInit()

    ### Internal class to represent arguments of generated HILTI parsing function. 
    
    class _Args:
        def __init__(self, fbuilder, args):
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

        parser = fbuilder.addLocal("parser", builder.idOp("BinPACIntern::Parser"))
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
        
        builder.new(pobj, builder.typeOp(self._typeParseObject()))
        
        parse_func = self._functionNonTerminal(grammar.startSymbol())
        builder.call(presult, builder.idOp(parse_func.name()), builder.tupleOp([builder.idOp(cur), pobj, _LookAheadNone]))
        
        builder.return_result(pobj)
        
    def _functionNonTerminal(self, symbol):
        """Generates the HILTI function for parsing a non-terminal."""
        
        rules = self._current_ptab[symbol]
        lookaheads = rules.keys()

        # If we have only one look-ahead symbol, the decision is easy: there
        # is only a single options for the RHS.
        if len(lookaheads) == 1:
            return self._functionRhsWithoutLookAhead(symbol, rules[lookaheads[0]])
            
        # If there are multiple look-ahead symbols, they must be all literals (a
        # property that check() ensures that). We then need to peek at the next
        # symbol to decide which one to take.
        for sym in lookaheads:
            assert isinstance(sym, grammar.Literal)
            
        return self._functionRhsWithLookAhead(symbol, rules)
        
    def _functionRhsWithoutLookAhead(self, symbol, rhs):
        """Generates the HILTI function for parsing a non-terminal for that 
        we do not need to do any look-ahead for deciding which RHS to use. 
        """
        def _makeSymFunction():
            (fbuilder, builder, args) = self._createParseFunction("lahead", symbol)
            builder = self._generateParseRhs(builder, rhs, args)
            builder.return_result(builder.tupleOp([args.cur, args.lahead]))
            return fbuilder.function()
        
        return self._mbuilder.cache(rhs, _makeSymFunction)
    
    def _functionRhsWithLookAhead(self, symbol, rules): 
        """Generates the HILTI function for parsing a non-terminal for that we
        need to do look-ahead for the next literal to decide which RHS to
        use."""
        
        def _makeLookAheadFunction():
            literals = rules.keys()
            (fbuilder, builder, args) = self._createParseFunction("lahead", symbol)

            # Initialize cache entry already here so that we can work
            # recursively.
            self._mbuilder.setCacheEntry(symbol, fbuilder.function())
            
            ### See if we have look-ahead symbol pending.
            cond = fbuilder.addLocal("cond", hilti.core.type.Bool(), reuse=True)
            builder.equal(cond, args.lahead, _LookAheadNone)
            (no_lahead, builder) = builder.makeIf(cond, tag="no-lahead")

            ### If no, search for the next pattern.
            
            # Build a regular expression for all the possible symbols. 
            syms = ["%s{#%d}" % (lit.literal().value(), lit.id()) for lit in literals]
            match = self._generateMatchToken(no_lahead, "regexp", symbol, syms, args.cur)
            builder.tuple_index(args.lahead, match, builder.constOp(0))

            ### Now branch according the look-ahead.

            # Built the branches.
            values = []
            branches = []

            done = hilti.core.builder.BlockBuilder("done", fbuilder)
            
            for lit in literals:
                rhs = rules[lit]
                
                def _makeBranch():
                    branch = hilti.core.builder.BlockBuilder("case", fbuilder)
                    nbranch = self._generateParseRhs(branch, rhs, args)
                    nbranch.jump(done.labelOp())
                    return branch
                
                branch = fbuilder.cache(rhs, _makeBranch)
                branch.setComment("-> Sym: %s" % lit)
                
                values += [builder.constOp(lit.id())]
                branches += [branch]
                
            (default, values, branches) = self._addMatchTokenErrorCases(fbuilder, values, branches, no_lahead)
            builder.makeSwitch(args.lahead, values, default=default, branches=branches, cont=done, tag="lahead-next-sym")
            
            # Done, return the result.
            done.return_result(done.tupleOp([args.cur, args.lahead]))
            
            return fbuilder.function()
            
        return self._mbuilder.cache(symbol, _makeLookAheadFunction)            

    
    ### Methods generating blocks of HILTI code. 

    def _generateParseRhs(self, builder, rhs, args):
        builder.setComment("-> Rhs: %s" % _flattenToStr(rhs))
        
        # Iterate through the RHS elements and parse them one after the other. 
        for prod in rhs:
            if isinstance(prod, str): # NonTerminal
                builder = self._generateParseNonTerminal(builder, prod, args)
            elif isinstance(prod, grammar.Literal):
                builder = self._generateParseLiteral(builder, prod, args)
            elif isinstance(prod, grammar.Variable):
                builder = self._generateParseVariable(builder, prod, args)
            elif isinstance(prod, grammar.EpsilonClass):
                # Nothing to do.
                pass
            else:
                util.internal_error("unexpected production in _generateParseRhs: %s" % prod)

        return builder    
    
    def _generateParseLiteral(self, builder, lit, args):
        """Generates code to parse a literal."""
        builder.setNextComment("Parsing literal \"%s\"" % lit.literal())
        
        fbuilder = builder.functionBuilder()
        
        # See if we have a look-ahead symbol. 
        cond = fbuilder.addLocal("cond", hilti.core.type.Bool(), reuse=True)
        cond = builder.equal(cond, args.lahead, _LookAheadNone)
        (no_lahead, have_lahead, done) = builder.makeIfElse(cond, tag="no-lahead")

        # If we do not have a look-ahead symbol pending, search for our literal.
        match = self._generateMatchToken(no_lahead, "literal", str(lit.id()), [lit.literal().value()], args.cur)
        symbol = fbuilder.addLocal("sym", hilti.core.type.Integer(32), reuse=True)
        builder.tuple_index(symbol, match, builder.constOp(0))
        
        found_lit = hilti.core.builder.BlockBuilder("found-sym", fbuilder)
        found_lit.tuple_index(args.cur, match, builder.constOp(1))
        found_lit.jump(done.labelOp())

        values = [no_lahead.constOp(lit.id())]
        branches = [found_lit]
        (default, values, branches) = self._addMatchTokenErrorCases(fbuilder, values, branches, no_lahead)
        builder.makeSwitch(symbol, values, default=default, branches=branches, cont=done, tag="next-sym")
        
        # If we have a look-ahead symbol, its value must match what we expect.
        have_lahead.setComment("Look-ahead symbol pending, check.")
        have_lahead.equal(cond, have_lahead.constOp(lit.id()), args.lahead)
        wrong_lahead = fbuilder.cacheBuilder("wrong-lahead", lambda b: self._generateParseError(b, "unexpected look-ahead symbol pending"))
        (match, _, _) = have_lahead.makeIfElse(cond, no=wrong_lahead, cont=done, tag="check-lahead")

        # If it matches, consume it (i.e., clear the look-ahead).
        match.setComment("Correct look-ahead symbol pending, consume.")
        match.assign(args.lahead, _LookAheadNone)

        return done
        
    def _generateParseVariable(self, builder, var, args):
        """Generates code to parse a variable."""
        builder.setNextComment("Parsing variable %s" % var)
        ### TODO: Missing.
        return builder
    
    def _generateParseNonTerminal(self, builder, prod, args):
        """Generates code to parse a non-terminal."""
        builder.setNextComment("Parsing non-terminal %s" % prod)
        fbuilder = builder.functionBuilder()
        result = fbuilder.addLocal("nterm", fbuilder.function().type().resultType(), reuse=True)
        func = self._functionNonTerminal(prod)
        
        builder.call(result, builder.idOp(func.name()), args.tupleOp())
        builder.tuple_index(args.cur, result, builder.constOp(0, hilti.core.type.Integer(32)))
        builder.tuple_index(args.lahead, result, builder.constOp(1, hilti.core.type.Integer(32)))
        
        return builder        
    
    def _generateParseError(self, builder, msg):
        """Generates code to raise an exception."""
        builder.makeRaiseException("BinPAC::ParseError", builder.constOp(msg, hilti.core.type.String()))
        
    def _generateYieldAndTryAgain(self, builder, cont):
        """Generates code that yields and then jumps to a previous block to
        repeat whatever it was doing."""
        builder.yield_()
        builder.jump(cont.labelOp())

    def _generateMatchToken(self, builder, ntag1, ntag2, patterns, cur):
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
        
    def _createParseFunction(self, prefix, tag):
        """Creates a HILTI function with the standard parse function
        signature."""
        bytes_iter = hilti.core.type.IteratorBytes(hilti.core.type.Bytes())
        
        arg1 = hilti.core.id.ID("cur", bytes_iter, hilti.core.id.Role.PARAM)
        arg2 = hilti.core.id.ID("obj", self._typeParseObjectRef(), hilti.core.id.Role.PARAM)
        arg3 = hilti.core.id.ID("lahead", _LookAheadType, hilti.core.id.Role.PARAM)
        result = hilti.core.type.Tuple([bytes_iter, _LookAheadType]) 

        args = [arg1, arg2, arg3]
        ftype = hilti.core.type.Function(args, result)
        name = self._name(prefix, tag)
        fbuilder = hilti.core.builder.FunctionBuilder(self._mbuilder, name, ftype)
        bbuilder = hilti.core.builder.BlockBuilder(None, fbuilder)

        return (fbuilder, bbuilder, ParserGen._Args(fbuilder, args))
        
    def _addMatchTokenErrorCases(self, fbuilder, values, branches, repeat):
        """Build the standard error branches for the switch-statement
        following a ``match_token`` instruction."""
        
        # Not found.
        values += [fbuilder.constOp(0)]
        branches += [fbuilder.cacheBuilder("not-found", lambda b: self._generateParseError(b, "look-ahead symbol not found"))]
        
        # Not enough input.
        values += [fbuilder.constOp(-1)]
        branches += [fbuilder.cacheBuilder("need-input", lambda b: self._generateYieldAndTryAgain(b, repeat))]
        
        # Unknown case.
        default = fbuilder.cacheBuilder("unexpected-sym", lambda b: b.makeInternalError("unexpected look-ahead symbol returned"))
        
        return (default, values, branches)

    
    
if __name__ == "__main__":
    from core.grammar import *
    from core import type
    from support import constant
    
    hs = Literal(constant.Constant("#", type.Bytes()))
    pl = Literal(constant.Constant("(", type.Bytes()))
    pr = Literal(constant.Constant(")", type.Bytes()))
    no = Literal(constant.Constant("!", type.Bytes()))
    qu = Literal(constant.Constant("?", type.Bytes()))
    st = Variable(type.Bytes())

    F = Sequence([no, st], sym="Fact") 
    Q = Sequence([qu, st], sym="Question")
    
    S = Alternative([], sym="Session")
    SS = Sequence([pl, S, pr, S])
    Fs = Alternative([], sym="Facts")
    FsQ = Sequence([Fs, Q])
    FFs = Sequence([F,Fs])
    
    S.addProduction(FsQ)
    S.addProduction(SS)
    
    Fs.addProduction(FFs)
    Fs.addProduction(Epsilon)

    root = Sequence([S,hs], sym="Start")
    g2 = Grammar("FooBar", root)
    
    error = g2.check()
    if error:
        print error

    print g2
        
    module = hilti.core.module.Module("ParserTest")
    pgen = ParserGen(module, [".", "../hilti/libhilti", "libbinpac"])
    pgen.compile(g2)

    hilti.printer.printAST(module)
