
#ifndef HILTI_BUILDER_H
#define HILTI_BUILDER_H

#include "hilti/hilti-intern.h"

namespace hilti {

namespace builder {

template<typename T>
shared_ptr<T> _sptr(T* ptr) { return shared_ptr<T>(ptr); }

typedef std::list<shared_ptr<hilti::Type>> type_list;

typedef AttributeSet attribute_set;

namespace expression {

/// Instantiates the default value of a type.
///
/// t: The type.
///
/// l: Location associated with the type.
///
/// Returns: The expressoin
inline shared_ptr<hilti::Expression> default_(shared_ptr<hilti::Type> t, const Location& l=Location::None) {
    return std::make_shared<hilti::expression::Default>(t, l);
}

}

namespace any {

/// Instantiates an type::Any type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Any> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Any>();
}

}

namespace void_ {

/// Instantiates a type::Void type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Void> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Void>();
}

}

namespace caddr {

/// Instantiates an AST expression node representing a caddr null value.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const Location& l=Location::None)
{
    auto c = std::make_shared<constant::CAddr>(l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates a type::Void type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::CAddr> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::CAddr>();
}

}

namespace integer {

/// Instantiates an AST expression node representing an integer constant. The
/// constant will be created with 64 bit width (which can normally be coerced
/// to types of appropiate smaller width automatically).
///
/// i: The value of the constant.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(int64_t i, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Integer>(i, 64, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an type::Integer type.
///
/// width: The bit width.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Integer> type(int width, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Integer>(width, l);
}

}

namespace string {

/// Instantiates an AST expression node representing a string constant.
///
/// s: The string value.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const ::string& s, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::String>(s, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates a type::String type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::String> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::String>(l);
}

}

namespace boolean {

/// Instantiates an AST expression node representing a boolean constant.
///
/// b: The boolean value.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(bool b, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Bool>(b, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates a type::Bool type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Bool> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Bool>(l);
}

}

namespace double_ {

/// Instantiates an AST expression node representing a double constant.
///
/// d: The double value.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(double d, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Double>(d, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates a type::Double type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Double> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Double>(l);
}

}

namespace tuple {

typedef hilti::constant::Tuple::element_list element_list;

/// Instantiates an AST expression node representing a tuple constant.
///
/// elems: The tuple elements.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const element_list elems, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Tuple>(elems, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates a type::Tuple type.
///
/// types: The types of the tuple's elements.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Tuple> type(const type_list& types, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Tuple>(types, l);
}

/// Instantiates a type::Tuple type that matches any other tuple type (i.e., \c tuple<*>).
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Tuple> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Tuple>(l);
}

}

namespace bytes {

/// Instantiates an AST expression node representing a bytes ctor.
///
/// b: The bytes value.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(const ::string& b, const Location& l=Location::None)
{
    auto c = std::make_shared<ctor::Bytes>(b, l);
    return std::make_shared<hilti::expression::Ctor>(c, l);
}

/// Instantiates a type::Bytes type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Bytes> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Bytes>(l);
}

}

namespace reference {

/// Instantiates an AST expression node representing a null reference.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> createNull(const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Reference>(l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates a type::Reference type.
///
/// rtype: The referenced type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Reference> type(shared_ptr<hilti::Type> rtype, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Reference>(rtype, l);
}

/// Instantiates a type::Reference type that matches any other reference type (i.e., \c ref<*>).
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Reference> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Reference>(l);
}

}

namespace iterator {

/// Instantiates a type::Iterator for a given target type.
///
/// ttype: The target type. This must be an type::trait::Iterable.
///
/// l: Location associated with the type.
///
/// Returns: The type node. Returns null if \c l is not iterable.
inline shared_ptr<hilti::Type> type(shared_ptr<Type> type, const Location& l=Location::None) {

    if ( ! type::hasTrait<type::trait::Iterable>(type) )
        return nullptr;

    return ast::as<type::trait::Iterable>(type)->iterType();
}

/// Instantiates a type::Iterator type that matches any other iterator type (i.e., \c iter<*>).
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Iterator> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Iterator>(l);
}

/// Instantiates a type::iterator::Bytes.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Iterator> typeBytes(const Location& l=Location::None) {
    auto ty = shared_ptr<Type>(new type::Bytes(l));
    return std::make_shared<hilti::type::iterator::Bytes>(l);
}

}

namespace label {

/// Instantiates an AST expression node representing a label constant.
///
/// id: The label ID.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const ::string& id, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Label>(id, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates a type::Label type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Label> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Label>(l);
}


}

namespace codegen {

/// Instantiates an AST expression node representing a CodeGen internal expression.
///
/// t: The type of the expression.
///
/// cookie: The cookie value to store with the expression.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::CodeGen> create(shared_ptr<hilti::Type> t, void* cookie, const Location& l=Location::None)
{
    return std::make_shared<hilti::expression::CodeGen>(t, cookie, l);
}

}

namespace module {

/// Instantiates an AST node representing a top-level module.
///
/// ctx: The compiler context the module is part of.
///
/// id: The name of the module.
///
/// file: The filename associated with the module. This should include the
/// full path to find the corresponding file.
///
/// l: Location associated with the type.
///
/// Returns: The module node.
inline shared_ptr<Module> create(shared_ptr<CompilerContext> ctx, shared_ptr<ID> id, ::string file = "-", const Location& l=Location::None) {
    return std::make_shared<Module>(ctx, id, file, l);
}

}

namespace global {

/// Instantiates an AST node representating the declaration of a global
/// variable.
///
/// id: The name of the veriable.
///
/// type: The type of the variable.
///
/// init: An optional expression to initialize the variable with.
///
/// l: Location associated with the type.
///
/// Returns: The declaration node.
inline shared_ptr<declaration::Variable> variable(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, const Location& l=Location::None) {
    auto var = std::make_shared<hilti::variable::Global>(id, type, init, l);
    return std::make_shared<declaration::Variable>(id, var, l);
}

/// Instantiates an AST node representating the declaration of a global named
/// type.
///
/// id: The name of the type.
///
/// type: The type being declared..
///
/// l: Location associated with the type.
///
/// Returns: The declaration node.
inline shared_ptr<declaration::Type> type(shared_ptr<ID> id, shared_ptr<Type> type, const Location& l=Location::None) {
    return std::make_shared<declaration::Type>(id, type, l);
}

}

namespace type {

/// Instantiates an AST expression representing a type.
///
/// t: The type.
///
/// l: Location associated with the type.
///
/// Returns: The result node.
inline shared_ptr<hilti::expression::Type> create(const shared_ptr<Type> t, Location l=Location::None) {
    return std::make_shared<hilti::expression::Type>(t, l);
}

/// Creates a reference to a type via its name. The named type must be
/// defined elsewhere via a declaration::Type, and the resolver must be run
/// to eventually link the two.
inline shared_ptr<hilti::Type> byName(shared_ptr<ID> id, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Unknown>(id, l);
}

/// Creates a reference to a type via its name. The named type must be
/// defined elsewhere via a declaration::Type, and the resolver must be run
/// to eventually link the two.
inline shared_ptr<hilti::Type> byName(const std::string& name, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Unknown>(std::make_shared<ID>(name, l), l);
}

/// Instantiates a type::Type type.
///
/// ttype: The typed type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::TypeType> type(shared_ptr<hilti::Type> ttype, const Location& l=Location::None) {
    return std::make_shared<hilti::type::TypeType>(ttype, l);
}

/// Instantiates wildcard type::Type type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::TypeType> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::TypeType>(l);
}

}


namespace local {

/// Instantiates an AST node representating the declaration of a local
/// variable.
///
/// id: The name of the veriable.
///
/// type: The type of the variable.
///
/// init: An optional expression to initialize the variable with.
///
/// l: Location associated with the type.
///
/// Returns: The declaration node.
inline shared_ptr<declaration::Variable> variable(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, const Location& l=Location::None) {
    auto var = std::make_shared<hilti::variable::Local>(id, type, init, l);
    return std::make_shared<declaration::Variable>(id, var, l);
}

}

namespace id {

/// Instantiates an AST expression node representing an ID reference.
///
/// id: The ID being referenced.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::ID> create(shared_ptr<ID> id, const Location& l=Location::None) {
    return std::make_shared<hilti::expression::ID>(id, l);
}

/// Instantiates an AST expression node representing an ID reference.
///
/// s: The name of the ID, either scoped or unscoped.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::ID> create(::string s, const Location& l=Location::None) {
    return create(std::make_shared<ID>(s, l));
}

/// Instantiates an AST node representating an ID.
///
/// s: The name of the ID, either scoped or unscoped.
///
/// l: Location associated with the type.
///
/// Returns: The ID node.
inline shared_ptr<ID> node(::string s, const Location& l=Location::None) {
    return std::make_shared<ID>(s, l);
}

}

namespace unset {

/// Instantiates an AST expression node representing the constant marking an
/// unset value.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Unset>(l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

}

namespace instruction {

/// Instantiates an AST statement node representing an instruction. This
/// returns an statement::instruction::Unresolved instruction, which the
/// passes::IdResolver will need to resolve before the AST can be used
/// further.
///
/// mnemonic: The name of the instruction.
///
/// target: Expression representing the target operand, or null if none.
///
/// op1: Expression representing the 1st operand, or null if none.
///
/// op2: Expression representing the 2ndoperand, or null if none.
///
/// op3: Expression representing the 3rd operand, or null if none.
///
/// l: Location associated with the type.
///
/// Returns: The statement node.
inline shared_ptr<statement::Instruction> create(shared_ptr<ID> mnemonic,
                                                 shared_ptr<Expression> target = nullptr,
                                                 shared_ptr<Expression> op1 = nullptr,
                                                 shared_ptr<Expression> op2 = nullptr,
                                                 shared_ptr<Expression> op3 = nullptr,
                                                 const Location& l=Location::None) {
    // FIXME: Seems we should be able to use an C++11 initializer_list
    // directly instead of calling this function. Doesn't get that to work
    // however, may not yet be supported by clang.
    hilti::instruction::Operands ops;
    ops.push_back(target);
    ops.push_back(op1);
    ops.push_back(op2);
    ops.push_back(op3);

    return std::make_shared<statement::instruction::Unresolved>(mnemonic, ops, l);
}

/// Instantiates an AST statement node representing an instruction. This
/// returns an statement::instruction::Unresolved instruction, which the
/// passes::IdResolver will need to resolve before the AST can be used
/// further.
///
/// mnemonic: The name of the instruction.
///
/// ops: The list of expressions representing target/op1/op2/op3,
/// respectively. To leave out the target operand, set it to null. To leave
/// out any of the others, either likewise use null or pass in a shorter list.
///
/// l: Location associated with the type.
///
/// Returns: The statement node.
inline shared_ptr<statement::Instruction> create(shared_ptr<ID> mnemonic, const hilti::instruction::Operands& ops, const Location& l=Location::None) {
    return std::make_shared<statement::instruction::Unresolved>(mnemonic, ops, l);
}

/// Instantiates an AST statement node representing an instruction. This
/// returns an statement::instruction::Unresolved instruction, which the
/// passes::IdResolver will need to resolve before the AST can be used
/// further.
///
/// mnemonic: The name of the instruction.
///
/// ops: The list of expressions representing target/op1/op2/op3,
/// respectively. To leave out the target operand, set it to null. To leave
/// out any of the others, either likewise use null or pass in a shorter list.
///
/// l: Location associated with the type.
///
/// Returns: The statement node.
inline shared_ptr<statement::Instruction> create(const ::string& mnemonic, const hilti::instruction::Operands& ops, const Location& l=Location::None) {
    auto id = shared_ptr<ID>(new ID(mnemonic, l));
    return create(id, ops, l);
}

}

namespace function {

typedef hilti::function::parameter_list parameter_list;

/// Instantiates an AST node representating the declaration of a function.
///
/// id: The name of the function.
///
/// result: The parameter representing the return value. Can be null for a
/// void function.
///
/// params: The parameters to the functions.
///
/// module: The module will function is declared in. Can be null if not yet
/// associated with a module.
///
/// cc: The function's calling convention.
///
/// scope: The function's scheduling scope.
///
/// body: The body of the function. Can be null to declare a function with
/// giving it a body (like for prototyping an external function).
///
/// l: Location associated with the type.
///
/// Returns: The function node.
inline shared_ptr<declaration::Function> create(shared_ptr<ID> id,
                                                shared_ptr<hilti::function::Result> result = nullptr,
                                                const hilti::function::parameter_list& params = hilti::function::parameter_list(),
                                                hilti::type::function::CallingConvention cc = hilti::type::function::HILTI,
                                                shared_ptr<statement::Block> body = nullptr,
                                                shared_ptr<Module> module = nullptr,
                                                const hilti::AttributeSet& attrs = hilti::AttributeSet(),
                                                const Location& l=Location::None) {

    if ( ! result )
        result = std::make_shared<hilti::function::Result>(builder::void_::type(), true);

    auto ftype = std::make_shared<hilti::type::HiltiFunction>(result, params, cc, l);
    ftype->setAttributes(attrs);

    auto func = std::make_shared<Function>(id, ftype, module, body, l);

    return std::make_shared<declaration::Function>(func, l);
}

/// Instantiates an AST node representing a function parameter for its type description.
///
/// id: The name of the parameter.
///
/// type: The type of the parameter.
///
/// constant: True if it's a constant parameter.
///
/// default: The paramemter's default value if any.
///
/// l: Location associated with the type.
///
/// Returns: The parameter node.
inline shared_ptr<hilti::function::Parameter> parameter(shared_ptr<ID> id, shared_ptr<Type> type, bool constant, shared_ptr<Expression> default_value, Location l=Location::None) {
    return std::make_shared<hilti::function::Parameter>(id, type, constant, default_value, l);
}

/// Instantiates an AST node representing a function parameter for its type description.
///
/// name: The name of the parameter.
///
/// type: The type of the parameter.
///
/// constant: True if it's a constant parameter.
///
/// default: The paramemter's default value if any.
///
/// l: Location associated with the type.
///
/// Returns: The parameter node.
inline shared_ptr<hilti::function::Parameter> parameter(const std::string& name, shared_ptr<Type> type, bool constant, shared_ptr<Expression> default_value, Location l=Location::None) {
    return std::make_shared<hilti::function::Parameter>(id::node(name), type, constant, default_value, l);
}

/// Instantiates an AST node representing a function result for its type
/// description.
///
/// type: The type of the result.
///
/// l: Location associated with the type.
///
/// Returns: The result node.
inline shared_ptr<hilti::function::Result> result(shared_ptr<Type> type, Location l=Location::None) {
    return std::make_shared<hilti::function::Result>(type, false, l);
}

/// Instantiates an AST node representing a function result for its type
/// description.
///
/// type: The type of the result.
///
/// constant: True if it's a constant result.
///
/// l: Location associated with the type.
///
/// Returns: The result node.
inline shared_ptr<hilti::function::Result> result(shared_ptr<Type> type, bool constant, Location l=Location::None) {
    return std::make_shared<hilti::function::Result>(type, constant, l);
}

/// Instantiates an type::Function type.
///
/// result: The parameter representing the return value. Can be null for a
/// void function.
///
/// params: The parameters to the functions.
///
/// cc: The function's calling convention.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Function> type(shared_ptr<hilti::function::Result> result = nullptr,
					      const hilti::function::parameter_list& params = hilti::function::parameter_list(),
					      hilti::type::function::CallingConvention cc = hilti::type::function::HILTI,
					      const Location& l=Location::None)
{
    return std::make_shared<hilti::type::HiltiFunction>(result, params, cc, l);
}

}

namespace hook {

typedef hilti::function::parameter_list parameter_list;

/// Instantiates an AST node representating the declaration of a hook implementation.
///
/// id: The name of the hook.
///
/// result: The parameter representing the return value.
///
/// params: The parameters to the hook.
///
/// module: The module the hook implementation is declared in.
///
/// body: The body of the function. Can be null to declare a function with
/// giving it a body (for prototyping a hook).
///
/// attrs: The attributes defined for the hook.
///
/// l: Location associated with the type.
///
/// Returns: The hook node.
inline shared_ptr<declaration::Hook> create(shared_ptr<ID> id,
                                            shared_ptr<hilti::function::Result> result,
                                            const hilti::function::parameter_list& params,
                                            const hilti::AttributeSet& attrs,
                                            shared_ptr<Module> module,
                                            shared_ptr<statement::Block> body,
                                            const Location& l=Location::None)
{
    auto ftype = std::make_shared<hilti::type::Hook>(result, params, l);
    ftype->setAttributes(attrs);

    auto func = std::make_shared<Hook>(id, ftype, module, body, l);
    return std::make_shared<declaration::Hook>(func, l);
}

/// Instantiates an type::Hook type.
///
/// result: The parameter representing the return value. Can be null for a
/// void function.
///
/// params: The parameters to the functions.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Function> type(shared_ptr<hilti::function::Result> result = nullptr,
					      const hilti::function::parameter_list& params = hilti::function::parameter_list(),
                          const Location& l=Location::None)
{
    return std::make_shared<hilti::type::Hook>(result, params, l);
}

}

namespace block {

typedef hilti::statement::Block::stmt_list statement_list;
typedef hilti::statement::Block::decl_list declaration_list;

/// Instantiates an AST statement node representing a statement block.
///
/// decls: A list of declarations that are part of the block.
///
/// stmts: A list of statements making up the block
///
/// parent: The parent scope.
///
/// l: Location associated with the block.
///
/// Returns: The block node.
inline shared_ptr<statement::Block> create(const declaration_list& decls, const statement_list& stmts, const shared_ptr<Scope>& parent, Location l=Location::None) {
    return std::make_shared<hilti::statement::Block>(decls, stmts, parent, nullptr, l);
}

/// Instantiates an AST statement node representing an empty statement block.
///
/// parent: The parent scope.
///
/// l: Location associated with the block.
///
/// Returns: The block node.
inline shared_ptr<statement::Block> create(const shared_ptr<Scope>& parent, Location l=Location::None) {
    return std::make_shared<hilti::statement::Block>(parent, nullptr, l);
}

typedef statement::Try::catch_list catch_list;

inline shared_ptr<statement::try_::Catch> catch_(shared_ptr<Type> type, shared_ptr<ID> id, shared_ptr<statement::Block> block, const Location& l)
{
    return std::make_shared<statement::try_::Catch>(type, id, block, l);
}

inline shared_ptr<statement::try_::Catch> catchAll(shared_ptr<statement::Block> block, const Location& l)
{
    return std::make_shared<statement::try_::Catch>(nullptr, nullptr, block, l);
}

inline shared_ptr<statement::Try> try_(shared_ptr<statement::Block> block, const catch_list& catches, const Location& l=Location::None)
{
    return std::make_shared<hilti::statement::Try>(block, catches, l);
}

}

namespace address {

/// Instantiates an AST expression node representing an Address constant.
///
/// addr: The address in dotted-quad (v4) or hex (v6) form.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const std::string& addr, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Address>(addr, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an type::Address type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Address> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Address>(l);
}

}

namespace network {

/// Instantiates an AST expression node representing a Network constant.
///
/// cidr: The network in CIDR format.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const std::string& cidr, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Network>(cidr, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an AST expression node representing a Network constant.
///
/// prefix: The prefix of the network.
///
/// width: The width of the network,
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const std::string& prefix, int width, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Network>(prefix, width, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an type::Network type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Network> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Network>(l);
}

}

namespace port {

/// Instantiates an AST expression node representing an Port constant.
///
/// port: A string of the form "<port>/<proto>".
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const std::string& port, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Port>(port, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an AST expression node representing an Port constant.
///
/// port: The port number.
///
/// proto: The ports protocol.
///
/// l: Location associated with the expression.
///
/// Throws: ConstantParseError if the \a port isn't parseable.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(int port, constant::PortVal::Proto proto, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Port>(port, proto, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an type::Port type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Port> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Port>(l);
}

}

namespace interval {

/// Instantiates an AST expression node representing an Interval constant.
///
/// secs: The seconds representing the interval.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(double secs, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Interval>(secs, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an AST expression node representing an Interval constant.
///
/// secs: The seconds representing the interval.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(uint64_t secs, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Interval>(secs, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an AST expression node representing an Interval constant.
///
/// nsecs: The seconds representing the interval.
///
/// takes_nsecs: Just a marker that this methods takes nsecs.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(uint64_t nsecs, bool takes_nsecs, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Interval>(nsecs, true, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an type::Interval type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Interval> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Interval>(l);
}

}

namespace time {

/// Instantiates an AST expression node representing an Time constant.
///
/// secs: Seconds since the epoch.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(double secs, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Time>(secs, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an AST expression node representing an Time constant.
///
/// secs: Seconds since the epoch.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(uint64_t secs, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Time>(secs, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an AST expression node representing an Time constant.
///
/// nsecs: Nano seconds since the epoch.
///
/// takes_nsecs: Just a marker that this methods takes nsecs.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(uint64_t nsecs, bool takes_nsecs, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Time>(nsecs, true, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an type::Time type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Time> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Time>(l);
}

}

namespace enum_ {

/// An enum label. The first element is the name and the second its
/// predefined value, or -1 to define automatically.
typedef std::pair<shared_ptr<ID>, int> Label;

/// A list of enum labels used to define an enum type.
typedef std::list<Label> label_list;

/// Instantiates an AST expression node representing an enum constant.
///
/// label: The label the constants represents.
///
/// etype: The enum type, which must be of type type::Enum.
///
/// l: Location associated with the node.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(shared_ptr<ID> label, shared_ptr<Type> etype, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Enum>(label, etype);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an type::Enum type.
///
/// labels: The labels of the type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Enum> type(const label_list& labels, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Enum>(labels);
}

}

namespace bitset {

/// A list of bitset labels, used to define a bitset constant.
typedef std::list<shared_ptr<ID>> bit_list;

/// An bitset label. The first element is the name and the second its
/// predefined value, or -1 to define automatically.
typedef std::pair<shared_ptr<ID>, int> Label;

/// A list of bitset labels used to define a bitset type.
typedef std::list<Label> label_list;

/// Instantiates an AST expression node representing an bitset constant.
///
/// bits: The bits set in the bitset.
///
/// btype: The bitset type, which must be of type type::Bitset.
///
/// l: Location associated with the node.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const bit_list bits, shared_ptr<Type> btype, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Bitset>(bits, btype);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an AST expression node representing an bitset constant.
///
/// bit: A single but set in the bitset.
///
/// btype: The bitset type, which must be of type type::Bitset.
///
/// l: Location associated with the node.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const shared_ptr<ID>& bit, shared_ptr<Type> btype, const Location& l=Location::None)
{
    bit_list bits;
    bits.push_back(bit);
    return create(bits, btype, l);
}

/// Instantiates an type::Bitset type.
///
/// labels: The labels of the type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Bitset> type(const label_list& labels, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Bitset>(labels);
}

}

namespace exception {

/// Instantiates an type::Exception type.
///
/// base: Another exception type this one derived from. If null, the
/// top-level base exception is used.
///
/// arg: The type of the exceptions argument, or null if none.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Exception> type(shared_ptr<Type> base, shared_ptr<Type> arg, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Exception>(base, arg, l);
}

/// Instantiates a type::Exception wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Exception> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Exception>(l);
}

}

namespace callable {

typedef hilti::ctor::Callable::argument_list argument_list;

/// Instantiates a type::Callable type.
///
/// rtype: The callable return type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
///
/// \note This is an old version of the builder function and deprecated.
inline shared_ptr<hilti::type::Callable> type(shared_ptr<hilti::Type> rtype,
                                              const Location& l=Location::None) {
    auto result = std::make_shared<hilti::function::Result>(rtype, false);
    hilti::function::parameter_list params;
    return std::make_shared<hilti::type::Callable>(result, params);
}

/// Instantiates a type::callable type.
///
/// result: The parameter representing the return value.
///
/// params: The parameters to the callable.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Function> type(shared_ptr<hilti::function::Result> result,
					      const hilti::function::parameter_list& params = hilti::function::parameter_list(),
					      const Location& l=Location::None)
{
    return std::make_shared<hilti::type::Callable>(result, params, l);
}

/// Instantiates a type::callable type.
///
/// result: The parameter representing the return value. Can be null for a
/// void function.
///
/// params: The type of the parameters to the callable.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Function> type(shared_ptr<Type> result,
					      const type_list& params,
					      const Location& l=Location::None)
{
    hilti::function::parameter_list p;

    int i = 0;
    for ( auto t : params ) {
        auto id = builder::id::node(::util::fmt("a%d", ++i));
        p.push_back(std::make_shared<hilti::function::Parameter>(id, t, false, nullptr));
    }

    return std::make_shared<hilti::type::Callable>(function::result(result, false), p, l);
}

/// Instantiates a type::Callable wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Callable> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Callable>(l);
}

/// Instantiates an AST expression node representing a Callable ctor.
///
/// rtype: The callable's function result type.
///
/// params: The callable's parameter types.
///
/// function: Function being bound.
///
/// args: The arguments of \a function to bind statically into the
/// callable. These must match the function's signature starting from the
/// left, but can leave some unbound.
///
/// l: An associated location.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(shared_ptr<hilti::type::function::Result> result,
                                                  const function::parameter_list& params,
                                                  shared_ptr<Expression> function,
                                                  const argument_list& args,
                                                  const Location& l=Location::None)
{
    auto c = std::make_shared<ctor::Callable>(result, params, function, args, l);
    return std::make_shared<hilti::expression::Ctor>(c, l);
}

/// Instantiates an AST expression node representing a Callable ctor.
///
/// rtype: The callable's function result type.
///
/// params: The callable's parameter types.
///
/// function: Function being bound.
///
/// args: The arguments of \a function to bind statically into the
/// callable. These must match the function's signature starting from the
/// left, but can leave some unbound.
///
/// l: An associated location.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(shared_ptr<Type> result,
                                                  const type_list& params,
                                                  shared_ptr<Expression> function,
                                                  const argument_list& args,
                                                  const Location& l=Location::None)
{
    auto ct = type(result, params, l);
    auto c = std::make_shared<ctor::Callable>(ct->result(), ct->Function::parameters(), function, args, l);
    return std::make_shared<hilti::expression::Ctor>(c, l);
}

}

namespace channel {

/// Instantiates a type::Channel type.
///
/// t: The channel's element type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Channel> type(shared_ptr<Type> type, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Channel>(type, l);
}

/// Instantiates a type::Channel wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Channel> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Channel>(l);
}

}

namespace classifier {

/// Instantiates a type::Classifier type.
///
/// rtype: The type for the classifier rules. This must be a reference to
/// heap type that's of trait trait::TypeList.
///
/// vtype: The type for values associated with rules.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Classifier> type(shared_ptr<Type> rtype, shared_ptr<Type> vtype, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Classifier>(rtype, vtype, l);
}

/// Instantiates a type::Classifier wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Classifier> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Classifier>(l);
}

}

namespace file {

/// Instantiates a type::File type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::File> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::File>(l);
}

}

namespace iosource {

/// Instantiates a type::IOSource type.
///
/// kind: An enum identifier referencing the type of IOSource
/// (``Hilti::IOSrc::*``).
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::IOSource> type(shared_ptr<ID> id, const Location& l=Location::None) {
    return std::make_shared<hilti::type::IOSource>(id, l);
}

/// Instantiates a type::IOSource wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::IOSource> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::IOSource>(l);
}

}

namespace list {

typedef ctor::List::element_list element_list;

/// Instantiates an AST expression node representing a list ctor.
///
/// etype: The type of the the list's elements.
///
/// elems: The list of elements.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(shared_ptr<Type> etype, const element_list& elems, const Location& l=Location::None)
{
    auto c = std::make_shared<ctor::List>(etype, elems, l);
    return std::make_shared<hilti::expression::Ctor>(c, l);
}

/// Instantiates a type::List type.
///
/// etype: The type of the elements.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::List> type(shared_ptr<Type> etype, const Location& l=Location::None) {
    return std::make_shared<hilti::type::List>(etype, l);
}

/// Instantiates a type::List wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::List> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::List>(l);
}

}

namespace set {

typedef ctor::Set::element_list element_list;

/// Instantiates an AST expression node representing a set ctor.
///
/// etype: The type of the sets's elements.
///
/// elems: The list of elements.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(shared_ptr<Type> etype, const element_list& elems, const Location& l=Location::None)
{
    auto c = std::make_shared<ctor::Set>(etype, elems, l);
    return std::make_shared<hilti::expression::Ctor>(c, l);
}

/// Instantiates an AST expression node representing a set ctor.
///
/// dummy: Dummy flag that differentiates this function from the one giving
/// the element type. The value is ignored.
///
/// stype: The type of the set, which must be \a type::Set.
///
/// elems: The list of elements.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(bool dummy, shared_ptr<Type> stype, const element_list& elems, const Location& l=Location::None)
{
    auto c = std::make_shared<ctor::Set>(true, stype, elems, l);
    return std::make_shared<hilti::expression::Ctor>(c, l);
}

/// Instantiates a type::Set type.
///
/// etype: The type of the elements.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Set> type(shared_ptr<Type> etype, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Set>(etype, l);
}

/// Instantiates a type::Set wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Set> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Set>(l);
}

}

namespace vector {

typedef ctor::Vector::element_list element_list;

/// Instantiates an AST expression node representing a vector ctor.
///
/// etype: The type of vector's elements.
///
/// elems: The list of elements.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(shared_ptr<Type> etype, const element_list& elems, const Location& l=Location::None)
{
    auto c = std::make_shared<ctor::Vector>(etype, elems, l);
    return std::make_shared<hilti::expression::Ctor>(c, l);
}

/// Instantiates a type::Vector type.
///
/// etype: The type of the elements.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Vector> type(shared_ptr<Type> etype, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Vector>(etype, l);
}

/// Instantiates a type::Vector wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Vector> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Vector>(l);
}

}

namespace expression {

typedef std::list<node_ptr<Expression>> list;

}

namespace map {

typedef ctor::Map::element_list element_list;

/// Instantiates an AST expression node representing a map ctor.
///
/// ktype: The type of the map's index values.
///
/// vtype: The type of the map's values.
///
/// elems: The list of elements.
///
/// def: If given a default value to return for unused indices.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(shared_ptr<Type> ktype, shared_ptr<Type> vtype, const element_list& elems, shared_ptr<Expression> def = nullptr, const AttributeSet& attrs = AttributeSet(), const Location& l=Location::None)
{
    auto c = std::make_shared<ctor::Map>(ktype, vtype, elems, l);

    c->setAttributes(attrs);

    if ( def )
        c->attributes().add(attribute::DEFAULT, def);

    return std::make_shared<hilti::expression::Ctor>(c, l);
}

/// Instantiates an AST expression node representing a map ctor.
///
/// mtype: The type of the map, which must be \a type::Map.
///
/// elems: The list of elements.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(shared_ptr<Type> mtype, const element_list& elems, const Location& l=Location::None)
{
    auto c = std::make_shared<ctor::Map>(mtype, elems, l);
    return std::make_shared<hilti::expression::Ctor>(c, l);
}

/// Instantiates an map element for use with create().
///
/// key: The key.
///
/// value: The value.
///
/// Returns: The element.
inline ctor::Map::element element(shared_ptr<Expression> key, shared_ptr<Expression> value)
{
    return std::make_pair(key, value);
}

/// Instantiates a type::Map type.
///
/// key: The type of the map's indices.
///
/// value: The type of the map's values.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Map> type(shared_ptr<Type> key, shared_ptr<Type> value, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Map>(key, value, l);
}

/// Instantiates a type::Map wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Map> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Map>(l);
}

}

namespace overlay {

typedef hilti::type::Overlay::field_list field_list;

/// Instantiates a type::Overlay type.
///
/// fields: The overlay's fields.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Overlay> type(const field_list& fields, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Overlay>(fields, l);
}

/// Instantiates a type::Overlay wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Overlay> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Overlay>(l);
}

/// Instanties a overlay field for its type description.
///
/// name: The name of the field.
///
/// type: The type of the field.
///
/// start: Offset in bytes form the start of the overlay where the field's
/// data starts.
///
/// fmt: An expression refering to a ``Hilti::Packed`` label defining the
/// format used for unpacking the field.
///
/// arg: An argument expression that might be need for the given \a fmt
/// unpack format.
///
/// l: Associated location.
inline shared_ptr<hilti::type::overlay::Field> field(shared_ptr<ID> name, shared_ptr<Type> type, int start, shared_ptr<Expression> fmt, shared_ptr<Expression> arg = nullptr, const Location& l=Location::None)
{
    return std::make_shared<hilti::type::overlay::Field>(name, type, start, fmt, arg, l);
}

/// Instanties a overlay field for its type description.
///
/// name: The name of the field.
///
/// type: The type of the field.
///
/// start: Name of an another field which this one follows immediately in
/// the overlay layout.
///
/// fmt: An expression refering to a ``Hilti::Packed`` label defining the
/// format used for unpacking the field.
///
/// arg: An argument expression that might be need for the given \a fmt
/// unpack format.
///
/// l: Associated location.
inline shared_ptr<hilti::type::overlay::Field> field(shared_ptr<ID> name, shared_ptr<Type> type, shared_ptr<ID> start, shared_ptr<Expression> fmt, shared_ptr<Expression> arg = nullptr, const Location& l=Location::None)
{
    return std::make_shared<hilti::type::overlay::Field>(name, type, start, fmt, arg, l);
}

}

namespace regexp {

typedef hilti::ctor::RegExp::pattern re_pattern;
typedef hilti::ctor::RegExp::pattern_list re_pattern_list;

/// Instantiates an AST expression node representing a RegExp ctor with a
/// series of patterns.
///
/// patterns: The patterns.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(const re_pattern_list& patterns,
                                                  const hilti::AttributeSet& attrs = hilti::AttributeSet(),
                                                  const Location& l=Location::None)
{
    auto c = std::make_shared<ctor::RegExp>(patterns, l);
    c->setAttributes(attrs);
    return std::make_shared<hilti::expression::Ctor>(c, l);
}

/// Instantiates an AST expression node representing a RegExp ctor with a single pattern.
///
/// patterns: The pattern. A pattern is a tuple of two ::strings.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(const re_pattern& pattern,
                                                  const hilti::AttributeSet& attrs = hilti::AttributeSet(),
                                                  const Location& l=Location::None)
{
    re_pattern_list patterns;
    patterns.push_back(pattern);
    return create(patterns, attrs, l);
}

/// Instantiates a type::RegExp type.
///
/// attrs: List of optional attributes controlling specifics of the regular
/// expression. These must include the ampersand, e.g., \c &nosub.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::RegExp> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::RegExp>(l);
}

}

namespace timer {

/// Instantiates a type::Timer type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Timer> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Timer>(l);
}

}

namespace timer_mgr {

/// Instantiates a type::TimerMgr type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::TimerMgr> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::TimerMgr>(l);
}

}

namespace match_token_state {

/// Instantiates a type::MatchTokenState type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::MatchTokenState> type(const Location& l=Location::None) {
    return std::make_shared<hilti::type::MatchTokenState>(l);
}

}

namespace struct_ {

typedef hilti::type::Struct::field_list field_list;
typedef tuple::element_list element_list;

/// Instantiates an AST expression node representing a struct ctor.
///
/// elems: The elements to initialize the struct with. To leave a field
/// unset, you an expression of type \a Unset.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const element_list& elems, const Location& l=Location::None)
{
    return tuple::create(elems, l);
}

/// Instantiates a type::Struct type.
///
/// fields: The struct's fields.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Struct> type(const field_list& fields, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Struct>(fields, l);
}

/// Instantiates a type::Struct wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Struct> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Struct>(l);
}

/// Instanties a struct field for its type description.
///
/// id:  The name of the field.
///
/// type: The type of the field.
///
/// default_: An optional default value, null if no default.
///
/// internal: If true, the field will not be printed when the struct
/// type is rendered as a string. Internal IDS are also skipped from
/// ctor expressions and list conversions.
///
/// l: Location associated with the field.
inline shared_ptr<hilti::type::struct_::Field> field(shared_ptr<ID> id, shared_ptr<hilti::Type> type, shared_ptr<Expression> default_ = nullptr, const AttributeSet& attrs = AttributeSet(), bool internal = false, const Location& l=Location::None)
{
    auto field = std::make_shared<hilti::type::struct_::Field>(id, type, internal, l);

    field->setAttributes(attrs);

    if ( default_ )
        field->attributes().add(attribute::DEFAULT, default_);

    return field;
}

/// Instanties a struct field for its type description.
///
/// id:  The name of the field.
///
/// type: The type of the field.
///
/// default_: An optional default value, null if no default.
///
/// internal: If true, the field will not be printed when the struct
/// type is rendered as a string. Internal IDS are also skipped from
/// ctor expressions and list conversions.
///
/// l: Location associated with the field.
inline shared_ptr<hilti::type::struct_::Field> field(shared_ptr<ID> id, shared_ptr<hilti::Type> type, shared_ptr<Expression> default_ = nullptr, bool internal = false, const Location& l=Location::None)
{
    return field(id, type, default_, AttributeSet(), internal, l);
}

/// Instanties a struct field for its type description.
///
/// name:  The name of the field.
///
/// type: The type of the field.
///
/// default_: An optional default value, null if no default.
///
/// internal: If true, the field will not be printed when the struct
/// type is rendered as a string. Internal IDS are also skipped from
/// ctor expressions and list conversions.
///
/// l: Location associated with the field.
inline shared_ptr<hilti::type::struct_::Field> field(const std::string& name, shared_ptr<hilti::Type> type, shared_ptr<Expression> default_, const AttributeSet& attrs, bool internal = false, const Location& l=Location::None)
{
    auto field = std::make_shared<hilti::type::struct_::Field>(id::node(name), type, internal, l);

    field->setAttributes(attrs);

    if ( default_ )
        field->attributes().add(attribute::DEFAULT, default_);

    return field;
}

/// Instanties a struct field for its type description.
///
/// name:  The name of the field.
///
/// type: The type of the field.
///
/// default_: An optional default value, null if no default.
///
/// internal: If true, the field will not be printed when the struct
/// type is rendered as a string. Internal IDS are also skipped from
/// ctor expressions and list conversions.
///
/// l: Location associated with the field.
inline shared_ptr<hilti::type::struct_::Field> field(const std::string& name, shared_ptr<hilti::Type> type, shared_ptr<Expression> default_ = nullptr, bool internal = false, const Location& l=Location::None)
{
    return field(name, type, default_, AttributeSet(), internal, l);
}

}

namespace union_ {

typedef hilti::type::Union::field_list field_list;

/// Instantiates a type::Union type with named fields
///
/// fields: The union's fields.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Union> type(const field_list& fields, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Union>(fields, l);
}

/// Instantiates a type::Union type with anonymous fields, based on types
/// only. This resembles a variant.
///
/// types: The unions's types.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Union> type(const type_list& types, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Union>(types, l);
}

/// Instantiates a type::Union wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Union> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Union>(l);
}

/// Instanties a union field for its type description.
///
/// id:  The name of the field.
///
/// type: The type of the field.
///
/// default_: An optional default value, null if no default.
///
/// internal: If true, the field will not be printed when the union
/// type is rendered as a string. Internal IDS are also skipped from
/// ctor expressions and list conversions.
///
/// l: Location associated with the field.
inline shared_ptr<hilti::type::union_::Field> field(shared_ptr<ID> id, shared_ptr<hilti::Type> type, shared_ptr<Expression> default_ = nullptr, const AttributeSet& attrs = AttributeSet(), bool internal = false, const Location& l=Location::None)
{
    auto field = std::make_shared<hilti::type::union_::Field>(id, type, internal, l);

    field->setAttributes(attrs);

    if ( default_ )
        field->attributes().add(attribute::DEFAULT, default_);

    return field;
}

/// Instanties a union field for its type description.
///
/// name:  The name of the field.
///
/// type: The type of the field.
///
/// default_: An optional default value, null if no default.
///
/// internal: If true, the field will not be printed when the union
/// type is rendered as a string. Internal IDS are also skipped from
/// ctor expressions and list conversions.
///
/// l: Location associated with the field.
inline shared_ptr<hilti::type::union_::Field> field(const std::string& name, shared_ptr<hilti::Type> type, const AttributeSet& attrs = AttributeSet(), const Location& l=Location::None)
{
    auto f = std::make_shared<hilti::type::union_::Field>(id::node(name), type, l);
    f->setAttributes(attrs);
    return f;
}

/// Instantiates an AST expression node representing a union constant that
/// has one named field set.
///
/// utype: The union's type. This must be a union with named fields. The type
/// can be null, in which case, the constant's type is derived as a union
/// with just a single field of type corresponding to that of *value*.
///
/// field: The ID of the field to initialize.
///
/// value: The value to set the field to.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(shared_ptr<Type> utype, shared_ptr<ID> field, shared_ptr<Expression> value, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Union>(utype, field, value, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an AST expression node representing a union constant that
/// has one named field set.
///
/// utype: The union's type. This must be a union with named fields. The type
/// can be null, in which case, the constant's type is derived as a union
/// with just a single field of type corresponding to that of *value*.
///
/// field: A constant string expression with the ID of the field to
/// initialize.
///
/// value: The value to set the field to.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(shared_ptr<Type> utype, shared_ptr<Expression> field, shared_ptr<Expression> value, const Location& l=Location::None)
{
    auto cexpr = ast::checkedCast<::hilti::expression::Constant>(field);
    auto cval = ast::checkedCast<::hilti::constant::String>(cexpr->constant());
    auto id = std::make_shared<::hilti::ID>(cval->value(), field->location());
    auto c = std::make_shared<constant::Union>(utype, id, value, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an AST expression node representing a union constant that
/// has an anoymous field set.
///
/// utype: The union's type. This must be a union with anoymous fields. The
/// type can be null, in which case, the constant's type is derived as a
/// union with just a single anonymous field of type corresponding to that of
/// *value*.
///
/// value: The value to set the union to.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(shared_ptr<Type> utype, shared_ptr<Expression> value, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Union>(utype, nullptr, value, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an AST expression node representing a union constant of a
/// single field.
///
/// value: The value to set the single union field to.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(shared_ptr<Expression> value, const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Union>(nullptr, nullptr, value, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

/// Instantiates an AST expression node representing an unset union constant of wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const Location& l=Location::None)
{
    auto c = std::make_shared<constant::Union>(nullptr, nullptr, nullptr, l);
    return std::make_shared<hilti::expression::Constant>(c, l);
}

}

////

namespace context {

typedef hilti::type::Struct::field_list field_list;

/// Instantiates a type::Context type.
///
/// fields: The context's fields.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Context> type(const field_list& fields, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Context>(fields, l);
}

/// Instantiates a type::Context wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Context> typeAny(const Location& l=Location::None) {
    return std::make_shared<hilti::type::Context>(l);
}

/// Instanties a context field for its type description.
///
/// id:  The name of the field.
///
/// type: The type of the field.
///
/// l: Location associated with the field.
inline shared_ptr<hilti::type::struct_::Field> field(shared_ptr<ID> id, shared_ptr<hilti::Type> type, const Location& l=Location::None)
{
    return std::make_shared<hilti::type::struct_::Field>(id, type, false, l);
}

/// Instanties a context field for its type description.
///
/// name:  The name of the field.
///
/// type: The type of the field.
///
/// l: Location associated with the field.
inline shared_ptr<hilti::type::struct_::Field> field(const std::string& name, shared_ptr<hilti::Type> type, const Location& l=Location::None)
{
    return std::make_shared<hilti::type::struct_::Field>(id::node(name), type, false, l);
}

}

/////

namespace scope {

/// A list of bitset labels used to define a bitset type.
typedef ::type::Scope::field_list field_list;

/// Instantiates an type::Scope type.
///
/// labels: The fields of the type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Scope> type(const field_list& fields, const Location& l=Location::None) {
    return std::make_shared<hilti::type::Scope>(fields);
}

}

/////

namespace loop {

/// Instantiates a for-each loop iterating over a sequence.
///
/// var: The iteration variable.
///
/// seq: The sequence to iterate over. The type must be of trait
/// trait::Iterable.
///
/// body: The loop body.
///
/// l: Associated location information.
inline shared_ptr<hilti::statement::ForEach> foreach(shared_ptr<ID> var, shared_ptr<Expression> seq, shared_ptr<statement::Block> body, const Location& l=Location::None)
{
    return std::make_shared<statement::ForEach>(var, seq, body, l);
}

/// Instantiates a "magic" block label that can be branched to inside a loop
/// for proceeding directly with the next iteration.
///
/// Returns: An expression refering the "magic" block.
inline shared_ptr<hilti::Expression> next(const Location& l=Location::None)
{
    return label::create(hilti::statement::foreach::IDLoopNext);
}

/// Instantiates a "magic" block label that can be branched to inside a loop
/// for aborting iteration.
///
/// Returns: An expression refering the "magic" block.
inline shared_ptr<hilti::Expression> break_(const Location& l=Location::None)
{
    return label::create(hilti::statement::foreach::IDLoopBreak);
}

}

}

}

#endif

