
#ifndef BINPAC_OPERATOR_H
#define BINPAC_OPERATOR_H

#include <unordered_map>

#include "common.h"

namespace binpac {

namespace passes { class Validator; }

namespace operator_ {

enum Kind {
    None = 0,
    Assign,
    Attribute,
    BitAnd,
    BitOr,
    BitXor,
    Call,
    DecrPostfix,
    DecrPrefix,
    Deref,
    Div,
    Equal,
    Greater,
    HasAttribute,
    IncrPostfix,
    IncrPrefix,
    Index,
    IndexAssign,
    LogicalAnd,
    LogicalOr,
    Lower,
    MethodCall,
    Minus,
    Mod,
    Mult,
    New,
    Not,
    Plus,
    PlusAssign,
    Power,
    ShiftLeft,
    ShiftRight,
    SignNeg,
    SignPos,
    Size,
};

const std::unordered_map<Kind, string, std::hash<int>> UnaryOperators = {
    {SignNeg, "-" },
    {SignPos, "-" },
    {DecrPrefix, "--" },
    {IncrPrefix, "--" },
    {Deref, "*" },
};

// These are used by the printer. Operators not listed in one of these maps
// are handled indidividually.
const std::unordered_map<Kind, string, std::hash<int>> BinaryOperators = {
    {Assign, "=" },
    {BitAnd, "&" },
    {BitOr, "|" },
    {BitXor, "^" },
    {Div, "/" },
    {Equal, "==" },
    {Greater, ">" },
    {IncrPrefix, "++" },
    {LogicalAnd, "&&" },
    {LogicalOr, "||" },
    {Lower, "<" },
    {Minus, "-" },
    {Mod, "mod" },
    {Mult, "*" },
    {Not, "!" },
    {Plus, "+" },
    {PlusAssign, "+=" },
    {Power, "**" },
    {ShiftLeft, "<<" },
    {ShiftRight, ">>" },
};

}

/// Base class for all operators.
class Operator
{
public:
    Operator(operator_::Kind kind);
    virtual ~Operator();

    /// Returns the operator's type.
    operator_::Kind kind() { return _kind; }

    /// Returns true if the given expressions are compatible with this
    /// operator.
    bool match(shared_ptr<Expression> result, expression_list& ops);

    typedef shared_ptr<expression::ResolvedOperator> (*expression_factory)(shared_ptr<Operator> op, const expression_list& ops, const Location& l);

    /// Returns a textual representation of the operator suitable for
    /// inclusion in messages to the user.
    const string& render() const;

protected:
    /// Returns the current validator.
    passes::Validator* validator() const { return _validator; }

    /// Method to report errors found by validate(). This forwards to the
    /// current passes::Validator.
    ///
    /// op: The AST node triggering the error.
    ///
    /// msg: An error message for the user.
    void error(Node* op, string msg) const;

    /// Method to report errors found by validate(). This forwards to the
    /// current passes::Validator.
    ///
    /// op: The AST node triggering the error.
    ///
    /// msg: An error message for the user.
    void error(shared_ptr<Node> op, string msg) const {
       return error(op.get(), msg);
    }

    /// Returns a factory function that instantiates a resolved operator
    /// expression for this operator with the given operands.
    expression_factory factory() const { return _factory; }

    /// Validates whether a set of operands is valid for the instruction.
    /// Errors are reported via the given passes::Validator instance (which is
    /// also normally the one calling this method).
    ///
    /// v: The validator pass.
    ///
    /// ops: The operands to check.
    void validate(passes::Validator* v, const expression_list& ops) {
       _validator = v;
       __validate(ops);
       _validator = nullptr;
    }

    // The follwing may be overridden by derived classes via macros.
    virtual shared_ptr<Type> __typeOp1() const    { return nullptr; }
    virtual shared_ptr<Type> __typeOp2() const    { return nullptr; }
    virtual shared_ptr<Type> __typeOp3() const    { return nullptr; }

    virtual bool                   __match(const expression_list& ops) { return true; };
    virtual void                   __validate(const expression_list& ops) { };
    virtual shared_ptr<Expression> __simplify(const expression_list& exprs) const { return nullptr; }
    virtual string                 __doc() const { return "<No documentation>"; }
    virtual shared_ptr<Type>       __typeResult(const expression_list& exprs) const = 0;

private:
    operator_::Kind _kind;
    expression_factory _factory;
    passes::Validator* _validator = 0;
};

}

#endif
