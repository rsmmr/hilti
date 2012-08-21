
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

enum OpType { UNARY_PREFIX, UNARY_POSTFIX, BINARY, OTHER };

#define _OP(kind, display, type) {kind, OperatorDef(kind, #kind, display, type)}

struct OperatorDef {
    Kind kind;
    string kind_txt;
    string display;
    OpType type;

    OperatorDef(Kind kind, string kind_txt, string display, OpType type)
        : kind(kind), kind_txt(kind_txt), display(display), type(type) {}
};

// These are used by the printer.
const std::unordered_map<Kind, OperatorDef, std::hash<int>> OperatorDefinitions = {
    _OP(SignNeg, "-", UNARY_PREFIX),
    _OP(SignPos, "-", UNARY_PREFIX),
    _OP(Deref, "*", UNARY_PREFIX),
    _OP(DecrPrefix, "--", UNARY_PREFIX),
    _OP(IncrPrefix, "--", UNARY_PREFIX),

    _OP(DecrPostfix, "--", UNARY_POSTFIX),
    _OP(IncrPostfix, "--", UNARY_POSTFIX),

    _OP(Assign, "=", BINARY),
    _OP(BitAnd, "&", BINARY),
    _OP(BitOr, "|", BINARY),
    _OP(BitXor, "^", BINARY),
    _OP(Div, "/", BINARY),
    _OP(Equal, "==", BINARY),
    _OP(Greater, ">", BINARY),
    _OP(IncrPrefix, "++", BINARY),
    _OP(LogicalAnd, "&&", BINARY),
    _OP(LogicalOr, "||", BINARY),
    _OP(Lower, "<", BINARY),
    _OP(Minus, "-", BINARY),
    _OP(Mod, "mod", BINARY),
    _OP(Mult, "*", BINARY),
    _OP(Not, "!", BINARY),
    _OP(Plus, "+", BINARY),
    _OP(PlusAssign, "+=", BINARY),
    _OP(Power, "**", BINARY),
    _OP(ShiftLeft, "<<", BINARY),
    _OP(ShiftRight, ">>", BINARY),

    _OP(Attribute, "<Attribute>", OTHER),
    _OP(Call, "<Call>", OTHER),
    _OP(HasAttribute, "<HasAttribute>", OTHER),
    _OP(Index, "<INdex>", OTHER),
    _OP(IndexAssign, "<IndexAssign>", OTHER),
    _OP(MethodCall, "<MethodCall>", OTHER),
    _OP(Size, "<Size>", OTHER),
};

}

class OperatorRegistry;

/// Base class for all operators.
class Operator
{
public:
    typedef shared_ptr<expression::ResolvedOperator> (*expression_factory)(shared_ptr<Operator> op, const expression_list& ops, const Location& l);

    Operator(operator_::Kind kind, expression_factory factory);
    virtual ~Operator();

    /// Returns the operator's type.
    operator_::Kind kind();

    /// Returns true if the given expressions are compatible with this
    /// operator.
    ///
    /// ops: The operands.
    ///
    /// coerce: True if coercing the operands for a match is ok.
    bool match(const expression_list& ops, bool coerce);

    /// Checks the correct usage of the operator, given a set of operands and
    /// the expected result type.
    ///
    /// validator: The validator to use for error reporting.
    ///
    /// result: Expected result type.
    ///
    /// ops: The operands.
    void validate(passes::Validator* vld, shared_ptr<Type> result, const expression_list& ops);

    /// Returns the operator's result type, given a set of operands.
    ///
    /// ops: The operands.
    ///
    /// Returns: The result type.
    shared_ptr<Type> type(const expression_list& ops);

    /// Returns a textual representation of the operator suitable for
    /// inclusion in messages to the user.
    string render() const;

    struct Info {
        operator_::Kind kind;         /// The operator's kind.
        string kind_txt;              /// A textual rendering of the kind.
        string description;           /// Textual description of the operator.
        string render;                /// Textual rendering of the operator.
        shared_ptr<Type> type_op1;    /// Type of the operator's first operand.
        shared_ptr<Type> type_op2;    /// Type of the operator's second operand.
        shared_ptr<Type> type_op3;    /// Type of the operator's third operand.
        string class_op1;             /// Name of class of the operator's first operand.
        string class_op2;             /// Name of class of the operator's second operand.
        string class_op3;             /// Name of class of the operator's third operand.

        // Note, we don't store the result's type here as that's dynamically computed.
    };

    /// Returns information about the operator for use in the reference documentation.
    Info info() const;

protected:
    /// Returns the current validator.
    passes::Validator* validator() const { return _validator; }

    /// Returns the current set of operands. This is only defined while
    /// executing one of the virtual methods working on operands.
    const expression_list& operands() const { return __operands; }

    /// Returns the current 1st operand. This is only defined while executing
    /// one of the virtual methods working on operands.
    const shared_ptr<Expression> op1() const { assert(__operands.size() >= 1); auto i = __operands.begin(); return *i; }

    /// Returns the current 2nd operand. This is only defined while executing
    /// one of the virtual methods working on operands.
    const shared_ptr<Expression> op2() const { assert(__operands.size() >= 2); auto i = __operands.begin(); ++i; return *i; }

    /// Returns the current 3rd operand. This is only defined while executing
    /// one of the virtual methods working on operands.
    const shared_ptr<Expression> op3() const { assert(__operands.size() >= 3); auto i = __operands.begin(); ++i; ++i; return *i; }

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
    void error(shared_ptr<Node> op, string msg) const;

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
    void validate(passes::Validator* v, const expression_list& ops);

    // The follwing may be overridden by derived classes via macros.
    virtual shared_ptr<Type> __typeOp1() const    { return nullptr; }
    virtual shared_ptr<Type> __typeOp2() const    { return nullptr; }
    virtual shared_ptr<Type> __typeOp3() const    { return nullptr; }
    virtual shared_ptr<Type> __typeResult() const = 0;

    virtual bool                   __match() { return true; };
    virtual void                   __validate() { };
    virtual string                 __doc() const { return "<No documentation>"; }

private:
    friend class OperatorRegistry;

    operator_::Kind _kind;
    expression_factory _factory;

    passes::Validator* _validator = 0;
    expression_list __operands;
};

/// Singleton class that provides a register of all available BinPAC++
/// operators.
class OperatorRegistry
{
public:
    OperatorRegistry();
    ~OperatorRegistry();

    typedef std::unordered_multimap<operator_::Kind, shared_ptr<Operator>, std::hash<int>> operator_map;

    /// Returns a list of operators of a given Kind matching a set of
    /// operands. This is the main function for overload resolution.
    ///
    /// kind: The name to look for.
    ///
    /// ops: The operands to match all the instructions with the given name for.
    ///
    /// Returns: A list of instructions compatatible with \a id and \a ops.
    operator_list getMatching(operator_::Kind kind, const expression_list& ops) const;

    /// Returns all operators of a given kind.
    ///
    /// kind: The kind
    ///
    /// Returns: The operators.
    operator_list byKind(operator_::Kind kind) const;

    /// Instantiates a expression::ResolvedOperator for a kind/operands
    /// combination.
    ///
    /// operator: The operator to instantiate an expression for.
    ///
    /// ops: The operands, which must match with what \a operator expects.
    ///
    /// l: Associated location information.
    ///
    /// Returns: The resolved operator.
    shared_ptr<expression::ResolvedOperator> resolveOperator(shared_ptr<Operator> op, const expression_list& ops, const Location& l = Location::None);

    /// Registers an operator with the registry. This will be called from the
    /// autogenerated operator code.
    void addOperator(shared_ptr<Operator> op);

    /// Returns a map of all operators, indexed by their kind.
    const operator_map& getAll() const;

    /// Return the singleton for the global instruction registry.
    static OperatorRegistry* globalRegistry();

private:
    operator_map _operators;

    static OperatorRegistry* _registry; // Singleton.
};

}

#endif
