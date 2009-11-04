# $Id$
#
# Grammar productions.

import sys
import copy

from support import util

class Production(object):
    """Base class for all grammar productions. All productions are generally
    context-insensitive but can have semantic conditions attached to them.
    
    name: string - A name for this production; can be the None for anonymous
    productions.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    _counter = 0
    
    def __init__(self, name=None, location=None):
        self._name = name
        self._pred = None
        self._pre = []
        self._post = []
        self._location = location
        
        # Internal production name.
        Production._counter += 1
        self._tag = "T%d" % Production._counter
        
    def location(self):
        """Returns the location where the production was defined.
        
        Returns: ~~Location - The location.
        """
        return self._location
        
    def setPredicate(self, expr):
        """Adds a semantic Predicate to the production. The production will
        only be considered if the Predicate evaluates to True. 
        
        expr: ~~Expr - The expression to set a the predicate. It must evaluate
        to a boolen.
        """
        expr = expr.simplify()
        self._pred = expr
        
    def setName(self, name):
        """Sets the production's name.
        
        name: string - The name. Setting it to None turns the production into
        an anonymous one,
        """
        self._name = name
        
    def name(self):
        """Returns the production's name.
        
        Returns: string - The name, or None for anonymous productions. 
        """
        return self._name
    
    def predicate(self):
        """Returns the production's predicate.
        
        Returns: ~~Expr or None - The predicate, or None if none defined.
        """
        return self._pred

    def __str__(self):
        name = "($%s)" % self._name if self._name else ""
        pred = " if %s" % str(self._pred) if self._pred else ""
        return "%s%s%s" % (name, self._fmtDerived(), pred)
    
    def _fmtDerived(self):
        # To be overridden by derived classes.
        return "<general production>"

class Terminal(Production):
    """A terminal.
    
    type: ~~Type - The type of the literal.
    
    name: string - A name for this production; can be the None for anonymous
    productions.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, type, name=None, location=None):
        super(Terminal, self).__init__(name, location=location)
        self._type = type

    def type(self):
        """Returns the type of the terminal.
        
        Returns: ~~Type - The type.
        """
        return self._type

    def _fmtDerived(self):
        return "<generic terminal>"
    
class Literal(Terminal):
    """A literal. A literal is anythig which can be directly scanned for as
    sequence of bytes. 
    
    literal: ~~Constant - The literal. 
    
    name: string - A name for this production; can be the None for anonymous
    productions.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    
    def __init__(self, literal, name=None, location=None):
        super(Literal, self).__init__(literal.type(), name)
        self._literal = literal

    def literal(self):
        """Returns the literal.
        
        Returns: ~~Constant - The literal.
        """
        return self._literal

    def _fmtDerived(self):
        return "literal('%s')" % self._literal

class Variable(Terminal):
    """A variable. A variable is terminal that will be parsed from the input
    stream according to its type, yet is not recognizable as such in advance
    by just looking at the available bytes. If we start parsing, we assume it
    will match (and if not, generate an error).
    
    type: ~~Type - The type of the variable.
    
    name: string - A name for this production; can be the None for anonymous
    productions.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    
    def __init__(self, type, name=None, location=None):
        super(Variable, self).__init__(type, name, location=location)

    def _fmtDerived(self):
        return "type(%s)" % self.type()

class EpsilonClass(Production):
    """The empty production.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, location=None):
        super(EpsilonClass, self).__init__(None, location=location)
        self._tag = "epsilon"
        
    def _fmtDerived(self):
        return "<epsilon>"    

Epsilon = EpsilonClass()
    
class NonTerminal(Production):
    """A non-terminal.
    
    type: ~~Type - The type of the literal.
    
    name: string - A name for this production; can be the None for anonymous
    productions.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, name=None, sym=None, location=None):
        super(NonTerminal, self).__init__(name, location=location)
        self._sym = sym
        if sym:
            self._tag = "%s_%s" % (sym, self._tag)

    def _fmtDerived(self):
        return "<generic non-terminal>"
    
class Sequence(NonTerminal):
    """A sequence of other productions. 
    
    seq: list of ~~Production - The sequence of productions. 
    
    name: string - A name for this production; can be the None 
    for anonymous productions.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, seq, name=None, sym=None, location=None):
        super(Sequence, self).__init__(name, sym, location=location)
        self._seq = seq

    def sequence(self):
        """Returns the sequence of productions.
        
        Returns: list of ~~Production - The sequence.
        """
        return self._seq
    
    def addProduction(self, prod):
        """Appends one production to the sequence.
        
        prod: ~~Production - The production
        """
        self._seq += [prod]

    def _fmtDerived(self):
        return "( " + "; ".join([str(p) for p in self._seq]) + " )"
    
class Alternative(NonTerminal):
    """A alternative of other productions. 
    
    seq: list of ~~Production - The alternative of productions. 
    
    name: string - A name for this production; can be the None 
    for anonymous productions.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, alt, name=None, sym=None, location=None):
        super(Alternative, self).__init__(name, sym, location=location)
        self._alt = alt

    def alternatives(self):
        """Returns the alternative of productions.
        
        Returns: list of ~~Production - The alternative.
        """
        return self._alt
    
    def addProduction(self, prod):
        """Appends one production to the alternative.
        
        prod: ~~Production - The production
        """
        self._alt += [prod]

    def _fmtDerived(self):
        return "( " + " || ".join([str(p) for p in self._alt]) + " )"

    
class Grammar:
    def __init__(self, name, root):
        """Instantiates a grammar given its root production.
        
        name: string - A name which uniquely identifies this grammar.
        root: Production - The grammar's root production.
        """
        self._name = name
        self._start = root._tag
        self._terms = {}
        self._prods = {}
        self._locations = {}
        self._nullable = {}
        self._first = {}
        self._follow = {}
        
        self._addProduction(root)
        #self._simplify()
        #self._simplify()
        self._computeTables()

    def name(self):
        """Returns the name of the grammar. The name uniquely identifies the
        grammar. 
        
        Returns: string - The name.
        """
        return self._name
        
    def check(self):
        """Checks the grammar for ambiguity. From an ambigious grammar, no
        parser can be generated.
        
        Returns: string or None - If the return valuue is None, the grammar is
        fine; otherwise the returned string contains a descriptions of the
        ambiguities encountered, suitable for inclusion in an error message.
        """
        
        def _loc(sym):
            try:
                return self._locations[sym]
            except KeyError:
                return str(sym)
        
        msg = ""
        
        for (sym, lookahead) in self._parser.items():
            for (term, rhss) in lookahead.values():
                if len(rhss) > 1:
                    msg += "%s: productions are ambigious for look-ahead symbol %s\n" % (_loc(sym), term)

            have_var = False
            total = 0
            for (term, rhss) in lookahead.values():
                if isinstance(term, Variable):
                    have_var = True
                total += 1
                
            if have_var and total > 1:
                msg += "%s: a variable must be the only option\n" % _loc(sym)
                
        return msg if msg != "" else None

    def parseTable(self):
        """Returns the grammar's parse table. The parse table has the form of
        a two-level dictionary. The first level is indexed by the LHS tag, and
        yield a second-level dictionary describing the alternatives we have
        when deriving a LHS. The alternative dictionary is indexed by the
        look-ahead symbol and yields the RHS as a list of (a) ~~Terminal
        instances for terminals, and (b) strings for non-terminals where the
        string is the LHS tag of the production to follow (the tag can be
        recursively used as the next index into the parse table). 
          
        The grammar must not be ambigious; if it is, this function will abort.
        Use ~~check to make sure the grammar is ok.
        
        Returns: dictionary - The dictionary as described above.
        
        Note: The dictionary returned by this function is different from the
        internal representation, which can map a look-ahead to multiple RHSs.
        In that case the grammar is ambigious though and will be flagged by
        ~~check.
        """
        assert not self.check()
        
        ptab = {}
        for (lhs, lookaheads) in self._parser.items():
            ptab[lhs] = dict([(sym, rhss[0]) for (sym, rhss) in lookaheads.values()])

        return ptab

    def startSymbol(self):
        """Returns the LHS symbol of the starting production. The tag can be
        used as index into the dictionary returned by ~~parseTable.
        
        Returns: string - The starting symbol.
        """
        return self._start
    
    @staticmethod
    def _isTerminal(p):
        return isinstance(p, Terminal)

    @staticmethod
    def _isEpsilon(p):
        return isinstance(p, EpsilonClass)
    
    @staticmethod
    def _isNonTerminal(p):
        return not (isinstance(p, Terminal) or isinstance(p, EpsilonClass))
        
    def _addProduction(self, p):
        if p._tag in self._prods:
            return

        self._locations[p._tag] = p.location() if p.location() else p._tag
        
        def _conv(p):
            return p._tag if Grammar._isNonTerminal(p) else p
        
        # We turn the productions into the classic lhs->rhs style here, with
        # alternatives being converted into multiple of them. We keep terminals
        # separately. For non-terminals, we only store their tag; for
        # Terminals the instance itself.
        if isinstance(p, Sequence):
            self._prods[p._tag] = [[_conv(q) for q in p.sequence()]]

            for q in p.sequence():
                self._addProduction(q)
            
        elif isinstance(p, Alternative):
            self._prods[p._tag] = [[_conv(q)] for q in p.alternatives()]

            for q in p.alternatives():
                self._addProduction(q)
                
        elif Grammar._isTerminal(p):
            pass

        elif Grammar._isEpsilon(p):
            pass
            
        else:
            util.internal_error("unknown case in Grammar.addProduction: %s" % str(p))
            
    def _simplify(self):
        # Remove some types of unecessary rules.
        for alts in self._prods.values():
            for i in range(len(alts)):
                p = alts[i]
            
                # For productions of the form "A->B; B->C;", let A points to C
                # directly.
                if len(p) == 1 and Grammar._isNonTerminal(p[0]):
                    succ = self._prods[p[0]]
                    if len(succ) == 1:
                        alts[i] = succ[0]
                        
                # For productions of the form "A-> xBy ; B -> terminal"',
                # replace B with the Terminal.
                for j in range(len(p)):
                    q = p[j]
                    if Grammar._isNonTerminal(q):
                        succ = self._prods[q]
                        if len(succ) == 1 and len(succ[0]) == 1 and not Grammar._isNonTerminal(succ[0][0]):
                            p[j] = succ[0][0]
            
        # Remove unused productions.
        def _closure(root, used):
            used.add(root)
            for a in self._prods[root]:
                for p in a:
                    if Grammar._isNonTerminal(p) and not p in used:
                        _closure(p, used)
                        
            return used
        
        used = _closure(self._start, set())
        
        for tag in set(self._prods) - used:
            del self._prods[tag]

        # Remove duplicates in the alternative list. We use the str()
        # representation to identify identical alternatives.
        for (tag, alts) in self._prods.items():
            news = []
            for a in alts:
                if str(a) not in [str(n) for n in news]:
                    news += [a]
                
            self._prods[tag] = news

    def _computeTables(self):
        # Computes FIRST, FOLLOW, & NULLABLE. This follows roughly the Algorithm
        # 3.13 from Modern Compiler Implementation in C by Appel/Ginsburg. See
        # http://books.google.com/books?id=A3yqQuLW5RsC&pg=PA49
        
        def _add(tbl, dst, src, changed):
            if tbl[dst] | set(src) == tbl[dst]:
                return changed
            
            tbl[dst] |= set(src)
            return True
            
        def _nullable(a, i, j):
            for l in range(i, j):
                if Grammar._isEpsilon(a[l]):
                    continue
                
                if Grammar._isTerminal(a[l]) or not self._nullable[a[l]]:
                    return False
            
            return True
        
        def _first(t):
            if Grammar._isEpsilon(t):
                return []
            
            if Grammar._isTerminal(t):
                return [t]
            
            return self._first[t]

        # Initialize sets.
        for t in self._prods:
            self._nullable[t] = False
            self._first[t] = set()
            self._follow[t] = set()

        # Iterate until no further change.
        while True:
            changed = False
            for (tag, alts) in self._prods.items():
                for a in alts:
                    k = len(a)

                    if _nullable(a, 0, k) and not self._nullable[tag]:
                        self._nullable[tag] = True
                        changed = True
                        
                    for i in range(k):
                        if _nullable(a, 0, i):
                            changed = _add(self._first, tag, _first(a[i]), changed)

                        if not Grammar._isNonTerminal(a[i]):
                            continue
                                
                        if _nullable(a, i+1, k):
                            changed = _add(self._follow, a[i], self._follow[tag], changed)

                        for j in range(i+1, k):
                            if _nullable(a, i+1, j):
                                changed = _add(self._follow, a[i], _first(a[j]), changed)
                                    
            if not changed:
                break
            
        # Build the parse table. 
        def _addla(la, term, rhs):
            assert Grammar._isTerminal(term)
            try:
                (term, cur) = la[str(term)]
                for c in cur:
                    if id(rhs) == id(c):
                        return

                cur += [rhs]
                    
                la[str(term)] = (term, cur)
                
            except KeyError:
                la[str(term)] = (term, [rhs])
 
        def _firstOfRhs(rhs):
            first = set()
            
            for sym in rhs:
                if Grammar._isEpsilon(sym):
                    continue
                
                if Grammar._isTerminal(sym):
                    return [sym]
                
                first |= self._first[sym]
                
                if not self._nullable[sym]:
                    break
            
            return first
                
        self._parser = {}
        
        for (tag, alts) in self._prods.items():
            lookahead = {}
            for rhs in alts:
                for term in _firstOfRhs(rhs):
                    _addla(lookahead, term, rhs)
                                
                if _nullable(rhs, 0, len(rhs)):
                    for term in self._follow[tag]:
                        _addla(lookahead, term, rhs)
                         
            self._parser[tag] = lookahead
        
    def __str__(self):
        s = "Grammar '%s'\n" % self._name
        for (tag, alts) in self._prods.items():
            for a in alts:
                start = "(*)" if tag == self._start else""
                prod = "; ".join([p if Grammar._isNonTerminal(p) else str(p) for p in a])
                s += "%3s %2s -> %s\n" % (start, tag, prod)
                
        s += "\n"
        s += "    Epsilon:\n"
        
        for (tag, eps) in self._nullable.items():
            s += "      %s = %s\n" % (tag, "true" if eps else "false" )
        
        s += "\n"
        s += "    First_1:\n"
        
        for (tag, first) in self._first.items():
            s += "      %s = %s\n" % (tag, [str(f) for f in first])
            
        s += "\n"
        s += "    Follow:\n"
        
        for (tag, follow) in self._follow.items():
            s += "      %s = %s\n" % (tag, [str(f) for f in follow])
            
        s += "\n"
        s += "    Parse table:\n"
        
        for (tag, lookahead) in self._parser.items():
            s += "      %s\n" % tag
            for (sym, rhss) in lookahead.values():
                s += "          %s -> #%d %s\n" % (sym, len(rhss), " || ".join([" ".join([str(r) for r in rhs]) for rhs in rhss]))
                
        return s

if __name__ == "__main__":

    from core import type
    from support import constant
    
    # Simple example grammar from
    #
    # http://www.cs.uky.edu/~lewis/essays/compilers/td-parse.html

    
    print "=== Example grammar 1"
    
    a = Literal(constant.Constant("a", type.Bytes()))
    b = Literal(constant.Constant("b", type.Bytes()))
    c = Literal(constant.Constant("c", type.Bytes()))
    d = Literal(constant.Constant("d", type.Bytes()))

#    A = Sequence([a])
#    B = Alternative([A, Sequence([A])])
#    print Grammar(B)
    
    
#    sys.exit(1)
    
    cC = Sequence([])
    bD = Sequence([])
    AD = Sequence([])
    aA = Sequence([])
    
    A = Alternative([Epsilon, a], sym="A")
    B = Alternative([Epsilon, bD], sym="B")
    C = Alternative([AD, b], sym="C")
    D = Alternative([aA, c], sym="D")

    ABA = Sequence([A,B,A])
    S = Alternative([ABA, cC], sym="S")

    cC.addProduction(c)
    cC.addProduction(C)
    
    bD.addProduction(b)
    bD.addProduction(D)

    AD.addProduction(A)
    AD.addProduction(D)
    
    aA.addProduction(a)
    aA.addProduction(A)

    g1 = Grammar("Grammar1", S)
    print str(g1)
    
    error = g1.check()
    if error:
        print error
    
    # Simple example grammar from "Parsing Techniques", Fig. 8.9
        
    print "=== Example grammar 2"

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
    g2 = Grammar("Grammar2", root)
    
    print str(g2)
    
    error = g2.check()
    if error:
        print error
    
    # Simple example grammar from "Parsing Techniques", Fig. 8.9
        
    print "=== Example grammar 2"

    hdrkey = Variable(type.Bytes())
    colon = Literal(constant.Constant(":", type.Bytes()))
    ws = Literal(constant.Constant("[ \\t]*", type.Bytes()))
    hdrval = Variable(type.Bytes())
    nl = Literal(constant.Constant("[\\r\\n]", type.Bytes()))

    header = Sequence([hdrkey, ws, colon, ws, hdrval, nl], sym="Header")

    list1 = Sequence([header], sym="List1")
    list2 = Alternative([list1, Epsilon], sym="List2")
    list1.addProduction(list2)
    
    all = Alternative([list2, nl], sym="All")
    g3 = Grammar("Grammar3", all)
    
    print str(g3)
    
    error = g3.check()
    if error:
        print error
