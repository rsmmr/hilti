
#ifndef BINPAC_OPERATOR_H
#define BINPAC_OPERATOR_H

#include <unordered_map>

#include "common.h"
#include "type.h"

namespace binpac {

namespace passes { class Validator; }

namespace operator_ {

enum Kind {
    None = 0,
    Add,
    Attribute,
    AttributeAssign,
    BitAnd,
    BitOr,
    BitXor,
    Call,
    Coerce,
    Cast,
    DecrPostfix,
    DecrPrefix,
    Delete,
    Deref,
    Div,
    Equal,
    Greater,
    HasAttribute,
    IncrPostfix,
    IncrPrefix,
    In,
    Index,
    IndexAssign,
    LogicalAnd,
    LogicalOr,
    Lower,
    MethodCall,
    Minus,
    MinusAssign,
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

enum OpType { UNARY_PREFIX, UNARY_POSTFIX, BINARY, BINARY_COMMUTATIVE, OTHER };

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
    _OP(IncrPrefix, "++", UNARY_PREFIX),
    _OP(New, "new ", UNARY_PREFIX),
    _OP(Not, "!", UNARY_PREFIX),

    _OP(DecrPostfix, "--", UNARY_POSTFIX),
    _OP(IncrPostfix, "++", UNARY_POSTFIX),

    _OP(BitAnd, "&", BINARY),
    _OP(BitOr, "|", BINARY),
    _OP(BitXor, "^", BINARY),
    _OP(Div, "/", BINARY),
    _OP(Equal, "==", BINARY_COMMUTATIVE),
    _OP(Greater, ">", BINARY),
    _OP(In, "in", BINARY),
    _OP(LogicalAnd, "&&", BINARY),
    _OP(LogicalOr, "||", BINARY),
    _OP(Lower, "<", BINARY),
    _OP(Minus, "-", BINARY),
    _OP(MinusAssign, "-=", BINARY),
    _OP(Mod, "mod", BINARY),
    _OP(Mult, "*", BINARY_COMMUTATIVE),
    _OP(Plus, "+", BINARY),
    _OP(PlusAssign, "+=", BINARY),
    _OP(Power, "**", BINARY),
    _OP(ShiftLeft, "<<", BINARY),
    _OP(ShiftRight, ">>", BINARY),

    _OP(Add, "<Add>", OTHER),
    _OP(Attribute, "<Attribute>", OTHER),
    _OP(AttributeAssign, "<AttributeAssign>", OTHER),
    _OP(Call, "<Call>", OTHER),
    _OP(Coerce, "<Coerce>", OTHER),
    _OP(Cast, "<Cast>", OTHER),
    _OP(Delete, "<Delete>", OTHER),
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
    typedef shared_ptr<expression::ResolvedOperator> (*expression_factory)(shared_ptr<Operator> op, const expression_list& ops, shared_ptr<Module> module, const Location& l);

    Operator(operator_::Kind kind, expression_factory factory);
    virtual ~Operator();

    /// Returns the operator's type.
    operator_::Kind kind();

    /// Returns true if the given expressions are compatible with this
    /// operator.
    ///
    /// ops: The operands.
    ///
    /// new_ops: TODO.
    ///
    /// coerce: True if coercing the operands for a match is ok.
    bool match(const expression_list& ops, bool coerce, expression_list* new_ops);

    /// Checks the correct usage of the operator, given a set of operands and
    /// the expected result type.
    ///
    /// validator: The validator to use for error reporting.
    ///
    /// result: Expected result type.
    ///
    /// ops: The operands.
    ///
    /// \todo: \a result is currently unused and I believe we can remove it. Right?
    void validate(passes::Validator* vld, shared_ptr<Type> result, const expression_list& ops);

    /// Returns the operator's result type, given a set of operands.
    ///
    /// ops: The operands.
    ///
    /// Returns: The result type.
    shared_ptr<Type> type(shared_ptr<Module> module, const expression_list& ops);

    /// Returns a textual representation of the operator suitable for
    /// inclusion in messages to the user.
    string render() const;

    struct Info {
        operator_::Kind kind;         /// The operator's kind.
        string namespace_;            /// The namespace is the operator is defined in.
        string kind_txt;              /// A textual rendering of the kind.
        string description;           /// Textual description of the operator.
        string render;                /// Textual rendering of the operator.
        shared_ptr<Type> type_op1;              /// Type of the operator's first operand.
        shared_ptr<Type> type_op2;              /// Type of the operator's second operand.
        shared_ptr<Type> type_op3;              /// Type of the operator's third operand.
        shared_ptr<Type> type_result;           /// Type of the operator's result.
        std::pair<string, shared_ptr<Type>> type_callarg1; /// Name and type of first method call argument.
        std::pair<string, shared_ptr<Type>> type_callarg2; /// Name and type of first method call argument.
        std::pair<string, shared_ptr<Type>> type_callarg3; /// Name and type of first method call argument.
        std::pair<string, shared_ptr<Type>> type_callarg4; /// Name and type of first method call argument.
        std::pair<string, shared_ptr<Type>> type_callarg5; /// Name and type of first method call argument.
        string class_op1;             /// Name of class of the operator's first operand.
        string class_op2;             /// Name of class of the operator's second operand.
        string class_op3;             /// Name of class of the operator's third operand.

        // Note, we don't store the result's type here as that's dynamically computed.
    };

    /// Returns information about the operator for use in the reference documentation.
    Info info() const;

    /// Returns the current validator.
    passes::Validator* validator() const { return _validator; }

    /// Returns the current set of operands. This is only defined while
    /// executing one of the virtual methods working on operands.
    const expression_list& operands() const { assert(__operands.size()); return __operands.back(); }

    /// Returns the current 1st operand. This is only defined while executing
    /// one of the virtual methods working on operands.
    const shared_ptr<Expression> op1() const { assert(operands().size() >= 1); auto i = operands().begin(); return *i; }

    /// Returns the current 2nd operand. This is only defined while executing
    /// one of the virtual methods working on operands.
    const shared_ptr<Expression> op2() const { assert(operands().size() >= 2); auto i = operands().begin(); ++i; return *i; }

    /// Returns the current 3rd operand. This is only defined while executing
    /// one of the virtual methods working on operands.
    const shared_ptr<Expression> op3() const { assert(operands().size() >= 3); auto i = operands().begin(); ++i; ++i; return *i; }

    /// Returns true if the operator has a given operand defined.
    ///
    /// n: The operand in the range from 1 to 3.
    bool hasOp(int n);

    /// Checks whether an operand can be coerced into a given type. If not,
    /// an error is reported.
    ///
    /// op: The operand
    ///
    /// type: The type.
    ///
    /// Returns: True if coercable.
    bool canCoerceTo(shared_ptr<Expression> op, shared_ptr<Type> type);

    /// Checks whether two types are equal. If not, an error is reported.
    ///
    /// type1: The first type.
    ///
    /// type2: The seconf type.
    ///
    /// Returns: True if coercable.
    bool sameType(shared_ptr<Type> type1, shared_ptr<Type> type2);

    /// Checks whether an element expression can be coerced into a
    /// container's element type. If not, an error is reported.
    ///
    /// element: The expression which's type to match against the container's
    /// element type.
    ///
    /// container: The container type, which must be of trait of
    /// type::trait::Container.
    ///
    /// Returns: True if the element is compatible with the container..
    bool matchesElementType(shared_ptr<Expression> element, shared_ptr<Type> container);

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

    /// Helper method to check that an argument tuple is of an expected type
    /// for a function or method call.. If not, it reports an error. Note
    /// that this method expects a constant tuple.
    ///
    /// tuple: The tuple expression; must be of a \a expression::Constant and
    ///        of type \a type::Tuple, otherwise will abort. Can be null to
    ///        specify an empty tuple.
    ///
    /// types: The expected types of the tuple elements.
    ///
    /// Returns: True if all tuple elements can be coerced to their
    /// corresponding type.
    bool checkCallArgs(shared_ptr<Expression> tuple, const type_list& types);

    /// Helper method to match that an argument tuple is of an expected type
    /// for a function or method call. If not, it return false, but does not report an error.
    ///
    /// tuple: The tuple expression; must be of a \a expression::Constant and
    ///        of type \a type::Tuple, otherwise will abort. Can be null to
    ///        specify an empty tuple.
    ///
    /// types: The expected types of the tuple elements.
    ///
    /// Returns: True if all tuple elements can be coerced to their
    /// corresponding type.
    bool matchCallArgs(shared_ptr<Expression> tuple, const type_list& types);

protected:
    /// Returns a factory function that instantiates a resolved operator
    /// expression for this operator with the given operands.
    expression_factory factory() const { return _factory; }

    /// For method calls, returns a list of argument types.
    type_list _callArgs() const;

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
    virtual shared_ptr<Type> __typeOp3() const; // This one default to type::Tuple for MethodCalls.
    virtual shared_ptr<Type> __typeResult() const = 0;

    virtual std::pair<string, shared_ptr<Type>> __typeCallArg1() const { return std::make_pair("", nullptr); }
    virtual std::pair<string, shared_ptr<Type>> __typeCallArg2() const { return std::make_pair("", nullptr); }
    virtual std::pair<string, shared_ptr<Type>> __typeCallArg3() const { return std::make_pair("", nullptr); }
    virtual std::pair<string, shared_ptr<Type>> __typeCallArg4() const { return std::make_pair("", nullptr); }
    virtual std::pair<string, shared_ptr<Type>> __typeCallArg5() const { return std::make_pair("", nullptr); }

    virtual bool                   __match() { return true; };
    virtual void                   __validate() { };
    virtual string                 __doc() const { return "<No documentation>"; }
    virtual string                 __namespace() const { return "<no namespace>"; }

    virtual shared_ptr<Type> __docTypeOp1() const    { return __typeOp1() ? __typeOp1() : nullptr; }
    virtual shared_ptr<Type> __docTypeOp2() const    { return __typeOp2() ? __typeOp2() : nullptr; }
    virtual shared_ptr<Type> __docTypeOp3() const    { return __typeOp3() ? __typeOp3() : nullptr; }
    virtual shared_ptr<Type> __docTypeResult() const  { return nullptr; }

    virtual std::pair<string, shared_ptr<Type>> __docTypeCallArg1() const    {
        auto t = __typeCallArg1();
        return t.second ? t : std::make_pair(t.first, nullptr);
    }

    virtual std::pair<string, shared_ptr<Type>> __docTypeCallArg2() const    {
        auto t = __typeCallArg2();
        return t.second ? t : std::make_pair(t.first, nullptr);
    }

    virtual std::pair<string, shared_ptr<Type>> __docTypeCallArg3() const    {
        auto t = __typeCallArg3();
        return t.second ? t : std::make_pair(t.first, nullptr);
    }

    virtual std::pair<string, shared_ptr<Type>> __docTypeCallArg4() const    {
        auto t = __typeCallArg4();
        return t.second ? t : std::make_pair(t.first, nullptr);
    }

    virtual std::pair<string, shared_ptr<Type>> __docTypeCallArg5() const    {
        auto t = __typeCallArg5();
        return t.second ? t : std::make_pair(t.first, nullptr);
    }

private:
    void pushOperands(const expression_list& ops) { __operands.push_back(ops); }
    void popOperands() { __operands.pop_back(); }
    std::pair<shared_ptr<Node>, string> matchArgsInternal(shared_ptr<Expression> tuple, const type_list& types);

    friend class OperatorRegistry;

    operator_::Kind _kind;
    expression_factory _factory;

    passes::Validator* _validator = 0;
    std::list<expression_list> __operands;
};

/// Singleton class that provides a register of all available BinPAC++
/// operators.
class OperatorRegistry
{
public:
    OperatorRegistry();
    ~OperatorRegistry();

    typedef std::unordered_multimap<operator_::Kind, shared_ptr<Operator>, std::hash<int>> operator_map;
    typedef std::list<std::pair<shared_ptr<Operator>, expression_list>> matching_result;

    /// Returns a list of operators of a given Kind matching a set of
    /// operands. This is the main function for overload resolution.
    ///
    /// kind: The name to look for.
    ///
    /// ops: The operands to match all the instructions with the given name for.
    ///
    /// try_coercion: If true and there's not direct match for the operands'
    /// types, the method also tries to coerce the operands as necessary to
    /// get a match.
    ///
    /// Returns: A list of (operators, operands) pairs. The operands are the
    /// ones found to be compatible, and each comes with a set of operands to
    /// use with them. Usally, these operands are just those passed into the
    /// method, however the matching process may adapt them to make them
    /// compatible.
    matching_result getMatching(operator_::Kind kind, const expression_list& ops, bool try_coercion = true, bool try_commutative = true) const;

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
    shared_ptr<expression::ResolvedOperator> resolveOperator(shared_ptr<Operator> op, const expression_list& ops, shared_ptr<Module> module, const Location& l = Location::None);

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
