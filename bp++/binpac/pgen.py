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
        
        tyr: ~~ParseableType - The type to generate a grammar for
        
        hilti_import_paths: list of string - Paths to search for HILTI modules.
        """
        self._cg = cg
        self._mbuilder = cg.moduleBuilder()
        
        self._builders = {}
        self._rhs_names = {}
        
        self._grammar = ty.grammar()
        self._type = ty
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
        
        def _doCompile():
            self._cg.beginCompile(self)
            self._functionHostApplication()
            self._cg.endCompile()
            return True
    
        self._mbuilder.cache(self._grammar.name(), _doCompile)
        
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
        def __init__(self, fbuilder=None, args=None):
            def _makeArg(arg):
                return arg if isinstance(arg, hilti.operand.Operand) else fbuilder.idOp(arg)
            
            self.cur = _makeArg(args[0])
            self.obj = _makeArg(args[1])
            self.lahead = _makeArg(args[2])
            self.lahstart = _makeArg(args[3])
            self.flags = _makeArg(args[4])
                
        def tupleOp(self, addl=None):
            elems = [self.cur, self.obj, self.lahead, self.lahstart, self.flags] + (addl if addl else [])
            const = hilti.constant.Constant(elems, hilti.type.Tuple([e.type() for e in elems]))
            return hilti.operand.Constant(const)

    ### Methods generating parsing functions. 

    def _functionInit(self):
        """Generates the init function registering the parser with the BinPAC
        runtime."""
        grammar = self._grammar
        
        ftype = hilti.type.Function([], hilti.type.Void())
        name = self._name("init")
        (fbuilder, builder) = self.cg().beginFunction(name, ftype)
        fbuilder.function().id().setLinkage(hilti.id.Linkage.INIT)

        parser_type = fbuilder.typeByID("BinPACIntern::Parser")
        parser = builder.addLocal("parser", parser_type)
        funcs = builder.addLocal("funcs", hilti.type.Tuple([hilti.type.CAddr()] * 2))
        f = builder.addLocal("f", hilti.type.CAddr())

        builder.new(parser, builder.typeOp(parser_type.refType()))
        
        builder.caddr_function(funcs, builder.idOp(self._name("parse")))
        
        builder.struct_set(parser, builder.constOp("name"), builder.constOp(grammar.name()))
        builder.struct_set(parser, builder.constOp("description"), builder.constOp("(No description)"))
        builder.tuple_index(f, funcs, builder.constOp(0))
        builder.struct_set(parser, builder.constOp("parse_func"), f)
        builder.tuple_index(f, funcs, builder.constOp(1))
        builder.struct_set(parser, builder.constOp("resume_func"), f)
        builder.call(None, builder.idOp("BinPACIntern::register_parser"), builder.tupleOp([builder.idOp("parser")]))
        builder.return_void()

        self.cg().endFunction()
        
    def _functionHostApplication(self):
        """Generates the C function visible to the host application for
        parsing the given grammar. See ``binpac.h`` for the exact semantics of
        the generated function.
        
        Todo: Passing in additional parameters from C hasn't been tested
        yet. 
        """

        grammar = self._grammar
        
        cur = hilti.id.Parameter("__cur", _BytesIterType)
        flags = hilti.id.Parameter("__flags", _FlagsType)
        result = self._typeParseObjectRef()

        args = [cur, flags]
        
        for p in self._grammar.params():
            args += [hilti.id.Parameter(p.name(), p.type().hiltiType(self._cg))]
        
        ftype = hilti.type.Function(args, result)
        name = self._name("parse")
        
        (fbuilder, builder) = self.cg().beginFunction(name, ftype)
        
        fbuilder.function().id().setLinkage(hilti.id.Linkage.EXPORTED)
        
        pobj = builder.addLocal("__pobj", self._typeParseObjectRef())
        presult = builder.addLocal("__presult", _ParseFunctionResultType)
        lahead = builder.addLocal("__lahead", _LookAheadType)
        lahstart = builder.addLocal("__lahstart", _BytesIterType)
        
        builder.assign(lahead, _LookAheadNone)
        
        params = []
        for p in self._grammar.params():
            params += [builder.idOp(p.name())]

        self._newParseObject(pobj, params)
        
        args = ParserGen._Args(fbuilder, ["__cur", "__pobj", "__lahead", "__lahstart", "__flags"])
        self._parseStartSymbol(args)
        
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
            self._startingProduction(args.obj, prod)            
            self._finishedProduction(args.obj, prod, None)
            pass
        
        elif isinstance(prod, grammar.Sequence):
            self._parseSequence(prod, args)

        elif isinstance(prod, grammar.LookAhead):
            self._parseLookAhead(prod, args)
            
        elif isinstance(prod, grammar.Boolean):
            self._parseBoolean(prod, args)
            
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
        
        self._startingProduction(args.obj, lit)
        
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
        (default, values, branches) = self._addMatchTokenErrorCases(builder, values, branches, no_lahead, [lit])
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

        # Run the value through any potential filter function. 
        filter = lit.filter()
        if filter:
            token = operator.evaluate(operator.Operator.Call, self.cg(), [filter, [expr.Hilti(token, lit.parsedType())]])
        
        self._finishedProduction(args.obj, lit, token)
        
    def _parseVariable(self, var, args):
        """Generates code to parse a variable."""
        type = var.type()
        
        # Do we actually need the parsed value? We do if (1) we're storing it
        # in the destination struct, or (2) we a hook that has a '$$'
        # parameter.
        need_val = (var.name() != None) or var.forEachHook()
        
        self.builder().setNextComment("Parsing variable %s" % var)

        self._startingProduction(args.obj, var)
        
        builder = self.builder()
        
        # We must not have a pending look-ahead symbol at this point. 
        cond = builder.addTmp("__cond", hilti.type.Bool())
        builder.equal(cond, args.lahead, _LookAheadNone)
        builder.debug_assert(cond)

        # Call the type's parse function.
        name = var.name() if var.name() else "__tmp"
        dst = self.builder().addTmp(name , var.parsedType().hiltiType(self.cg()))
        args.cur = var.parsedType().generateParser(self.cg(), args.cur, dst, not need_val)
        
        # Run the value through any potential filter function. 
        filter = var.filter()
        if filter:
            dst = operator.evaluate(operator.Operator.Call, self.cg(), [filter, [expr.Hilti(dst, var.parsedType())]])
        
        # We have successfully parsed a rule. 
        self._finishedProduction(args.obj, var, dst if need_val else None)

    def _parseChildGrammar(self, child, args):
        """Generates code to parse another type represented by its own grammar."""
        
        utype = child.type()

        self.builder().setNextComment("Parsing child grammar %s" % child.type().name())

        self._startingProduction(args.obj, child)
        
        builder = self.builder()
        
        # Generate the parsing code for the child grammar. 
        cpgen = ParserGen(self._cg, child.type())
        cpgen.compile()

        # Call the parsing code. 
        result = builder.addTmp("__presult", _ParseFunctionResultType)
        cobj = builder.addTmp("__cobj_%s" % utype.name(), utype.hiltiType(self._cg))

        cargs = ParserGen._Args(self.functionBuilder(), (args.cur, cobj, args.lahead, args.lahstart, args.flags))
        params = [p.evaluate(self._cg) for p in child.params()]
        
        cpgen._newParseObject(cobj, params)
        
        cpgen._parseStartSymbol(cargs)

        if child.name():
            builder.struct_set(args.obj, builder.constOp(child.name()), cobj)

        builder.assign(args.cur, cargs.cur)
        builder.assign(args.lahead, cargs.lahead)
        builder.assign(args.lahstart, cargs.lahstart)
            
        self._finishedProduction(args.obj, child, cobj)

    def _parseSequence(self, prod, args):
        def _makeFunction():
            (func, args) = self._createParseFunction("sequence", prod)
            
            self.builder().setNextComment("Parse function for production '%s'" % prod)
            
            self._startingProduction(args.obj, prod)
            
            # Initialize cache entry already here so that we can work
            # recursively.
            self.moduleBuilder().setCacheEntry(prod.symbol(), func)
            
            for p in prod.sequence():
                self._parseProduction(p, args)
                
            self._finishedProduction(args.obj, prod, None)
            
            builder = self.builder()
            builder.return_result(builder.tupleOp([args.cur, args.lahead, args.lahstart]))
            
            self.cg().endFunction()
            
            return func
        
        func = self.moduleBuilder().cache(prod.symbol(), _makeFunction)
        
        self.builder().setNextComment("Parsing non-terminal %s" % prod.symbol())

        self._startingProduction(args.obj, prod)
        
        builder = self.builder()
         
        result = builder.addTmp("__presult", _ParseFunctionResultType)

        builder.call(result, builder.idOp(func.name()), args.tupleOp([]))
        builder.tuple_index(args.cur, result, builder.constOp(0))
        builder.tuple_index(args.lahead, result, builder.constOp(1))
        builder.tuple_index(args.lahstart, result, builder.constOp(2))

        self._finishedProduction(args.obj, prod, None)
        
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

            ### Now branch according the look-ahead.
            done = self.functionBuilder().newBuilder("done")
            
            # Built the cases.
            def _makeBranch(i, alts, literals):
                branch = self.functionBuilder().newBuilder("case-%d" % i)
                branch.setComment("For look-ahead set {%s}" % ", ".join(['"%s"' % l.literal().value() for l in literals[i]]))
                branch.tuple_index(args.cur, match, builder.constOp(1)) # Update current position.
                self.cg().setBuilder(branch)
                self._startingProduction(args.obj, alts[i])
                self._parseProduction(alts[i], args)
                self._finishedProduction(args.obj, alts[i], None)
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
                    
            (default, values, branches) = self._addMatchTokenErrorCases(builder, values, branches, no_lahead, expected)
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
            self._startingProduction(args.obj, alts[i])
            self._parseProduction(alts[i], args)
            self._finishedProduction(args.obj, alts[i], None)
            self.builder().jump(done.labelOp())
            
        save_builder.if_else(bool, branches[0].labelOp(), branches[1].labelOp())
        
        self.cg().setBuilder(done)

    def _parseSwitch(self, prod, args):
        cg = self.cg()
        self.builder().setNextComment("Parsing switch %s" % prod.symbol())

        prods = [p for (e, p) in prod.cases()]
        
        expr = prod.expr().evaluate(cg)
        
        dsttype = prod.expr().type()
        
        values = [e.coerceTo(dsttype, cg).evaluate(cg) for (e, p) in prod.cases()]

        (default_builder, case_builders, done) = cg.builder().makeSwitch(expr, values);
        
        for (case_prod, builder) in zip(prods, case_builders):
            cg.setBuilder(builder)
            self._startingProduction(args.obj, case_prod)
            self._parseProduction(case_prod, args)
            self._finishedProduction(args.obj, case_prod, None)
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
        
    def _yieldAndTryAgain(self, builder, cont):
        """Generates code that yields and then jumps to a previous block to
        repeat whatever it was doing."""
        builder.yield_()
        builder.jump(cont.labelOp())

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
        
    def _startingProduction(self, obj, prod):
        """Called whenever a production is about to be parsed."""
        if not prod.name():
            return
        
        default = prod.type().hiltiDefault(self.cg(), False)
        
        if not default:
            return 
        
        # Initalize the struct field with its default value if not already set.
        not_set = self.cg().functionBuilder().addTmp("__not_set", hilti.type.Bool())
        name = self.cg().builder().constOp(prod.name())
        self.cg().builder().struct_is_set(not_set, obj, name)
        self.cg().builder().bool_not(not_set, not_set)
        (set, cont) = self.cg().builder().makeIf(not_set)
        self.cg().setBuilder(set)
        self.cg().builder().struct_set(obj, name, default)
        self.cg().builder().jump(cont.labelOp())
        self.cg().setBuilder(cont)
        
    def _finishedProduction(self, obj, prod, value):
        """Called whenever a production has sucessfully parsed value."""
        
        builder = self.builder()
        fbuilder = self.functionBuilder()
        
        if isinstance(prod, grammar.Terminal):
            
            if value:
                if prod.name():
                    builder.makeDebugMsg("binpac", "%s = '%%s'" % prod.name(), [value])
                builder.makeDebugMsg("binpac-verbose", "- matched '%s' to '%%s'" % prod, [value])
            else:
                builder.makeDebugMsg("binpac-verbose", "- matched '%s'" % prod)
                
        if prod.name() and value:
            builder.struct_set(obj, builder.constOp(prod.name()), value)

        foreach = prod.forEachField()
            
        if foreach and value:
            # Foreach field hook.
            result = fbuilder.addTmp("__hookrc", hilti.type.Bool(), builder.constOp(True))
            self.cg().runFieldHook(foreach, obj, value, result=result)
            (true, cont) = self.builder().makeIf(result)
            self.cg().setBuilder(true)
            
        else:
            cont = None
            
        if prod.field():
            # Additional hook with extended name.
            import pactypes.unit
            if isinstance(prod.field(), pactypes.unit.SwitchFieldCase):
                self.cg().runFieldHook(prod.field(), obj, addl=prod.field().caseNumber())
            
            # Standard field hook.
            self.cg().runFieldHook(prod.field(), obj)

        if cont:
            true.jump(cont.labelOp())
            self.cg().setBuilder(cont)
            
    ### Methods defining types. 
    
    def _typeParseObject(self):
        """Returns the struct type for the parsed grammar."""
        
        def _makeType():
            fields = self._grammar.scope().IDs()
            ids = []

            for f in fields:
                default = f.type().attributeExpr("default")
                if default:
                    hlt_default = default.hiltiInit(self.cg())
                else:
                    hlt_default = f.type().hiltiUnitDefault(self.cg())
                #   hlt_default = f.type().hiltiDefault(self.cg(), True)
                
                ids += [(hilti.id.Local(f.name(), f.type().hiltiType(self._cg)), hlt_default)]
                
            for p in self._grammar.params():
                ids += [(hilti.id.Local("__param_%s" % p.name(), p.type().hiltiType(self._cg)), None)]
                
            structty = hilti.type.Struct(ids)
            self._mbuilder.addTypeDecl(self._name("object"), structty)
            return structty
        
        return self._mbuilder.cache("struct_%s" % self._grammar.name(), _makeType)

    def _typeParseObjectRef(self):
        """Returns a reference to the struct type for the parsed grammar."""
        #return hilti.type.Reference(self._typeParseObject())
        return hilti.type.Reference(hilti.type.Unknown(self._name("object")))

    def _runUnitHook(self, obj, hook):
        builder = self.cg().builder()
        
        op1 = self.cg().declareHook(self._type, hook, self.objectType())
        op2 = builder.tupleOp([obj])
        
        builder.hook_run(None, op1, op2)
    
    def _newParseObject(self, obj, params):
        """Allocates and initializes a struct type for the parsed grammar.
        
        obj: hilti.operand.Operand - The operand to the new object in.
        """
        builder = self.cg().builder()
        self.builder().new(obj, self.builder().typeOp(self._typeParseObject()))
        
        for (f, p) in zip(self._grammar.params(), params):
            field = self.builder().constOp("__param_%s" % f.name())
            self.builder().struct_set(obj, field, p)
            
        self._runUnitHook(obj, "%ctor")
        
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
        return (fbuilder.function(), ParserGen._Args(fbuilder, args))
        
    def _addMatchTokenErrorCases(self, builder, values, branches, repeat, expected_literals):
        """Build the standard error branches for the switch-statement
        following a ``match_token`` instruction."""
        
        fbuilder = builder.functionBuilder()

        expected = ",".join([str(e) for e in expected_literals])
        
        # Not found.
        values += [fbuilder.constOp(0)]
        branches += [fbuilder.cacheBuilder("not-found", lambda b: self._parseError(b, "look-ahead symbol(s) [%s] not found" % expected))]
        
        # Not enough input.
        values += [fbuilder.constOp(-1)]
        branches += [fbuilder.cacheBuilder("need-input", lambda b: self._yieldAndTryAgain(b, repeat))]
        
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
