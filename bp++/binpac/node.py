# $Id$

import visitor
import util

_recursives = set()

def check_recursion(m):
    """Decorator for ~~Node methods that automatically avoids infinite
    recursions. Decorated methods must have a single *driver* argument that is
    passed on to all recursive calls. Each time the method is called, it will
    be checked whether it has already by called for the current object with
    the same driver instance. If yes, the method call will not be executed.
    """
    global _recursives
    _recursives.add(m.__name__)
    return m

class CheckRecursion(object):
    def __getattribute__(self, name):
        """Implements the ~~check_recursion checking."""

        method = object.__getattribute__(self, name)

        # FIXME: Not quite clear why need name here.
        # Without it, sometimes methods aren't called. Might that be a GC
        # effect when an object gets recycled? If so, howver, adding the name
        # will probably reduce the chance of running into the problem, but not
        # elmininate it ...

        idx = str(id(self))
        def _checkRecursion(driver):
            try:
                if idx in driver._already:
                    return self

            except AttributeError:
                driver._already = {}

            # We store the object itself to prevent GC from deleting it; that
            # would mess up our tracking because new objects may get the same
            # ID.
            driver._already[idx] = self
            return method(driver)

        if name in _recursives:
            return _checkRecursion

        if name == "reset":
            # Clear memory.
            driver._already = {}

        if name == "codegen":
            import sys
            print >>sys.stderr, "### %s" % self.__class__.__name__

        return method

class Node(visitor.Visitable, CheckRecursion):
    """Base class for all classes that can be nodes in an |ast|.
    """

    def __init__(self, location):
        self._location = location
        self._comment = []

    def location(self):
        """Returns the location associated with the node.

        Returns: ~~Location - The location.
        """
        return self._location

    def setLocation(self, loc):
        """Sets the location associated with the node.

        loc: ~~Location - The location.
        """
        self._location = loc

    def setComment(self, comment):
        """Associates a comment with this node. It's up to the node how/where
        to use the comment.

        comment: string or list of string - Associates either single-line
        comment with the node (if given a single string), or a multi-line
        comment (if given a list of strings).
        """
        if isinstance(comment, str):
            comment = [comment]

        self._comment += comment

    def comment(self):
        """Returns the comment associated with this node.

        Returns: list of strings - Each list entry represent one line
        of the comment. The list may be empty.

        Note: This function always returns a list of string. For a single-line
        comment, the list will have only a single entry.
        """
        return self._comment

    ### Method that can be overridden by derived classes.

    @check_recursion
    def resolve(self, resolver):
        """Resolves any unknown types the node may have.

        Can be overridden by derived classes; the default implementation does
        nothing. If overridden, the parent's implementation should be called.
        If there are any sub-nodes that may also have any types to resolve,
        the method needs to do that recursively.

        The method is called before ~~validate, and thus needs to deal
        robustly with unexpected situations. It should however leave any error
        reporting to ~~validate and just return if it can't resolve something.

        resolver: ~~Resolver - The resolver to use.

        Note: The method is decorated with ~~check_recursion. That means
        recursion cycles will be avoided automatically. However, that also
        means that one can't use ``super`` to call the parent's
        implementation. Instead use ``ParentClass.resolve(self, resolver)``.
        """
        pass

    @check_recursion
    def validate(self, vld):
        """Validates the semantic correctness of the node.

        Can be overridden by derived classes; the default implementation does
        nothing.  If overridden, the parent's implementation should be called.
        If there are any errors encountered during validation, the method must
        call ~~Validator.error. If there are any sub-nodes that also need to
        be checked, the method needs to do that recursively.

        vld: ~~Validator - The validator triggering the validation.

        Note: The method is decorated with ~~check_recursion. That means
        recursion cycles will be avoided automatically. However, that also
        means that one can't use ``super`` to call the parent's
        implementation. Instead use ``ParentClass.validate(self, vld)``.
        """
        pass

    def pac(self, printer):
        """Converts the node back into parseable source code.

        Must be overidden by derived classes if the method can get called.

        printer: ~~Printer - The printer to use.
        """
        util.internal_error("Node.pac2() not overidden by %s" % self.__class__)



