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

        Note: To implement validation for derived classes, you must
        not override this method but instead ~~_validate.
        """
        if self._already(vld, "validate"):
            return

        self._validate(vld)

    def canonify(self, canonifier):
        """Transform the node into canonified state before code generation.

        The method must only be called if ~~validate has not reported any
        errors.

        canonifier: ~~Canonifier - The canonfier to use.

        Returns: ~~Node - Returns a node equivalent to *self* with all
        canonifications carried out; that may either be *self* itself, or a
        newly instantiated type; the caller must then use the returned node
        instead of *self* afterwards.

        Note: To implement canonification for derived classes, you must
        not override this method but instead ~~_canonify.
        """
        if self._already(canonifier, "canonify"):
            return

        return self._canonify(canonifier)

    def codegen(self, cg):
        """Generates code for the node.

        cg: ~~CodeGen - The code generator to use.

        Note: To implement code generation for derived classes, you must
        not override this method but instead ~~_codegen.
        """
        if self._already(cg, "codegen"):
            return

        cg.trace("%s" % self)

        self._codegen(cg)

    def autodoc(self):
        """Prints out documentation for the node. The output is suitable as
        input for HILTI's Sphinx-based manual, and will be printed to stdout.

        Can be overridden by derived classes. The default implementation does
        nothing.
        """
        pass

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

    def _canonify(self, canonifier):
        """Implements canonification for derived classes.

        Can be overridden by derived classes; the default implementation just
        returns *self*. If overridden, the parent's implementation should be
        called first, and then the object *returned* canonified.

        The method will only be called if ~~validate has not reported any
        errors.

        canonifier: ~~Canonifier - The canonfier to use.

        Returns: ~~Node - Returns a node equivalent to *self* with all
        canonifications carried out; that may either be *self* itself, or a
        newly instantiated type; the caller must then use the returned node
        instead of *self* afterwards.

        Todo: I think no class does this "canonify what the parent returns"
        yet. But they all should ...
        """
        return self

    def _codegen(self, cg):
        """Implements code generation for derived classes.

        cg: ~~CodeGen - The code generator to use.

        Must be overriden by derived classes. Derived classes should *not*
        call the parent's implementation.
        """
        util.internal_error("Node._codegen() not overidden by %s" % self.__class__)

    def output(self, printer):
        """Converts the node back into parseable source code.

        Must be overidden by derived classes.

        printer: ~~Printer - The printer to use.
        """
        util.internal_error("Node.srcCode() not overidden by %s" % self.__class__)

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

