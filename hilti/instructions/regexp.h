///
/// \type Regular expressions.
///
/// The *regexp* data type provides regular expression matching on *string*
/// and *bytes* objects.
///
/// \ctor /ab.*def/
///
/// \cproto hlt_regexp*
///

#include "define-instruction.h"

static inline void _checkMatchTokenTupleBytes(const Instruction* i, shared_ptr<Expression> target)
{
    auto i32 = builder::integer::type(32);
    auto ib = builder::iterator::type(builder::bytes::type());
    builder::type_list tt = { i32, ib };
    i->canCoerceTo(builder::tuple::type(tt), target);
}

iBegin(regexp, New, "new")
    iTarget(optype::refRegExp)
    iOp1(optype::typeRegExp, true)

    iValidate {
        equalTypes(referencedType(target), typedType(op1));
    }

    iDoc(R"(
        Instantiates a new *regexp* instance. Before any of the matching
        functionality can be used, ~~Compile must be used to compile a pattern.
    )")

iEnd


iBegin(regexp, CompileString, "regexp.compile")
    iOp1(optype::refRegExp, false)
    iOp2(optype::string, true)

    iValidate {
    }

    iDoc(R"(
        Compiles the pattern in *op2* for subsequent matching. The  pattern must
        only contain ASCII characters and must
        not contain any back-references.  Each regexp instance can be
        compiled only once. Throws ~~ValueError if a second compilation
        attempt is performed.

        .. todo: We should support other than ASCII
        characters too but need the notion of a local character set first.

        .. todo: We should add compilation flags, like case-insensitive.
    )")

iEnd

iBegin(regexp, CompileSet, "regexp.compile")
    iOp1(optype::refRegExp, false)
    iOp2(optype::refList, true)

    iValidate {
        equalTypes(elementType(referencedType(op2)), optype::string);
    }

    iDoc(R"(
        Compiles the list of patterns in *op2* for subsequent matching. The
        patterns must
        only contain ASCII characters and must
        not contain any back-references.  Each regexp instance can be
        compiled only once. Throws ~~ValueError if a second compilation
        attempt is performed.

        .. todo: We should support other than ASCII
        characters too but need the notion of a local character set first.

        .. todo: We should add compilation flags, like case-insensitive.
    )")

iEnd


iBegin(regexp, FindBytes, "regexp.find")
    iTarget(optype::int32)
    iOp1(optype::refRegExp, true)
    iOp2(optype::iterBytes, true)
    iOp3(optype::optional(optype::iterBytes), true)

    iValidate {
    }

    iDoc(R"(
        Scans either the byte iterator range between
        *op2* and *op3* for the regular expression *op1* (if op3 is not given,
        searches until the end of the bytes object). Returns a positive
        integer if a match found found; if a set of patterns has been
        compiled, the integer then indicates which pattern has matched. If
        multiple patterns from the set match, the left-most one is taken. If
        multiple patterns match at the left-most position, it is undefined
        which of them is returned. The instruction returns -1 if no match was
        found but adding more input bytes could change that (i.e., a partial
        match). Returns 0 if no match was found and adding more input would
        not change that.  Todo: The string variant is not yet implemented.
    )")

iEnd

iBegin(regexp, FindString, "regexp.find")
    iTarget(optype::int32)
    iOp1(optype::refRegExp, true)
    iOp2(optype::string, true)

    iValidate {
    }

    iDoc(R"(
        Scans either string in *op2* for the regular expression *op1*.
        Returns a positive
        integer if a match found found; if a set of patterns has been
        compiled, the integer then indicates which pattern has matched. If
        multiple patterns from the set match, the left-most one is taken. If
        multiple patterns match at the left-most position, it is undefined
        which of them is returned. The instruction returns -1 if no match was
        found but adding more input bytes could change that (i.e., a partial
        match). Returns 0 if no match was found and adding more input would
        not change that.

        .. todo: This string variant is not yet implemented.
    )")

iEnd


iBegin(regexp, GroupsString, "regexp.groups")
    iTarget(optype::refVector)
    iOp1(optype::refRegExp, true)
    iOp2(optype::string, true)

    iValidate {
        // auto ty_target = as<type::ref\ <vector\ <(iterator<bytes>,iterator<bytes>)>>>(target->type());

        // TODO: Check target vector.
    }

    iDoc(R"(
        Scans the string in *op2* for the regular expression op1*. If the regular
        expression is found, returns a vector with one entry for each group
        defined in the regular expression. Each entry is either the substring
        matching the corresponding subexpression or a range of iterators
        locating the matching bytes, respectively. Index 0 always contains the
        string/bytes that match the total expression. Returns an empty vector
        if the expression is not found.  This method is not compatible with
        sets of multiple patterns; throws PatternError if used with a set, or
        when no pattern has been compiled yet.  Todo: The string variant is
        not yet implemented.

        .. todo: This string variant is not yet implemented.
    )")

iEnd

iBegin(regexp, GroupsBytes, "regexp.groups")
    iTarget(optype::refVector)
    iOp1(optype::refRegExp, true)
    iOp2(optype::iterBytes, true)
    iOp3(optype::optional(optype::iterBytes), true)

    iValidate {
        auto ib = builder::iterator::type(builder::bytes::type());
        builder::type_list tt = { ib, ib };
        auto rv = builder::reference::type(builder::vector::type(builder::tuple::type(tt)));
        canCoerceTo(rv, target);

        // auto ty_target = as<type::ref\ <vector\ <(iterator<bytes>,iterator<bytes>)>>>(target->type());
        //
        // TODO: Check target vector.
    }

    iDoc(R"(
        Scans either the the byte iterator range between
        *op2* and *op3* for the regular expression op1*. If the regular
        expression is found, returns a vector with one entry for each group
        defined in the regular expression. Each entry is either the substring
        matching the corresponding subexpression or a range of iterators
        locating the matching bytes, respectively. Index 0 always contains the
        string/bytes that match the total expression. Returns an empty vector
        if the expression is not found.  This method is not compatible with
        sets of multiple patterns; throws PatternError if used with a set, or
        when no pattern has been compiled yet.  Todo: The string variant is
        not yet implemented.
    )")

iEnd


iBegin(regexp, MatchTokenString, "regexp.match_token")
    iTarget(optype::tuple)
    iOp1(optype::refRegExp, true)
    iOp2(optype::string, true)

    iValidate {
        // auto ty_target = as<type::(int\ <32>,iterator<bytes>)>(target->type());

        // TODO: Check target tuple.
    }

    iDoc(R"(
        Matches the beginning of the string in *op1* for the regular expression
        *op1* (if op3 is not given, searches until the end of the bytes
        object).  The regexp must have been compiled with the ~~NoSub
        attribute.  Returns a 2-tuple with (1) a integer match-indicator
        corresponding to the one returned by ~~Find; and (2) a bytes iterator
        that pointing one beyond the last examined byte (i.e., right after the
        match if we had one, or right after the input data if not).  Note: As
        the name implies, this a specialized version for parsing purposes,
        enabling optimizing for the case that we don't need any subexpression
        capturing and must start the match right at the initial position.
        Internally, the implementation is only slightly optimized at the
        moment but it could be improved further at some point.  Todo: The
        string variant is not yet implemented. The bytes implementation should
        be further optimized.

        .. todo: This string variant is not yet implemented.
    )")

iEnd

iBegin(regexp, MatchTokenBytes, "regexp.match_token")
    iTarget(optype::tuple)
    iOp1(optype::refRegExp, true)
    iOp2(optype::iterBytes, true)
    iOp3(optype::optional(optype::iterBytes), true)


    iValidate {
        _checkMatchTokenTupleBytes(this, target);
    }

    iDoc(R"(
        Matches the beginning of the byte
        iterator range between *op2* and *op3* for the regular expression
        *op1* (if op3 is not given, searches until the end of the bytes
        object).  The regexp must have been compiled with the ~~NoSub
        attribute.  Returns a 2-tuple with (1) a integer match-indicator
        corresponding to the one returned by ~~Find; and (2) a bytes iterator
        that pointing one beyond the last examined byte (i.e., right after the
        match if we had one, or right after the input data if not).  Note: As
        the name implies, this a specialized version for parsing purposes,
        enabling optimizing for the case that we don't need any subexpression
        capturing and must start the match right at the initial position.
        Internally, the implementation is only slightly optimized at the
        moment but it could be improved further at some point.  Todo: The
        string variant is not yet implemented. The bytes implementation should
        be further optimized.
    )")

iEnd


iBegin(regexp, MatchTokenAdvanceString, "regexp.match_token_advance")
    iTarget(optype::tuple)
    iOp1(optype::refMatchTokenState, false)
    iOp2(optype::string, true)

    iValidate {
        _checkMatchTokenTupleBytes(this, target);
    }

    iDoc(R"(
        Performs matching previously initialized with
        ~~regexp.match_token_init`` on the string in *op2*. If op3 is not given, searches
        until the end of the bytes object. This method can be called multiple
        times with new data as long as no match has been found, and it will
        continue matching from the previous state as if all data would have
        been concatenated.  Returns a 2-tuple with (1) a integer match-
        indicator corresponding to the one returned by ~~Find; and (2) a bytes
        iterator that pointing one beyond the last examined byte (i.e., right
        after the match if we had one, or right after the input data if not).
        The same match state must not be used again once this instructions has
        returned a match indicator >= zero.  Note: As their name implies, the
        ``regexp.match_token_*`` family of instructions are specialized
        versiond for parsing purposes, enabling optimizing for the case that
        we don't need any subexpression capturing and must start the match
        right at the initial position.  Todo: The string variant is not yet
        implemented.

        .. todo: This string variant is not yet implemented.

    )")

iEnd

iBegin(regexp, MatchTokenAdvanceBytes, "regexp.match_token_advance")
    iTarget(optype::tuple)
    iOp1(optype::refMatchTokenState, false)
    iOp2(optype::iterBytes, true)
    iOp3(optype::optional(optype::iterBytes), true)

    iValidate {
        _checkMatchTokenTupleBytes(this, target);
    }

    iDoc(R"(
        Performs matching previously initialized with
        ~~regexp.match_token_init`` the byte
        iterator range between *op2* and *op3*. If op3 is not given, searches
        until the end of the bytes object. This method can be called multiple
        times with new data as long as no match has been found, and it will
        continue matching from the previous state as if all data would have
        been concatenated.  Returns a 2-tuple with (1) a integer match-
        indicator corresponding to the one returned by ~~Find; and (2) a bytes
        iterator that pointing one beyond the last examined byte (i.e., right
        after the match if we had one, or right after the input data if not).
        The same match state must not be used again once this instructions has
        returned a match indicator >= zero.  Note: As their name implies, the
        ``regexp.match_token_*`` family of instructions are specialized
        versiond for parsing purposes, enabling optimizing for the case that
        we don't need any subexpression capturing and must start the match
        right at the initial position.  Todo: The string variant is not yet
        implemented.
    )")

iEnd

iBegin(regexp, MatchTokenInit, "regexp.match_token_init")
    iTarget(optype::refMatchTokenState)
    iOp1(optype::refRegExp, true)

    iValidate {
    }

    iDoc(R"(
        Initializes incrementatal matching for the regexp *op1*. *op1* will be
        considered implicitly anchored to the beginning of the data, and it
        must have been compiled with the ~~NoSub attribute.  This instruction
        does not perform any matching itself, you use
        ~~regexp.match_token_advance for that.  Note: As their name implies,
        the ``regexp.match_token_*`` family of instructions are specialized
        versiond for parsing purposes, enabling optimizing for the case that
        we don't need any subexpression capturing and must start the match
        right at the initial position.
    )")

iEnd

iBegin(regexp, SpanString, "regexp.span")
    iTarget(optype::tuple)
    iOp1(optype::refRegExp, true)
    iOp2(optype::string, true)

    iValidate {
        // TODO:Check tuple.
    }

    iDoc(R"(
        Scans either the string in *op2* for the regular expression *op1*. Returns a 2-tuple
        with (1) a integer match-indicator corresponding to the one returned
        by ~~Find; and (2) the matching substring or a tuple of iterators
        locating the bytes which match, respectively; if there's no match, the
        second element is either an empty string or a tuple with two
        ``bytes.end`` iterators, respectively.  Throws PatternError if no
        pattern has been compiled yet.  Todo: The string variant is not yet
        implemented.

        .. todo: This string variant is not yet implemented.
      
    )")

iEnd

iBegin(regexp, SpanBytes, "regexp.span")
    iTarget(optype::tuple)
    iOp1(optype::refRegExp, true)
    iOp2(optype::iterBytes, true)
    iOp3(optype::optional(optype::iterBytes), true)

    iValidate {
        auto i32 = builder::integer::type(32);
        auto ib = builder::iterator::type(builder::bytes::type());
        builder::type_list tt1 = { ib, ib };
        builder::type_list tt2 = { i32, builder::tuple::type(tt1) };
        canCoerceTo(builder::tuple::type(tt2), target);
    }

    iDoc(R"(
        Scans either the byte iterator range between
        *op2* and *op3* for the regular expression *op1*. Returns a 2-tuple
        with (1) a integer match-indicator corresponding to the one returned
        by ~~Find; and (2) the matching substring or a tuple of iterators
        locating the bytes which match, respectively; if there's no match, the
        second element is either an empty string or a tuple with two
        ``bytes.end`` iterators, respectively.  Throws PatternError if no
        pattern has been compiled yet.  Todo: The string variant is not yet
        implemented.
    )")

iEnd

