# $Id$

import visitor
import util

class Node(visitor.Visitable):
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

    def resolve(self, resolver):
        """Resolves any unknown types the node may have.

        resolver: ~~Resolver - The resolver to use.

        Note: To implement resolving for derived classes, you must
        not override this method but instead ~~_resolve.
        """
        if self._already(resolver, "resolve"):
            return

        self._resolve(resolver)

    def validate(self, vld):
        """Validates the semantic correctness of the node.

        vld: ~~Validator - The validator triggering the validation.

        Note: To implement validation for derived classes, you must not
        override this method but instead ~~_validate.
        """

    def pac(self, printer):
        """Converts the node back into parseable source code.

        Can be overridden by derived classes; the default implementation
        returns a descriptive string pointing at a non-overriden method.
        nothing.  If overridden, the parent's implementation may be called
        (but also may not, depending on what's needed). 

        printer: ~~Printer - The printer to use.
        """
        printer.output("<pac() not defined for %s>" % self.__class__)

    ### Method that can be overridden by derived classes.

    def _resolve(self, resolver):
        """Implements resolving for derived classes.

        Can be overridden by derived classes; the default implementation does
        nothing. If overridden, the parent's implementation should be called.
        If there are any sub-nodes that may also have any types to resolve,
        the method needs to do that recursively.

        The method is called before ~~validate, and thus needs to deal
        robustly with unexpected situations. It should however leave any error
        reporting to ~~validate and just return if it can't resolve something.
        """
        return

    def _validate(self, vld):
        """Implements validation for derived classes.

        vld: ~~Validator - The validator triggering the validation.

        Can be overridden by derived classes; the default implementation does
        nothing.  If overridden, the parent's implementation should be called.
        If there are any errors encountered during validation, the method must
        call ~~Validator.error. If there are any sub-nodes that also need to
        be checked, the method needs to do that recursively.
        """
        return

    ### Internal method.

    def _already(self, driver, name):

        idx = str(id(self))

        try:
            if idx in driver._already:
                return True

        except AttributeError:
            driver._already = {}

        # We store the object itself to prevent GC from deleting it; that would
        # mess up our tracking because new objects may get the same ID.
        driver._already[idx] = self
        return False

