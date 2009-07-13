# $Id$
#
# Grammar productions.

class Production(object):
    """Base class for all grammar productions. All productions are generally
    context-insensitive but can have semantic conditions attached to them.
    
    symbol: string - The lhs symbol.
    
    name: string - A name for this production, which can be the empty string
    for anonymous productions.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, symbol, name="", location=None):
        self._name = name
        self._symbol = symbol
        self._pred = None
        self._pre = []
        self._post = []
        self._location = location

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
        
    def symbol(self):
        """Returns the production's symbol.
        
        Returns: string - The symbol (will be None for a ~~Terminal).
        """
        return self._symbol

    def name():
        """Returns the production's name.
        
        Returns: string - The name, which may be the empty string.
        """
        return self._name
    
    def predicate():
        """Returns the production's predicate.
        
        Returns: ~~Expr or None - The predicate, or None if none defined.
        """
        return self._pred

    def __str__(self):
        name = "($%s)" % self._name if self._name else ""
        pred = " if %s" % str(self._pred) if self._pred else ""
        sym = "%s := " % self._symbol if self._symbol else ""
        return "%s%s%s%s" % (name, sym, self._fmtDerived(), pred)
    
    def _fmtDerived(self):
        # To be overridden by derived classes.
        return "<general production>"
    
class Terminal(Production):
    """A terminal.
    
    symbol: string - A string-representation of the terminal.

    type: ~~Type - The type of the literal.
    
    name: string - A name for this terminal, which can be the empty string.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, type, name="", location=None):
        super(Terminal, self).__init__(None, name, location=location)
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
    
    name: string - A name for this terminal, which can be the empty string.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    
    def __init__(self, literal, name="", location=None):
        super(Literal, self).__init__(literal.type(), name)
        self._literal = literal

    def terminal(self):
        """Returns the type of the terminal.
        
        Returns: ~~Constant - The terminal.
        """
        return self._terminal
    
    def _fmtDerived(self):
        return "literal(%s)" % self._literal

class Variable(Terminal):
    """A variable. A variable is terminal that will be parsed from the input
    stream according to its type, yet is not recognizable as such in advance
    by just looking at the available bytes. If we start parsing, we assume it
    will match (and if not, generate an error).
    
    type: ~~Type - The type of the variable.
    
    name: string - A name for this terminal, which can be the empty string.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    
    def __init__(self, type, name="", location=None):
        super(Variable, self).__init__(type, name, location=location)

    def _fmtDerived(self):
        return "type(%s)" % self.type()
        
class Sequence(Production):
    """A sequence of other productions. 
    
    symbol: string - The lhs symbol.
    
    seq: list of ~~Production - The sequence of productions. 
    
    name: string - A name for this production, which can be the empty string
    for anonymous productions.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, symbol, seq, name = "", location=None):
        super(Sequence, self).__init__(symbol, name, location=location)
        self._seq = seq

    def seq(self):
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

        
        
