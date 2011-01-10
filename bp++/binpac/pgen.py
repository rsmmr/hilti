# $Id$
#
# Turns a grammar into a recursive descent HILTI parser.

builtin_id = id

import hilti.printer

import grammar
import id
import stmt
import operator
import expr
import type

import binpac.util as util

_BytesIterType = hilti.type.IteratorBytes(hilti.type.Bytes())
_LookAheadType = hilti.type.Integer(32)
_LookAheadNone = hilti.operand.Constant(hilti.constant.Constant(0, _LookAheadType))
_FlagsType     = hilti.type.Integer(8) # Note: flags are currently unused.
_ParseFunctionResultType = hilti.type.Tuple([_BytesIterType, _LookAheadType, _BytesIterType])

def _flattenToStr(list):
    return " ".join([str(e) for e in list])

class ParserGen:
    def __init__(self, cg, ty):
        """Generates a parser for a parseable type.

        cg: ~~CodeGen - The code generator to use.

        ty: ~~ParseableType - The type to generate a grammar for.

        hilti_import_paths: list of string - Paths to search for HILTI modules.
        """
        self._cg = cg
        self._mbuilder = cg.moduleBuilder()

        self._builders = {}
        self._rhs_names = {}

        self._grammar = ty.grammar()
        self._type = ty
        self._parser = None
        ty._pgen = self

    def export(self):
        """Generates code for using the compiled parser from external C
        functions. The functions generates an appropiate intialization
        function that registers the parser with the BinPAC++ run-time library.

        grammar: Grammar - The grammar to generate the parser from.  The
        grammar must pass ~~Grammar.check.
        """

        def _doCompile():
            self._functionInit()
            return True

        self._mbuilder.cache(self._grammar.name() + "$export", _doCompile)

    def compile(self):
        """Adds the parser for a grammar to the current HILTI module.

        grammar: Grammar - The grammar to generate the parser from.  The
        grammar must pass ~~Grammar.check.
        """

        idx = self._grammar.name()

        def _doCompile():
            self._mbuilder.setCacheEntry(idx, True)
            self._cg.beginCompile(self)
            self._functionHostApplication(False)
            self._functionHostApplication(True)
            self._hiltiFunctionNew()
            self._cg.endCompile()
            return True

        self._mbuilder.cache(idx, _doCompile)

    def objectType(self):
        """Returns the type of the destination struct generated for a grammar.

        grammar: Grammar - The grammar to return the type for.  The grammar
        must pass ~~Grammar.check.

        Returns: hilti.Type.Reference - A reference type for
        struct generated for parsing this grammar.
        """
        return self._typeParseObjectRef()

    def cg(self):
        """Returns the current code generator.

        Returns: ~~CodeGen - The code generator.
        """
        return self._cg

    def builder(self):
        """Returns the current block builder. This is a convenience function
        just forwarding to ~~CodeGen.builder.

        Returns: ~~hilti.builder.BlockBuilder - The current builder.
        """
        return self._cg.builder()

    def functionBuilder(self):
        """Returns the current function builder. This is a convenience
        function just forwarding to ~~CodeGen.functionBuilder.

        Returns: ~~hilti.builder.FunctionBuilder - The current builder.
        """
        return self._cg.functionBuilder()

    def moduleBuilder(self):
        """Returns the current module builder. This is a convenience function
        just forwarding to ~~CodeGen.moduleBuilder.

        Returns: ~~hilti.builder.ModuleBuilder - The current builder.
        """
        return self._cg.moduleBuilder()

    ### Internal class to represent arguments of generated HILTI parsing function.

    class _Args:
        def __init__(self, fbuilder, exported, args):
            def _makeArg(arg):
                return arg if isinstance(arg, hilti.operand.Operand) else fbuilder.idOp(arg)

            self.cur = _makeArg(args[0])
            self.obj = _makeArg(args[1])
            self.lahead = _makeArg(args[2])
            self.lahstart = _makeArg(args[3])
            self.flags = _makeArg(args[4])

            self._exported = exported

        def tupleOp(self, addl=None):
            elems = [self.cur, self.obj, self.lahead, self.lahstart, self.flags] + (addl if addl else [])
            const = hilti.constant.Constant(elems, hilti.type.Tuple([e.type() for e in elems]))
            return hilti.operand.Constant(const)

        def exported(self):
            """Returns True if the parsing object this belongs two
            is exported."""
            return self._exported

    ### Methods generating parsing functions.

    def _globalParserObject(self):
        if not self._parser:
            name = "__binpac_parser_%s" % self._grammar.name()
            self._parser = self.cg().moduleBuilder().addGlobal(name, self.cg().functionBuilder().typeByID("BinPAC::Parser"), None)

        return self._parser

    def _functionInit(self):
        """Generates the init function registering the parser with the BinPAC
        runtime."""
        grammar = self._grammar

        ftype = hilti.type.Function([], hilti.type.Void())
        name = self._name("init")
        (fbuilder, builder) = self.cg().beginFunction(name, ftype)
        fbuilder.function().id().setLinkage(hilti.id.Linkage.INIT)

        parser = self._globalParserObject()
        funcs = builder.addLocal("funcs", hilti.type.Tuple([hilti.type.CAddr()] * 2))
        f = builder.addLocal("f", hilti.type.CAddr())

        mime_types = [hilti.operand.Constant(hilti.constant.Constant(t.value(), hilti.type.String())) for t in self._type.property("mimetype")]
        mime_types = hilti.operand.Ctor(mime_types, hilti.type.Reference(hilti.type.List(hilti.type.String())))

        builder.new(parser, builder.typeOp(parser.type().refType()))

        builder.struct_set(parser, builder.constOp("name"), builder.constOp(grammar.name()))
        builder.struct_set(parser, builder.constOp("description"), builder.constOp("(No description)"))
        builder.struct_set(parser, builder.constOp("mime_types"), mime_types)

        builder.caddr_function(funcs, builder.idOp(self._name("parse")))
        builder.tuple_index(f, funcs, builder.constOp(0))
        builder.struct_set(parser, builder.constOp("parse_func"), f)
        builder.tuple_index(f, funcs, builder.constOp(1))
        builder.struct_set(parser, builder.constOp("resume_func"), f)

        builder.caddr_function(funcs, builder.idOp(self._name("parse_sink")))
        builder.tuple_index(f, funcs, builder.constOp(0))
        builder.struct_set(parser, builder.constOp("parse_func_sink"), f)
        builder.tuple_index(f, funcs, builder.constOp(1))
        builder.struct_set(parser, builder.constOp("resume_func_sink"), f)

        # Set the new_func field if our parser does not receive further
        # parameters.
        if not self._grammar.params():
            name = self._type.nameFunctionNew()
            fid = hilti.operand.ID(hilti.id.Unknown(name, self.cg().moduleBuilder().module().scope()))
            builder.caddr_function(funcs, fid)
            builder.tuple_index(f, funcs, builder.constOp(0))
            builder.struct_set(parser, builder.constOp("new_func"), f)

        else:
            # Set the new_func field to null.
            null = builder.addLocal("null", hilti.type.CAddr())
            builder.struct_set(parser, builder.constOp("new_func"), null)

        builder.call(None, builder.idOp("BinPAC::register_parser"), builder.tupleOp([parser]))
        builder.return_void()
        self.cg().endFunction()

    def _functionHostApplication(self, sink):
        """Generates the C function visible to the host application for
        parsing the given grammar. See ``binpac.h`` for the exact semantics of
        the generated function.

        sink: bool - If True, we generate the internal
        ``parse_sink`` function, otherwise the public standard
        function.

        Todo: Passing in additional parameters from C hasn't been tested
        yet.
        """
        grammar = self._grammar

        cur = hilti.id.Parameter("__cur", _BytesIterType)
        flags = hilti.id.Parameter("__flags", _FlagsType)
        result = self._typeParseObjectRef()

        args = [cur, flags]

        if sink:
            pobj = hilti.id.Parameter("__pobj", self._typeParseObjectRef())
            args = [pobj] + args

        else:
            for p in self._grammar.params():
                args += [hilti.id.Parameter(p.name(), p.type().hiltiType(self._cg))]

        ftype = hilti.type.Function(args, result)
        name = self._name("parse")
        if sink:
            name += "_sink"

        (fbuilder, builder) = self.cg().beginFunction(name, ftype)

        fbuilder.function().id().setLinkage(hilti.id.Linkage.EXPORTED)

        if not sink:
            pobj = builder.addLocal("__pobj", self._typeParseObjectRef())
        else:
            pobj = builder.idOp("__pobj")

        presult = builder.addLocal("__presult", _ParseFunctionResultType)
        lahead = builder.addLocal("__lahead", _LookAheadType)
        lahstart = builder.addLocal("__lahstart", _BytesIterType)
        builder.assign(lahead, _LookAheadNone)

        args = ParserGen._Args(fbuilder, self._type.exported(), ["__cur", "__pobj", "__lahead", "__lahstart", "__flags"])

        if not sink:
            params = []
            for p in self._grammar.params():
                params += [self.cg().builder().idOp(p.name())]

            self.cg().builder().assign(pobj, self._allocateParseObject(params))

        self._prepareParseObject(args)
        self._parseStartSymbol(args)
        self._doneParseObject(args)

        self.builder().return_result(pobj)

        self.cg().endFunction()

    ### Methods generating parsing code for Productions.

    def _parseProduction(self, prod, args):
        pname = prod.__class__.__name__.lower()

        self.builder().makeDebugMsg("binpac-verbose", "bgn %s '%s'" % (pname, prod))

        self._debugShowInput("input", args.cur)
        self._debugShowToken("lahead symbol", args.lahead)
        self._debugShowInput("lahead start", args.lahstart)

        if isinstance(prod, grammar.Literal):
            self._parseLiteral(prod, args)

        elif isinstance(prod, grammar.Variable):
            self._parseVariable(prod, args)

        elif isinstance(prod, grammar.ChildGrammar):
            self._parseChildGrammar(prod, args)

        elif isinstance(prod, grammar.Epsilon):
            # Nothing else to do.
            self._startingProduction(args, prod)
            self._finishedProduction(args, prod, None)
            pass

        elif isinstance(prod, grammar.Sequence):
            self._parseSequence(prod, args)

        elif isinstance(prod, grammar.LookAhead):
            self._parseLookAhead(prod, args)

        elif isinstance(prod, grammar.Boolean):
            self._parseBoolean(prod, args)

        elif isinstance(prod, grammar.Counter):
            self._parseCounter(prod, args)

        elif isinstance(prod, grammar.Switch):
            self._parseSwitch(prod, args)

        else:
            util.internal_error("unexpected non-terminal type %s" % repr(prod))

        self.builder().makeDebugMsg("binpac-verbose", "end %s '%s'" % (pname, prod))

    def _parseStartSymbol(self, args):
        prod = self._grammar.startSymbol()
        pname = prod.__class__.__name__.lower()

        # The start symbol is always a sequence.
        assert isinstance(prod, grammar.Sequence)

        builder = self.builder()
        builder.makeDebugMsg("binpac-verbose", "bgn start-sym %s '%s' with flags %%d" % (pname, prod), [args.flags])

        builder.makeDebugMsg("binpac", "%s" % prod.type().name())
        builder.debug_push_indent()

        self._parseSequence(prod, args)

        builder = self.builder()
        builder.makeDebugMsg("binpac-verbose", "end start-sym %s '%s'" % (pname, prod))
        builder.debug_pop_indent()

    def _parseLiteral(self, lit, args):
        """Generates code to parse a literal."""
        self.builder().setNextComment("Parsing literal '%s'" % lit.literal().value())

        self._startingProduction(args, lit)

        builder = self.builder()

        # See if we have a look-ahead symbol.
        cond = builder.addTmp("__cond", hilti.type.Bool())
        cond = builder.equal(cond, args.lahead, _LookAheadNone)
        (no_lahead, have_lahead, done) = builder.makeIfElse(cond, tag="no-lahead")

        # If we do not have a look-ahead symbol pending, search for our literal.
        match = self._matchToken(no_lahead, "literal", str(lit.id()), [lit], args)
        symbol = builder.addTmp("__lahead", hilti.type.Integer(32))
        no_lahead.tuple_index(symbol, match, no_lahead.constOp(0))

        found_lit = self.functionBuilder().newBuilder("found-sym")
        found_lit.tuple_index(args.cur, match, found_lit.constOp(1))
        found_lit.jump(done.labelOp())

        values = [no_lahead.constOp(lit.id())]
        branches = [found_lit]
        (default, values, branches) = self._addMatchTokenErrorCases(lit, builder, args, values, branches, no_lahead, [lit])
        no_lahead.makeSwitch(symbol, values, default=default, branches=branches, cont=done, tag="next-sym")

        # If we have a look-ahead symbol, its value must match what we expect.
        have_lahead.setComment("Look-ahead symbol pending, check.")
        have_lahead.equal(cond, have_lahead.constOp(lit.id()), args.lahead)
        wrong_lahead = self.functionBuilder().cacheBuilder("wrong-lahead", lambda b: self._parseError(b, "unexpected look-ahead symbol pending"))
        (match, _, _) = have_lahead.makeIfElse(cond, no=wrong_lahead, cont=done, tag="check-lahead")

        # If it matches, consume it (i.e., clear the look-ahead).
        match.setComment("Correct look-ahead symbol pending, will be consumed.")
        match.jump(done.labelOp())

        # Consume look-ahead.
        done.assign(args.lahead, _LookAheadNone)

        # Extract token value.
        token = builder.addTmp("__token", hilti.type.Reference(hilti.type.Bytes()))
        done.bytes_sub(token, args.lahstart, args.cur)

        self.cg().setBuilder(done)

        token = self._runFilter(lit, token)

        self._finishedProduction(args, lit, token)

    def _parseVariable(self, var, args):
        """Generates code to parse a variable."""
        # Do we actually need the parsed value? We do if (1) we're storing it
        # in the destination struct, or (2) we a hook that has a '$$'
        # parameter; or (3) it's a bitfield.
        need_val = (
            var.name() != None or \
            (isinstance(var.type(), type.UnsignedInteger) and var.type().bits())
            )

        self.builder().setNextComment("Parsing variable %s" % var)

        self._startingProduction(args, var)

        builder = self.builder()

        # We must not have a pending look-ahead symbol at this point.
        cond = builder.addTmp("__cond", hilti.type.Bool())
        builder.equal(cond, args.lahead, _LookAheadNone)
        builder.debug_assert(cond)

        # Call the type's parse function.
        name = var.name() if var.name() else "__tmp"
        dst = self.builder().addTmp(name , var.parsedType().hiltiType(self.cg()))
        args.cur = var.parsedType().generateParser(self.cg(), var, args, dst, not need_val)

        dst = self._runFilter(var, dst)

        # We have successfully parsed a rule.
        self._finishedProduction(args, var, dst if need_val else None)

    def _parseChildGrammar(self, child, args):
        """Generates code to parse another type represented by its own grammar."""

        utype = child.type()

        self.builder().setNextComment("Parsing child grammar %s" % child.type().name())

        self._startingProduction(args, child)

        builder = self.builder()

        # Generate the parsing code for the child grammar.
        cpgen = ParserGen(self._cg, child.type())
        cpgen.compile()

        # Call the parsing code.
        result = builder.addTmp("__presult", _ParseFunctionResultType)
        cobj = builder.addTmp("__cobj_%s" % utype.name(), utype.hiltiType(self._cg))

        parse_from = child.type().attributeExpr("parse")

        builder = self.builder()

        if not parse_from:
            cur = args.cur
            lahead = args.lahead
            lahstart = args.lahstart
        else:
            cur = parse_from.evaluate(self.cg())
            lahead = builder.addTmp("__tmp_lhead", _LookAheadType)
            lahstart = builder.addTmp("__tmp_lahstart", _BytesIterType)
            builder.assign(lahead, _LookAheadNone)

        cargs = ParserGen._Args(self.functionBuilder(), child.type().exported(), (cur, cobj, lahead, lahstart, args.flags))
        params = [p.evaluate(self._cg) for p in child.params()]

        self._cg.builder().assign(cargs.obj, cpgen._allocateParseObject(params))
        cpgen._prepareParseObject(cargs)
        cpgen._parseStartSymbol(cargs)

        if child.name():
            builder.struct_set(args.obj, builder.constOp(child.name()), cobj)

        if not parse_from:
            builder.assign(args.cur, cargs.cur)
            builder.assign(args.lahead, cargs.lahead)
            builder.assign(args.lahstart, cargs.lahstart)

        self._finishedProduction(args, child, cobj)

        cpgen._doneParseObject(cargs)

    def _parseSequence(self, prod, args):
        def _makeFunction():
            (func, args) = self._createParseFunction("sequence", prod)

            self.builder().setNextComment("Parse function for production '%s'" % prod)

            self._startingProduction(args, prod)

            # Initialize cache entry already here so that we can work
            # recursively.
            self.moduleBuilder().setCacheEntry(prod.symbol(), func)

            for p in prod.sequence():
                self._parseProduction(p, args)

            self._finishedProduction(args, prod, None)

            builder = self.builder()
            builder.return_result(builder.tupleOp([args.cur, args.lahead, args.lahstart]))

            self.cg().endFunction()

            return func

        func = self.moduleBuilder().cache(prod.symbol(), _makeFunction)

        self.builder().setNextComment("Parsing non-terminal %s" % prod.symbol())

        self._startingProduction(args, prod)

        builder = self.builder()

        result = builder.addTmp("__presult", _ParseFunctionResultType)

        builder.call(result, builder.idOp(func.name()), args.tupleOp([]))
        builder.tuple_index(args.cur, result, builder.constOp(0))
        builder.tuple_index(args.lahead, result, builder.constOp(1))
        builder.tuple_index(args.lahstart, result, builder.constOp(2))

        self._finishedProduction(args, prod, None)

    def _parseLookAhead(self, prod, args):

        def _makeFunction():
            (func, args) = self._createParseFunction("lahead", prod)

            builder = self.builder()
            builder.setNextComment("Parse function for production '%s'" % prod)

            # Initialize cache entry already here so that we can work
            # recursively.
            self.moduleBuilder().setCacheEntry(prod.symbol(), func)

            ### See if we have look-ahead symbol pending.
            cond = builder.addTmp("__cond", hilti.type.Bool())
            builder.equal(cond, args.lahead, _LookAheadNone)
            (no_lahead, builder) = builder.makeIf(cond, tag="no-lahead")

            ### If no, search for the next pattern.

            literals = prod.lookAheads()
            alts = prod.alternatives()

            # Build a regular expression for all the possible symbols.
            match = self._matchToken(no_lahead, "regexp", prod.symbol(), literals[0] | literals[1], args)
            no_lahead.tuple_index(args.lahead, match, builder.constOp(0))
            no_lahead.jump(builder.labelOp())

            ### Now branch according the look-ahead.
            done = self.functionBuilder().newBuilder("done")

            # Built the cases.
            def _makeBranch(i, alts, literals):
                branch = self.functionBuilder().newBuilder("case-%d" % i)
                branch.setComment("For look-ahead set {%s}" % ", ".join(['"%s"' % l.literal().value() for l in literals[i]]))
                branch.tuple_index(args.cur, match, builder.constOp(1)) # Update current position.
                self.cg().setBuilder(branch)
                self._startingProduction(args, alts[i])
                self._parseProduction(alts[i], args)
                self._finishedProduction(args, alts[i], None)
                self.builder().jump(done.labelOp())
                return branch

            values = []
            branches = []
            expected = []
            cases = [_makeBranch(0, alts, literals), _makeBranch(1, alts, literals)]

            for i in (0, 1):
                case = cases[i]
                for lit in literals[i]:
                    values += [builder.constOp(lit.id())]
                    branches += [case]
                    expected += [lit]

            (default, values, branches) = self._addMatchTokenErrorCases(prod, builder, args, values, branches, no_lahead, expected)
            builder.makeSwitch(args.lahead, values, default=default, branches=branches, cont=done, tag="lahead-next-sym")

            # Done, return the result.
            done.return_result(done.tupleOp([args.cur, args.lahead, args.lahstart]))

            self.cg().endFunction()
            return func

        func = self.moduleBuilder().cache(prod.symbol(), _makeFunction)

        builder = self.builder()
        builder.setNextComment("Parsing non-terminal %s" % prod.symbol())

        result = builder.addTmp("__presult", _ParseFunctionResultType)

        builder.call(result, builder.idOp(func.name()), args.tupleOp([]))
        builder.tuple_index(args.cur, result, builder.constOp(0))
        builder.tuple_index(args.lahead, result, builder.constOp(1))
        builder.tuple_index(args.lahstart, result, builder.constOp(2))

    def _parseBoolean(self, prod, args):
        builder = self.builder()
        builder.setNextComment("Parsing boolean %s" % prod.symbol())

        bool = prod.expr().evaluate(self.cg())

        save_builder = builder
        branches = [None, None]

        done = self.functionBuilder().newBuilder("done")

        for (i, tag) in ((0, "True"), (1, "False")):
            alts = prod.branches()
            branches[i] = self.functionBuilder().newBuilder("if-%s" % tag)
            branches[i].setComment("Branch for %s" % tag)
            self.cg().setBuilder(branches[i])
            self._startingProduction(args, alts[i])
            self._parseProduction(alts[i], args)
            self._finishedProduction(args, alts[i], None)
            self.builder().jump(done.labelOp())

        save_builder.if_else(bool, branches[0].labelOp(), branches[1].labelOp())

        self.cg().setBuilder(done)

    def _parseCounter(self, prod, args):
        builder = self.builder()
        builder.setNextComment("Parsing counter %s" % prod.symbol())

        limit = prod.expr().evaluate(self.cg())
        cnt = self.cg().builder().addLocal("__loop_counter", limit.type())
        finished = self.cg().builder().addLocal("__loop_cond", hilti.type.Bool())

        body = self.functionBuilder().newBuilder("loop_body")
        done = self.functionBuilder().newBuilder("loop_done")
        cond = self.functionBuilder().newBuilder("loop_cond")

        builder = self.builder()
        builder.assign(cnt, limit)
        builder.jump(cond.labelOp())

        # Condition block checking whether we're done.
        zero = cond.constOp(0, cnt.type())
        cond.equal(finished, cnt, zero)
        cond.if_else(finished, done.labelOp(), body.labelOp())

        # The body doing one iteration of the production.
        body.decr(cnt, cnt)

        self.cg().setBuilder(body)
        self._startingProduction(args, prod.body())
        self._parseProduction(prod.body(), args)
        self._finishedProduction(args, prod.body(), None)
        self.cg().builder().jump(cond.labelOp())

        # All done.
        self.cg().setBuilder(done)

    def _parseSwitch(self, prod, args):
        cg = self.cg()
        self.builder().setNextComment("Parsing switch %s" % prod.symbol())

        prods = [p for (e, p) in prod.cases()]

        expr = prod.expr().evaluate(cg)

        dsttype = prod.expr().type()

        values = [ [e.coerceTo(dsttype, cg).evaluate(cg) for e in exprs] for (exprs, p) in prod.cases()]

        (default_builder, case_builders, done) = cg.builder().makeSwitch(expr, values);

        for (case_prod, builder) in zip(prods, case_builders):
            cg.setBuilder(builder)
            self._startingProduction(args, case_prod)
            self._parseProduction(case_prod, args)
            self._finishedProduction(args, case_prod, None)
            self.builder().jump(done.labelOp())

        cg.setBuilder(default_builder)

        if prod.default():
            self._parseProduction(prod.default(), args)
        else:
            self._parseError(default_builder, "unexpected switch case")

        self.builder().jump(done.labelOp())

        self.cg().setBuilder(done)

    ### Methods generating helper code.

    def _parseError(self, builder, msg):
        """Generates code to raise an exception."""
        builder.makeRaiseException("BinPAC::ParseError", builder.constOp(msg))

    def _yieldAndTryAgain(self, prod, builder, args, cont):
        """Generates code that yields and then jumps to a previous block to
        repeat whatever it was doing."""
        old_builder = self.cg().builder()
        self.cg().setBuilder(builder)

        if not prod.eodOk():
            self.cg().generateInsufficientInputHandler(args)
            self.cg().builder().jump(cont.labelOp())
        else:
            eod = self.functionBuilder().newBuilder("eod_ok")
            eod.return_result(builder.tupleOp([args.cur, args.lahead, args.lahstart]))

            at_eod = self.cg().generateInsufficientInputHandler(args, eod_ok=True)
            self.builder().if_else(at_eod, eod.labelOp(), cont.labelOp())

        self.cg().setBuilder(old_builder)

    def _matchToken(self, builder, ntag1, ntag2, literals, args):
        """Generates standard code around a ``regexp.match`` token
        instruction.
        """
        fbuilder = builder.functionBuilder()

#        match_rtype = hilti.type.Tuple([hilti.type.Integer(32), args.cur.type()])
        match_rtype = hilti.type.Tuple([hilti.type.Integer(32), _BytesIterType])
        match = fbuilder.addTmp("__match", match_rtype)
        cond = fbuilder.addTmp("__cond", hilti.type.Bool())
        name = self._name(ntag1, ntag2)

        def _makePatternConstant():
            tokens = ["%s{#%d}" % (lit.literal().value(), lit.id()) for lit in literals]
            op = hilti.operand.Ctor([tokens, []], hilti.type.Reference(hilti.type.RegExp()))
            cty = hilti.type.Reference(hilti.type.RegExp(["&nosub"]))
            return fbuilder.moduleBuilder().addGlobal(name, cty, op)

        pattern = fbuilder.moduleBuilder().cache(name, _makePatternConstant)
        builder.assign(args.lahstart, args.cur) # Record starting position.

        builder.regexp_match_token(match, pattern, args.cur)
        return match

    def _debugShowInput(self, tag, cur):
        fbuilder = self.cg().functionBuilder()
        builder = self.cg().builder()

        next5 = fbuilder.addTmp("__next5", _BytesIterType)
        str = fbuilder.addTmp("__str", hilti.type.Reference(hilti.type.Bytes()))
        builder.incr_by(next5, cur, builder.constOp(5))
        builder.bytes_sub(str, cur, next5)

        msg = "- %s is |" % tag
        builder.makeDebugMsg("binpac-verbose", msg + "%s| ...", [str])

    def _debugShowToken(self, tag, token):
        fbuilder = self.cg().functionBuilder()
        builder = self.cg().builder()
        msg = "- %s is " % tag
        builder.makeDebugMsg("binpac-verbose", msg + "%d ...", [token])

    def _startingProduction(self, args, prod):
        """Called whenever a production is about to be parsed."""
        if not prod.name():
            return

        default = prod.type().hiltiDefault(self.cg(), False)

        if not default:
            return

        # Initalize the struct field with its default value if not already set.
        not_set = self.cg().functionBuilder().addTmp("__not_set", hilti.type.Bool())
        name = self.cg().builder().constOp(prod.name())
        self.cg().builder().struct_is_set(not_set, args.obj, name)
        self.cg().builder().bool_not(not_set, not_set)
        (set, cont) = self.cg().builder().makeIf(not_set)
        self.cg().setBuilder(set)
        self.cg().builder().struct_set(args.obj, name, default)
        self.cg().builder().jump(cont.labelOp())
        self.cg().setBuilder(cont)

    def _finishedProduction(self, args, prod, value):
        """Called whenever a production has sucessfully parsed a value."""

        builder = self.builder()
        fbuilder = self.functionBuilder()

        if isinstance(prod, grammar.Terminal):

            if value:
                if prod.debugName():
                    builder.makeDebugMsg("binpac", "%s = '%%s'" % prod.debugName(), [value])

                builder.makeDebugMsg("binpac-verbose", "- matched '%s' to '%%s'" % prod, [value])
            else:
                builder.makeDebugMsg("binpac-verbose", "- matched '%s'" % prod)

        if prod.name() and value:
            builder.struct_set(args.obj, builder.constOp(prod.name()), value)

        foreach = prod.forEachField()

        if foreach and value:
            # Foreach field hook.
            result = fbuilder.addTmp("__hookrc", hilti.type.Bool(), builder.constOp(True))
            self._runFieldHook(foreach, args, value, result=result)
            (true, cont) = self.builder().makeIf(result)
            self.cg().setBuilder(true)

        else:
            cont = None

        if prod.field():
            # Additional hook with extended name.
            import pactypes.unit
            if isinstance(prod.field(), pactypes.unit.SwitchFieldCase):
                self._runFieldHook(prod.field(), args, addl=prod.field().caseNumber())

            # Standard field hook.
            self._runFieldHook(prod.field(), args)

        if cont:
            true.jump(cont.labelOp())
            self.cg().setBuilder(cont)

    def _runFilter(self, prod, value):
        filter = prod.filter()

        if not filter:
            return value

        value = operator.evaluate(operator.Operator.Call, self.cg(), [filter, [expr.Hilti(value, prod.parsedType())]])

        return value

    ### Methods defining types.

    def _typeParseObject(self):
        """Returns the struct type for the parsed grammar."""

        def _makeType():
            fields = self._grammar.scope().IDs()
            ids = []

            # Unit fields.
            for f in fields:
                default = f.type().attributeExpr("default")
                if default:
                    hlt_default = default.hiltiInit(self.cg())
                else:
                    if isinstance(f, id.Variable):
                        hlt_default = f.type().hiltiDefault(self.cg(), True)
                    else:
                        hlt_default = f.type().hiltiUnitDefault(self.cg())

                ids += [(hilti.id.Local(f.name(), f.type().hiltiType(self._cg)), hlt_default)]

            # The unit parameters.
            for p in self._grammar.params():
                ids += [(hilti.id.Local("__param_%s" % p.name(), p.type().hiltiType(self._cg)), None)]

            # The input() position.
            ids += [(hilti.id.Local("__input", hilti.type.IteratorBytes()), None)]

            # For passing the set_input() position back,
            ids += [(hilti.id.Local("__cur", hilti.type.IteratorBytes()), None)]

            ## We need the following only if the type is exported.
            if self._type.exported():

                # The parser object.
                ids += [(hilti.id.Local("__parser", self._globalParserObject().type()), None)]

                # The sink this parser is connected to.
                ids += [(hilti.id.Local("__sink", self.cg().functionBuilder().typeByID("BinPAC::Sink")),
                                        hilti.operand.Constant(hilti.constant.Constant(None, hilti.type.Reference(None))))]

                # The MIME type associated with our input.
                ids += [(hilti.id.Local("__mimetype", hilti.type.Reference(hilti.type.Bytes())), None)]

                # The filter chain attached to this parser.
                ids += [(hilti.id.Local("__filter", self.cg().functionBuilder().typeByID("BinPAC::ParseFilter")), None)]

                # State for transparently filtering input.
                    # The bytes object storign the not-yet-filtered data.
                ids += [(hilti.id.Local("__filter_decoded", hilti.type.Reference(hilti.type.Bytes())), None)]
                    # The current position in the non-yet-filtered data (i.e.,
                    # in __filter_decoded)
                ids += [(hilti.id.Local("__filter_cur", hilti.type.IteratorBytes()), None)]

            structty = hilti.type.Struct(ids)
            self._mbuilder.addTypeDecl(self._name("object"), structty)
            return structty

        return self._mbuilder.cache("struct_%s" % self._grammar.name(), _makeType)

    def _typeParseObjectRef(self):
        """Returns a reference to the struct type for the parsed grammar."""
        #return hilti.type.Reference(self._typeParseObject())
        return hilti.type.Reference(hilti.type.Unknown(self._name("object")))

    def _saveInputPointer(self, args):
        # Store the current input pointer for access by a hook. TODO: This
        # pointer will rarely be needed so we should be able to optimize it away
        # in most cases.
        builder = self.cg().builder()
        builder.struct_set(args.obj, builder.constOp("__cur"), args.cur)

    def _updateInputPointer(self, args):
        # Update the current input pointer in case a hook has changed it. TODO:
        # This pointer will rarely be needed so we should be able to optimize it
        # away in most cases.
        builder = self.cg().builder()
        builder.struct_get(args.cur, args.obj, builder.constOp("__cur"))

    def _runUnitHook(self, args, hook):
        builder = self.cg().builder()

        op1 = self.cg().declareHook(self._type, hook, self.objectType())
        op2 = builder.tupleOp([args.obj])

        self._saveInputPointer(args)
        builder.hook_run(None, op1, op2)
        self._updateInputPointer(args)

    def _runFieldHook(self, field, args, value=None, result=None, addl=None):
        """Runs a hook associated with a unit field.

        field: ~~Field - The unit field.

        obj: ~~hilti.operand.Operand - An operand with the hook's ``self``
        argument.

        value: ~~hilti.operand.Operand - An operand wit the hook's ``$$``
        argument. Must be given if hooks expects such.

        result: ~~hilti.operand.Operand - An operand receiving the hook's
        result. Must be given if hook returns a value.

        addl: string - If given, an additional string postfix to be added to
        the generated hook name.
        """
        builder = self.cg().builder()
        name = self.cg().hookName(field.parent(), field, value != None, addl=addl)
        op1 = builder.idOp(hilti.id.Unknown(name, self.cg().moduleBuilder().module().scope()))

        pargs_proto = [(hilti.id.Parameter("__self", args.obj.type()), None)]

        if value:
            pargs = [args.obj, value]
            pargs_proto += [(hilti.id.Parameter("__dollardollar", value.type()), None)]
        else:
            pargs = [args.obj]

        self.cg()._mbuilder.declareHook(name, pargs_proto, result.type() if result else hilti.type.Void())

        self._saveInputPointer(args)
        builder.hook_run(result, op1, builder.tupleOp(pargs))
        self._updateInputPointer(args)

    def _allocateParseObject(self, params, mtype=None, sink=None):
        """Allocates and initializes a struct type for the parsed grammar.
        """
        builder = self.cg().builder()
        obj = self.builder().addLocal("pobj", self._typeParseObjectRef())
        self.builder().new(obj, self.builder().typeOp(self._typeParseObject()))

        for (f, p) in zip(self._grammar.params(), params):
            field = self.builder().constOp("__param_%s" % f.name())
            self.builder().struct_set(obj, field, p)

        # Create the sink.
        # TODO: We should problably come up with a generic mechanism to
        # initialize non-const fields with defaults.
        for var in self._type.variables():
            var = self._type.variable(var)
            if not isinstance(var.type(), type.Sink):
                continue

            cg = self.cg()
            new_sink = cg.builder().addLocal("__new_sink", var.type().hiltiType(cg))
            cfunc = cg.builder().idOp("BinPAC::sink_new")
            cargs = cg.builder().tupleOp([])
            cg.builder().call(new_sink, cfunc, cargs)

            cg.builder().struct_set(obj, cg.builder().constOp(var.name()), new_sink);

        if self._type.exported():
            builder = self.cg().builder()
            builder.struct_set(obj, builder.constOp("__parser"), self._globalParserObject())

            if mtype:
                builder.struct_set(obj, builder.constOp("__mimetype"), mtype)

            if sink:
                builder.struct_set(obj, builder.constOp("__sink"), sink)

        return obj

    def _prepareParseObject(self, args):
        """Prepares a parse object before starting the parsing."""
        # Record the data we're parsing for calls to unit.input().
        self.cg().builder().struct_set(args.obj, self.cg().builder().constOp("__input"), args.cur)
        self._runUnitHook(args, "%init")

        if args.exported():
            self.filterInput(args, resume=False)

    def _doneParseObject(self, args):
        cg = self.cg()

        self._runUnitHook(args, "%done")

        # Close the filter chain, if any.
        if args.exported():
            filter = cg.builder().addLocal("__filter", cg.functionBuilder().typeByID("BinPAC::ParseFilter"))
            null = hilti.operand.Constant(hilti.constant.Constant(None, filter.type()))
            cg.builder().struct_get_default(filter, args.obj, cg.builder().constOp("__filter"), null)
            cfunc = cg.builder().idOp("BinPAC::filter_close")
            cargs = cg.builder().tupleOp([filter])
            cg.builder().call(None, cfunc, cargs)
            cg.builder().struct_unset(args.obj, cg.builder().constOp("__filter"))

        # Close all sinks.
        # TODO: We should problably come up with a generic mechanism to
        # uninitialize non-const fields with defaults.
        for var in self._type.variables():
            var = self._type.variable(var)
            if not isinstance(var.type(), type.Sink):
                continue

            cg = self.cg()
            sink = cg.builder().addLocal("__sink", cg.functionBuilder().typeByID("BinPAC::Sink"))
            cg.builder().struct_get(sink, args.obj, cg.builder().constOp(var.name()));
            cfunc = cg.builder().idOp("BinPAC::sink_close")
            cargs = cg.builder().tupleOp([sink])
            cg.builder().call(None, cfunc, cargs)

            cg.builder().struct_unset(args.obj, cg.builder().constOp(var.name()));

        # Clear what's unit.input() is returning.
        cg.builder().struct_unset(args.obj, cg.builder().constOp("__input"))

        # Close filters if we have any.
        if args.exported():
            (filter, done) = self.ifFilter(args)

            cfunc = cg.builder().idOp("BinPAC::filter_close")
            cargs = cg.builder().tupleOp([filter])
            cg.builder().call(None, cfunc, cargs)
            cg.builder().jump(done.labelOp())

            cg.setBuilder(done)

    def _hiltiFunctionNew(self):
        """Creates a function that can be called from external to create a new
        instance of a parse object. In addition to the type parameters, the
        function receives two parameters: a ``hlt_sink *`` with the sink the
        parser is connected to (NULL if none); and a ``bytes`` object with the
        MIME type associated with the input (empty if None).
        """
        sink = hilti.id.Parameter("__sink", self.cg().moduleBuilder().typeByID("BinPAC::Sink"))
        mtype = hilti.id.Parameter("__mimetype", hilti.type.Reference(hilti.type.Bytes()))

        ty = self._type
        params = [hilti.id.Parameter("p%d" % i, arg.type().hiltiType(self.cg())) for (i, arg) in zip(range(len(ty.args())), ty.args())]
        params += [sink, mtype]
        ftype = hilti.type.Function(params, self._typeParseObjectRef())
        name = ty.nameFunctionNew()
        (fbuilder, builder) = self.cg().beginFunction(name, ftype)

        obj = self._allocateParseObject([hilti.operand.ID(p) for p in params], self.builder().idOp("__mimetype"), self.builder().idOp("__sink"))

        self.cg().builder().return_result(obj)
        self.cg().endFunction()

        fbuilder.function().id().setLinkage(hilti.id.Linkage.EXPORTED)

        ### FIXME: HILTI generates incorrect code for C_HILTI functions. For
        ### example, it doesn't access function parameters correctly.
        ### fbuilder.function().setCallingConvention(hilti.function.CallingConvention.C_HILTI)

    ### Filter management, plugging potential filters transparently into the
    ### parsing process.

    def ifFilter(self, args):
        """Helper to execute code only when a parsing object has a filter
        defined. When the function returns, a ~~Builder is set that
        corresponds to the case that a filter has been defined for *args.obj*.
        The caller can then add further code there, and when done jump to the
        returned *done* builder. 

        args: An ~~Args object with the current parsing arguments, as used by
        the ~~ParserGen.

        Returns: tuple (filter, done) - *filter* is an ~~hilti.Operand with
        the filter, if defined; the operand must only be accessed in the case
        that filter is defined (i.e., within the builder left active when the
        method returns). *done* is builder where to continue when the
        filter-specific has been finished; the caller needs to jump there
        eventually.
        """
        cg = self.cg()

        filter_set = cg.builder().addLocal("__filter_set", hilti.type.Bool())
        cg.builder().struct_is_set(filter_set, args.obj, cg.builder().constOp("__filter"))

        fbuilder = cg.functionBuilder()
        has_filter = fbuilder.newBuilder("__has_filter")
        done = fbuilder.newBuilder("__done_filter")

        cg.builder().if_else(filter_set, has_filter.labelOp(), done.labelOp())

        cg.setBuilder(has_filter)

        filter = cg.builder().addLocal("__filter", cg.functionBuilder().typeByID("BinPAC::ParseFilter"))
        cg.builder().struct_get(filter, args.obj, cg.builder().constOp("__filter"))

        return (filter, done)

    def filterInput(self, args, resume):
        """Helper to transparently pipe input through a chain of filters if a
        parsing object has defined any. It adjust the parsing arguments
        appropiately so that subsequent code doesn't notice the filtering. 

        args: An ~~Args object with the current parsing arguments, as used by
        the ~~ParserGen.

        resume: bool - True if this method is called after resuming parsing
        because of having insufficient inout earlier. False if it's the
        initial parsing step for a newly instantiated parser. 
        """
        cg = self.cg()

        (filter, done) = self.ifFilter(args)

        builder = cg.builder()

        if not resume:
            builder.setComment("Passing input through filter chain")

        encoded = builder.addLocal("__encoded", hilti.type.Reference(hilti.type.Bytes()))
        decoded = builder.addLocal("__decoded", hilti.type.Reference(hilti.type.Bytes()), reuse=True)
        filter_decoded = builder.addLocal("__filter_decoded", hilti.type.Reference(hilti.type.Bytes()))
        filter_cur = builder.addLocal("__filter_cur", hilti.type.IteratorBytes())
        begin = builder.addLocal("bgn", hilti.type.IteratorBytes())
        end = builder.addLocal("eob", hilti.type.IteratorBytes()) # We rely on this being intialized with end()
        len = builder.addLocal("len", hilti.type.Integer(64))
        is_frozen = cg.builder().addLocal("__is_frozen", hilti.type.Bool())

        if not resume:
            builder.bytes_sub(encoded, args.cur, end)
            cg.builder().bytes_is_frozen(is_frozen, args.cur)

        else:
            builder.struct_get(filter_cur, args.obj, builder.constOp("__filter_cur"))
            builder.bytes_sub(encoded, filter_cur, end)
            cg.builder().bytes_is_frozen(is_frozen, filter_cur)

        cfunc = cg.builder().idOp("BinPAC::filter_decode")
        cargs = cg.builder().tupleOp([filter, encoded])
        cg.builder().call(decoded, cfunc, cargs)

        if not resume:
            builder.struct_set(args.obj, builder.constOp("__filter_cur"), args.cur)

            builder.struct_set(args.obj, builder.constOp("__filter_decoded"), decoded)
            builder.begin(begin, decoded)
            builder.struct_set(args.obj, builder.constOp("__input"), begin)
            builder.struct_set(args.obj, builder.constOp("__cur"), begin) # TODO: Do we need this?
            builder.assign(args.cur, begin)

            filter_decoded = decoded
        else:
            builder.struct_get(filter_decoded, args.obj, builder.constOp("__filter_decoded"))
            builder.bytes_append(filter_decoded, decoded)

        builder.struct_get(filter_cur, args.obj, builder.constOp("__filter_cur"))
        builder.bytes_length(len, encoded)
        builder.incr_by(filter_cur, filter_cur, len)
        builder.struct_set(args.obj, builder.constOp("__filter_cur"), filter_cur)

        # Freeze the decoded data if the input data is frozen.
        freeze = cg.functionBuilder().newBuilder("_freeze")
        cg.builder().if_else(is_frozen, freeze.labelOp(), done.labelOp())

        freeze.bytes_freeze(filter_decoded);
        freeze.jump(done.labelOp())

        cg.setBuilder(done)

    ### Helper methods.

    def _name(self, tag1, tag2 = None, prefix=None):
        """Combines two tags to an canonicalized ID name."""
        name = "%s_%s" % (self._grammar.name(), tag1)

        name = name.replace("::", "_")

        if tag2:
            name += "_%s" % tag2

        if prefix:
            name = "%s_%s" % (prefix, name)

        # Makes sure the name contains only valid characters.
        chars = [c if (c.isalnum() or c in "_") else "_0x%x_" % ord(c) for c in name]
        return "".join(chars).lower()

    def _createParseFunction(self, prefix, prod):
        """Creates a HILTI function with the standard parse function
        signature."""

        arg1 = hilti.id.Parameter("__cur", _BytesIterType)
        arg2 = hilti.id.Parameter("__self", self._typeParseObjectRef())
        arg3 = hilti.id.Parameter("__lahead", _LookAheadType)
        arg4 = hilti.id.Parameter("__lahstart", _BytesIterType)
        arg5 = hilti.id.Parameter("__flags", _FlagsType)
        result = _ParseFunctionResultType

        args = [arg1, arg2, arg3, arg4, arg5]

        ftype = hilti.type.Function(args, result)
        name = self._name(prefix, prod.symbol())

        (fbuilder, builder) = self.cg().beginFunction(name, ftype)
        return (fbuilder.function(), ParserGen._Args(fbuilder, self._type.exported(), args))

    def _addMatchTokenErrorCases(self, prod, builder, args, values, branches, repeat, expected_literals):
        """Build the standard error branches for the switch-statement
        following a ``match_token`` instruction."""

        fbuilder = builder.functionBuilder()

        expected = ",".join([str(e) for e in expected_literals])

        # Not found.
        values += [fbuilder.constOp(0)]
        branches += [fbuilder.cacheBuilder("not-found", lambda b: self._parseError(b, "look-ahead symbol(s) [%s] not found" % expected))]

        # Not enough input.
        values += [fbuilder.constOp(-1)]
        branches += [fbuilder.cacheBuilder("need-input", lambda b: self._yieldAndTryAgain(prod, b, args, repeat))]

        # Unknown case.
        default = fbuilder.cacheBuilder("unexpected-sym", lambda b: b.makeInternalError("unexpected look-ahead symbol returned"))

        return (default, values, branches)

if __name__ == "__main__":
    from binpac.grammar import *
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

    module = hilti.module.Module("ParserTest")
    mbuilder = hilti.builder.ModuleBuilder(module)
    pgen = ParserGen(mbuilder, [".", "../../../hilti/libhilti", "../../libbinpac"])
    pgen.compile(g2)

    hilti.printer.printAST(module)
