# $Id$
#
# Turns a grammar into a recursive descent HILTI parser. 

builtin_id = id

import hilti.core
import hilti.printer

import grammar
import id
import stmt

import binpac.support.util as util

_BytesIterType = hilti.core.type.IteratorBytes(hilti.core.type.Bytes())
_LookAheadType = hilti.core.type.Integer(32)
_LookAheadNone = hilti.core.instruction.ConstOperand(hilti.core.constant.Constant(0, _LookAheadType))
_ParseFunctionResultType = hilti.core.type.Tuple([_BytesIterType, _LookAheadType, _BytesIterType])

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
        
        self._builders = {}
        self._rhs_names = {}
        
        self._current_grammar = None

    def export(self, grammar):
        """Generates code for using the compiled parser from external C
        functions. The functions generates an appropiate intialization
        function that registers the parser with the BinPAC++ run-time library.

        grammar: Grammar - The grammar to generate the parser from.  The
        grammar must pass ~~Grammar.check.
        """

        def _doCompile():
            self._current_grammar = grammar
            self._functionInit()
            return True
            
        self._mbuilder.cache(grammar.name() + "$export", _doCompile)
        
    def compile(self, grammar):
        """Adds the parser for a grammar to the HILTI module. 
        
        This method can be called multiple times to generate a set of parsers
        for different grammars, all within the same module.
        
        grammar: Grammar - The grammar to generate the parser from.  The
        grammar must pass ~~Grammar.check.
        """
        
        def _doCompile():
            self._current_grammar = grammar
            self._functionHostApplication()
            return True
            
        self._mbuilder.cache(grammar.name(), _doCompile)
        
    def objectType(self, grammar):
        """Returns the type of the destination struct generated for a grammar. 
        
        grammar: Grammar - The grammar to return the type for.  The grammar
        must pass ~~Grammar.check.
        
        Returns: hilti.core.Type.Reference - A reference type for 
        struct generated for parsing this grammar. 
        """
        self._current_grammar = grammar
        return self._typeParseObjectRef()
    
    def cg(self):
        """Returns the current code generator. 
        
        Returns: ~~CodeGen - The code generator.
        """
        return self._cg
    
    def builder(self):
        """Returns the current block builder. This is a convenience function
        just forwarding to ~~CodeGen.builder.
        
        Returns: ~~hilti.core.builder.BlockBuilder - The current builder.
        """
        return self._cg.builder()
    
    def functionBuilder(self):
        """Returns the current function builder. This is a convenience
        function just forwarding to ~~CodeGen.functionBuilder.
        
        Returns: ~~hilti.core.builder.FunctionBuilder - The current builder.
        """
        return self._cg.functionBuilder()
    
    def moduleBuilder(self):
        """Returns the current module builder. This is a convenience function
        just forwarding to ~~CodeGen.moduleBuilder.
        
        Returns: ~~hilti.core.builder.ModuleBuilder - The current builder.
        """
        return self._cg.moduleBuilder()
    
    ### Internal class to represent arguments of generated HILTI parsing function. 
    
    class _Args:
        def __init__(self, fbuilder=None, args=None):
            def _makeArg(arg):
                return arg if isinstance(arg, hilti.core.instruction.Operand) else fbuilder.idOp(arg)

            self.cur = _makeArg(args[0])
            self.obj = _makeArg(args[1])
            self.lahead = _makeArg(args[2])
            self.lahstart = _makeArg(args[3])
                
        def tupleOp(self, addl=[]):
            return hilti.core.instruction.TupleOperand([self.cur, self.obj, self.lahead, self.lahstart] + addl)
        
    ### Methods generating parsing functions. 

    def _functionInit(self):
        """Generates the init function registering the parser with the BinPAC
        runtime."""
        grammar = self._current_grammar
        
        ftype = hilti.core.type.Function([], hilti.core.type.Void())
        name = self._name("init")
        (fbuilder, builder) = self.cg().beginFunction(name, ftype)
        fbuilder.function().setLinkage(hilti.core.function.Linkage.INIT)

        parser_type = fbuilder.typeByID("BinPACIntern::Parser")
        parser = builder.addLocal("parser", parser_type)
        funcs = builder.addLocal("funcs", hilti.core.type.Tuple([hilti.core.type.CAddr()] * 2))
        f = builder.addLocal("f", hilti.core.type.CAddr())

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

        grammar = self._current_grammar
        
        cur = hilti.core.id.ID("__cur", _BytesIterType, hilti.core.id.Role.PARAM)
        result = self._typeParseObjectRef()

        args = [cur]
        
        for p in self._current_grammar.params():
            args += [hilti.core.id.ID(p.name(), p.type().hiltiType(self._cg), hilti.core.id.Role.PARAM)]
        
        ftype = hilti.core.type.Function(args, result)
        name = self._name("parse")
        
        (fbuilder, builder) = self.cg().beginFunction(name, ftype)
        
        fbuilder.function().setLinkage(hilti.core.function.Linkage.EXPORTED)
        
        pobj = builder.addLocal("__pobj", self._typeParseObjectRef())
        presult = builder.addLocal("__presult", _ParseFunctionResultType)
        lahead = builder.addLocal("__lahead", _LookAheadType)
        lahstart = builder.addLocal("__lahstart", _BytesIterType)
        
        builder.assign(lahead, _LookAheadNone)

        self._newParseObject(pobj)
        
        args = ParserGen._Args(fbuilder, ["__cur", "__pobj", "__lahead", "__lahstart"])
        self._parseStartSymbol(args, [])
        
        self.builder().return_result(pobj)

        self.cg().endFunction()

    def _functionHook(self, fname, hook):
        """Generates the function to execute a hook statement.
        
        Todo: We currently create "normal" functions; once HILTI's hook data
        type is implemented, we'll switch over to using that. 
        """
        
        def makeFunc():
            cg = self.cg()

            args = []
            args += [hilti.core.id.ID("__self", self._typeParseObjectRef(), hilti.core.id.Role.PARAM)]
            
            for p in self._current_grammar.params():
                args += [hilti.core.id.ID(p.name(), p.type().hiltiType(self._cg), hilti.core.id.Role.PARAM)]

            if isinstance(hook, stmt.FieldControlHook):
                args += [hilti.core.id.ID("__dollardollar", hook.dollarDollarType().hiltiType(self.cg()), hilti.core.id.Role.PARAM)]
                
            ftype = hilti.core.type.Function(args, hilti.core.type.Bool())
            
            name = self._name("hook", fname)
            
            (fbuilder, builder) = cg.beginFunction(name, ftype)        
            
            func = fbuilder.function()
            self.moduleBuilder().setCacheEntry(hook, func)
            
            rc = hilti.core.id.ID("__hookrc", hilti.core.type.Bool(), hilti.core.id.Role.LOCAL)
            oprc = cg.builder().idOp(rc)
            
            func.addID(rc)
            cg.builder().assign(oprc, builder.constOp(True))
            
            hook.execute(cg)
            
            cg.builder().return_result(oprc)
            
            cg.endFunction()
            return func
        
        return self.moduleBuilder().cache(hook, makeFunc())
        
    ### Methods generating parsing code for Productions.

    def _parseProduction(self, prod, args):
        pname = prod.__class__.__name__.lower()
        
        self.builder().makeDebugMsg("binpac", "bgn %s '%s'" % (pname, prod))

        self._debugShowInput("input", args.cur)
        self._debugShowInput("lahead start", args.lahstart)
        
        if isinstance(prod, grammar.Literal):
            self._parseLiteral(prod, args)
            
        elif isinstance(prod, grammar.Variable):
            self._parseVariable(prod, args)

        elif isinstance(prod, grammar.ChildGrammar):
            self._parseChildGrammar(prod, args)
            
        elif isinstance(prod, grammar.Epsilon):
            # Nothing else to do.
            self._finishedProduction(args.obj, prod, None)
            pass
        
        elif isinstance(prod, grammar.Sequence):
            self._parseSequence(prod, args, [])

        elif isinstance(prod, grammar.LookAhead):
            self._parseLookAhead(prod, args)
            
        elif isinstance(prod, grammar.Boolean):
            self._parseBoolean(prod, args)
            
        elif isinstance(prod, grammar.Switch):
            util.internal_error("grammar.Switch not yet supported")

        else:
            util.internal_error("unexpected non-terminal type %s" % repr(prod))

        self.builder().makeDebugMsg("binpac", "end %s '%s'" % (pname, prod))

    def _parseStartSymbol(self, args, params):
        prod = self._current_grammar.startSymbol()
        pname = prod.__class__.__name__.lower()
        
        # The start symbol is always a sequence.
        assert isinstance(prod, grammar.Sequence)
        self.builder().makeDebugMsg("binpac", "bgn start-sym %s '%s'" % (pname, prod))
        
        self._parseSequence(prod, args, params)
        
        self.builder().makeDebugMsg("binpac", "end start-sym %s '%s'" % (pname, prod))
        
    def _parseLiteral(self, lit, args):
        """Generates code to parse a literal."""
        builder = self.builder()
        builder.setNextComment("Parsing literal '%s'" % lit.literal().value())
        
        # See if we have a look-ahead symbol. 
        cond = builder.addTmp("__cond", hilti.core.type.Bool())
        cond = builder.equal(cond, args.lahead, _LookAheadNone)
        (no_lahead, have_lahead, done) = builder.makeIfElse(cond, tag="no-lahead")
        
        # If we do not have a look-ahead symbol pending, search for our literal.
        match = self._matchToken(no_lahead, "literal", str(lit.id()), [lit], args)
        symbol = builder.addTmp("__lahead", hilti.core.type.Integer(32))
        no_lahead.tuple_index(symbol, match, no_lahead.constOp(0))
        
        found_lit = self.functionBuilder().newBuilder("found-sym")
        found_lit.tuple_index(args.cur, match, found_lit.constOp(1))
        found_lit.jump(done.labelOp())

        values = [no_lahead.constOp(lit.id())]
        branches = [found_lit]
        (default, values, branches) = self._addMatchTokenErrorCases(builder, values, branches, no_lahead)
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
        token = builder.addTmp("__token", hilti.core.type.Reference([hilti.core.type.Bytes()]))
        done.bytes_sub(token, args.lahstart, args.cur)
        
        self.cg().setBuilder(done)
        
        self._finishedProduction(args.obj, lit, token)
        
    def _parseVariable(self, var, args):
        """Generates code to parse a variable."""
        type = var.type()

        builder = self.builder()
        builder.setNextComment("Parsing variable %s" % var)
        
        # We must not have a pending look-ahead symbol at this point. 
        cond = builder.addTmp("__cond", hilti.core.type.Bool())
        builder.equal(cond, args.lahead, _LookAheadNone)
        builder.debug_assert(cond)

        # Do we actually need the parsed value? We do if (1) we're storing it
        # in the destination struct, or (2) the variable's type defines a '$$'
        # parameter (i.e., there is a control hook) that we need to set. 
        
        need_val = (var.name() != None)
        
        for h in var.hooks():
            if isinstance(h, stmt.FieldControlHook):
                need_val = True
        
        # Call the type's parse function.
        name = var.name() if var.name() else "__tmp"
        dst = self.builder().addTmp(name , var.type().hiltiType(self.cg()))
        args.cur = type.generateParser(self, args.cur, dst, not need_val)
        
        # We have successfully parsed a rule. 
        self._finishedProduction(args.obj, var, dst if need_val else None)

    def _parseChildGrammar(self, child, args):
        """Generates code to parse another type represented by its own grammar."""
        
        utype = child.type()

        builder = self.builder()
        builder.setNextComment("Parsing child grammar %s" % child.type().name())

        # Generate the parsing code for the child grammar. 
        cpgen = ParserGen(self._cg)
        cgrammar = child.type().grammar()
        cpgen.compile(cgrammar)

        # Call the parsing code. 
        result = builder.addTmp("__presult", _ParseFunctionResultType)
        cobj = builder.addTmp("__cobj_%s" % utype.name(), utype.hiltiType(self._cg))

        cpgen._newParseObject(cobj)
        
        cargs = ParserGen._Args(self.functionBuilder(), (args.cur, cobj, args.lahead, args.lahstart))
        
        params = [p.evaluate(self._cg) for p in child.params()]
        
        cpgen._parseStartSymbol(cargs, params)

        if child.name():
            builder.struct_set(args.obj, builder.constOp(child.name()), cobj)

        builder.assign(args.cur, cargs.cur)
        builder.assign(args.lahead, cargs.lahead)
        builder.assign(args.lahstart, cargs.lahstart)
            
        self._finishedProduction(args.obj, child, cobj)
        
    def _parseSequence(self, prod, args, params):
        def _makeFunction():
            (func, args) = self._createParseFunction("sequence", prod)
            
            self.builder().setNextComment("Parse function for production '%s'" % prod)
            
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
        
        builder = self.builder()
        builder.setNextComment("Parsing non-terminal %s" % prod.symbol())

        result = builder.addTmp("__presult", _ParseFunctionResultType)

        if not params:
            for p in self._current_grammar.params():
                params += [builder.idOp(p.name())]

        builder.call(result, builder.idOp(func.name()), args.tupleOp(params))
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
            cond = builder.addTmp("__cond", hilti.core.type.Bool())
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
                self._parseProduction(alts[i], args)
                self._finishedProduction(args.obj, alts[i], None)
                self.builder().jump(done.labelOp())
                return branch
            
            values = []
            branches = []
            cases = [_makeBranch(0, alts, literals), _makeBranch(1, alts, literals)]

            for i in (0, 1):
                case = cases[i]
                for lit in literals[i]:
                    values += [builder.constOp(lit.id())]
                    branches += [case]
                
            (default, values, branches) = self._addMatchTokenErrorCases(builder, values, branches, no_lahead)
            builder.makeSwitch(args.lahead, values, default=default, branches=branches, cont=done, tag="lahead-next-sym")
            
            # Done, return the result.
            done.return_result(done.tupleOp([args.cur, args.lahead, args.lahstart]))

            self.cg().endFunction()
            return func

        func = self.moduleBuilder().cache(prod.symbol(), _makeFunction)

        builder = self.builder()
        builder.setNextComment("Parsing non-terminal %s" % prod.symbol())
        
        result = builder.addTmp("__presult", _ParseFunctionResultType)
        
        params = []
        for p in self._current_grammar.params():
            params += [builder.idOp(p.name())]

        builder.call(result, builder.idOp(func.name()), args.tupleOp(params))
        builder.tuple_index(args.cur, result, builder.constOp(0))
        builder.tuple_index(args.lahead, result, builder.constOp(1))
        builder.tuple_index(args.lahstart, result, builder.constOp(2))

    def _parseBoolean(self, prod, args):
        builder = self.builder()
        builder.setNextComment("Parsing boolean %s" % prod.symbol())

        save_builder = self.builder()
        
        done = self.functionBuilder().newBuilder("done")
        branches = [None, None]
        
        for (i, tag) in ((0, "True"), (1, "False")):
            alts = prod.branches()
            branches[i] = self.functionBuilder().newBuilder("if-%s" % tag)
            branches[i].setComment("Branch for %s" % tag)
            self.cg().setBuilder(branches[i])
            self._parseProduction(alts[i], args)
            self._finishedProduction(args.obj, alts[i], None)
            self.builder().jump(done.labelOp())
        
        bool = prod.expr().evaluate(self.cg())
        save_builder.if_else(bool, branches[0].labelOp(), branches[1].labelOp())
        
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
        
#        match_rtype = hilti.core.type.Tuple([hilti.core.type.Integer(32), args.cur.type()])
        match_rtype = hilti.core.type.Tuple([hilti.core.type.Integer(32), _BytesIterType])
        match = fbuilder.addTmp("__match", match_rtype)
        cond = fbuilder.addTmp("__cond", hilti.core.type.Bool())
        name = self._name(ntag1, ntag2)
        
        def _makePatternConstant():
            tokens = ["%s{#%d}" % (lit.literal().value(), lit.id()) for lit in literals]
            cty = hilti.core.type.Reference([hilti.core.type.RegExp(["nosub"])])
            return fbuilder.moduleBuilder().addConstant(name, cty, tokens)
        
        pattern = fbuilder.moduleBuilder().cache(name, _makePatternConstant)
        builder.assign(args.lahstart, args.cur) # Record starting position.
        
        builder.regexp_match_token(match, pattern, args.cur)
        return match

    def _debugShowInput(self, tag, cur):
        fbuilder = self.cg().functionBuilder()
        builder = self.cg().builder()
        
        next5 = fbuilder.addTmp("__next5", _BytesIterType)
        str = fbuilder.addTmp("__str", hilti.core.type.Reference([hilti.core.type.Bytes()]))
        builder.assign(next5, cur)
        builder.incr(next5, next5)
        builder.incr(next5, next5)
        builder.incr(next5, next5)
        builder.incr(next5, next5)
        builder.incr(next5, next5)
        builder.bytes_sub(str, cur, next5)
        
        msg = "- %s is " % tag
        builder.makeDebugMsg("binpac", msg + "%s ...", [str])
    
    def _finishedProduction(self, obj, prod, value):
        """Called whenever a production has sucessfully parsed value."""
        
        builder = self.builder()
        fbuilder = self.functionBuilder()
        
        if isinstance(prod, grammar.Terminal):
            if value:
                builder.makeDebugMsg("binpac", "- matched '%s' to '%%s'" % prod, [value])
            else:
                builder.makeDebugMsg("binpac", "- matched '%s'" % prod)

        if prod.name() and value:
            builder.struct_set(obj, builder.constOp(prod.name()), value)
                
        for hook in prod.hooks():
            name = "on_%s" % (prod.name() if prod.name() else "anon_%s" % builtin_id(prod))
            hookf = self._functionHook(name, hook)
            
            params = []
            for p in self._current_grammar.params():
                params += [builder.idOp(p.name())]

            if isinstance(hook, stmt.FieldControlHook):
                # Add the implicit '$$' argument. 
                assert value
                params += [value]
                
            rc = fbuilder.addTmp("__hookrc", hilti.core.type.Bool())
            builder.call(rc, builder.idOp(hookf.name()), builder.tupleOp([obj] + params))
        
    ### Methods defining types. 
    
    def _typeParseObject(self):
        """Returns the struct type for the parsed grammar."""
        
        def _makeType():
            fields = self._current_grammar.scope().IDs()
            fields = [(hilti.core.id.ID(f.name(), f.type().hiltiType(self._cg), hilti.core.id.Role.LOCAL), None) for f in fields]
            structty = hilti.core.type.Struct(fields)
            self._mbuilder.addTypeDecl(self._name("object"), structty)
            return structty
        
        return self._mbuilder.cache("struct_%s" % self._current_grammar.name(), _makeType)

    def _typeParseObjectRef(self):
        """Returns a reference to the struct type for the parsed grammar."""
        return hilti.core.type.Reference([self._typeParseObject()])

    def _newParseObject(self, obj):
        """Allocates and initializes a struct type for the parsed grammar.
        
        obj: hilti.core.instruction.Operand - The operand to the new object in.
        """
        
        self.builder().new(obj, self.builder().typeOp(self._typeParseObject()))
        
        for f in self._current_grammar.scope().IDs():
            default = f.type().hiltiDefault(self.cg())
            if not default:
                continue
            
            name = self.builder().constOp(f.name())
            default = hilti.core.instruction.ConstOperand(default)
            self.builder().struct_set(obj, name, default)
    
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
        
        arg1 = hilti.core.id.ID("__cur", _BytesIterType, hilti.core.id.Role.PARAM)
        arg2 = hilti.core.id.ID("__self", self._typeParseObjectRef(), hilti.core.id.Role.PARAM)
        arg3 = hilti.core.id.ID("__lahead", _LookAheadType, hilti.core.id.Role.PARAM)
        arg4 = hilti.core.id.ID("__lahstart", _BytesIterType, hilti.core.id.Role.PARAM)
        result = _ParseFunctionResultType

        args = [arg1, arg2, arg3, arg4]
        
        for p in self._current_grammar.params():
            args += [hilti.core.id.ID(p.name(), p.type().hiltiType(self._cg), hilti.core.id.Role.PARAM)]

        ftype = hilti.core.type.Function(args, result)
        name = self._name(prefix, prod.symbol())
        
        (fbuilder, builder) = self.cg().beginFunction(name, ftype)
        return (fbuilder.function(), ParserGen._Args(fbuilder, args))
        
    def _addMatchTokenErrorCases(self, builder, values, branches, repeat):
        """Build the standard error branches for the switch-statement
        following a ``match_token`` instruction."""
        
        fbuilder = builder.functionBuilder()
        
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
