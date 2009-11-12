# $Id$
#
# Turns a grammar into a recursive descent HILTI parser. 

import hilti.core.module
import hilti.core.builder
import hilti.printer

from core import grammar

class ParserGen:
    def __init__(self, module):
        """Generates a parser from a grammar.
        
        module: hilti.core.module - The HILTI module to which the generated
        code will be added.
        """
        self._mbuilder = hilti.core.builder.ModuleBuilder(module)
        self._builders = {}
        
        self._current_grammar = None
        self._current_ptab = None
        self._current_builder = None
        self._current_type_struct = None
        self._current_type_struct_ref = None
        self._current_type_token = None
        
    def compile(self, grammar):
        """Adds a parser to the HILTI module. The parser is generated from the
        given grammar.
        
        This method can be called multiple times to generate a set of parsers
        for different grammars, all within the same module.
        
        grammar: Grammar - The grammar to generate the parser from.  The
        grammar must pass ~~Grammar.check.
        """

        self._current_grammar = grammar
        
        # Create the struct type for the parsed grammar.
        fields = [hilti.core.id.ID("dummy", hilti.core.type.Integer(16), hilti.core.id.Role.LOCAL)]
        structty = hilti.core.type.Struct([(f, None) for f in fields])
        self._mbuilder.addTypeDecl(self._makeName("object"), structty)
        
        self._current_ptab = grammar.parseTable()
        self._current_type_struct = structty
        self._current_type_struct_ref = hilti.core.type.Reference([structty])
        self._current_type_token = hilti.core.type.Integer(32)
        
        self._compileLHS(grammar.startSymbol())

    def _makeName(self, tag1, tag2 = None):
        name = "%s_%s" % (self._current_grammar.name(), tag1)
        if tag2:
            name += "_%s" % tag2
            
        # Makes sure the name contains only valid characters. 
        chars = [c if (c.isalnum() or c in "_") else "_0x%x_" % ord(c) for c in name]
        return "".join(chars).lower()
            
    def _getFunction(self, prefix, tag):
        name = self._makeName(prefix, tag)
        try:
            return self._builders[name].function()
        except KeyError:
            return None
    
    def _addFunction(self, prefix, tag, args = None, return_type = None):
        # If we got passed a return_type, we create a function of type
        #
        #     tuple<iterator<bytes>, return_type> f(iterator<bytes>[, <args>])
        #
        # If not, the function's type will be 
        #
        #     iterator<bytes> f(iterator<bytes>[, <args>])
                
        name = self._makeName(prefix, tag)
        iter_bytes = hilti.core.type.IteratorBytes(hilti.core.type.Bytes())
        arg1 = hilti.core.id.ID("start", iter_bytes, hilti.core.id.Role.PARAM)
        args = [(arg1, None)] + args
        result = hilti.core.type.Tuple([iter_bytes, return_type]) if return_type else iter_bytes
        ftype = hilti.core.type.Function(args, result)
        
        fbuilder = hilti.core.builder.FunctionBuilder(self._mbuilder, name, ftype)
        self._builders[name] = fbuilder
        self._current_builder = fbuilder
        return fbuilder

    def _compileLHS(self, symbol):
        
        rules = self._current_ptab[symbol]
        
        # We have two different cases here for the look-ahead terminal we can
        # have: (1) a single variable of a specific type; and (2) a set of
        # literals. We handle them separately. Everything else is not allowed
        # (and shouldn't show up here because Grammar.check() takes care of
        # that). 
    
        lookaheads = rules.keys()
    
        if len(lookaheads) == 1 and isinstance(lookaheads[0], grammar.Variable):
            # A variable. We don't need to do any look-ahead and can directly
            # parse the RHS. 
            
            # XXX TODO XXX
            
            rhs = rules[lookahead[0]]
            self._compileRHS(rules[lookahead[0]])
            
        else:
            # A set of literals. We need to do the look-ahead to figure out
            # what the RHS is.
            for sym in lookaheads:
                assert isinstance(sym, grammar.Literal)
            
            self._compileLookAhead(symbol, rules)
            
    def _compileRHS(self, rhs):
        # XXX
        pass
        
    def _compileLookAhead(self, symbol, rules):
        
        func = self._getFunction(symbol, "lahead")
        if not func:
            arg2 = hilti.core.id.ID("obj", self._current_type_struct_ref, hilti.core.id.Role.PARAM)
            arg3 = hilti.core.id.ID("lahead", self._current_type_token, hilti.core.id.Role.PARAM)
            fbuilder = self._addFunction(symbol, "lahead", [(arg, None) for arg in (arg2, arg3)], return_type=self._current_type_token)
            
            obj = fbuilder.idOp(arg2)
            lahead = fbuilder.idOp(arg3)
            
            # Build a regular expression for all the possible symbols. 
            syms = ["%s{#%d}" % (term.literal().value(), term.id()) for term in rules.keys()]
            cty = hilti.core.type.Reference([hilti.core.type.RegExp()])
            pattern = self._mbuilder.addConstant(self._makeName(symbol, "regexp"), cty, syms)

            ### Add the function body.
            
            # If we don't have a look-ahead symbol, search for the next symbol.
            cond = fbuilder.addLocal("cond", hilti.core.type.Bool())
            zero = fbuilder.constOp(0, hilti.core.type.Integer(32))
            cond = fbuilder.equal(cond, lahead, zero)
            no = fbuilder.makeIf(cond)
            no.regexp_find(lahead, pattern, fbuilder.idOp("start"))

            # If we don't find it because of lack of input, yield and try
            # again. 
            minus1 = fbuilder.constOp(-1, hilti.core.type.Integer(32))
            cond = fbuilder.equal(cond, lahead, minus1)
            again = fbuilder.makeIf(cond)
            again.yield_()
            again.jump(fbuilder.idOp(no.block().name()))
            

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
    pgen = ParserGen(module)
    pgen.compile(g2)

    hilti.printer.printAST(module)
    
        
    
    
        
        
        
