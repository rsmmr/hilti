
#ifndef HILTI_BUILDER_H
#define HILTI_BUILDER_H

#include "hilti.h"

namespace hilti {

namespace builder {

template<typename T>
shared_ptr<T> _sptr(T* ptr) { return shared_ptr<T>(ptr); }

typedef std::list<node_ptr<hilti::Type>> type_list;

namespace any {

/// Instantiates an type::Any type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Any> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Any());
}

}

namespace void_ {

/// Instantiates a type::Void type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Void> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Void());
}

}

namespace caddr {

/// Instantiates a type::Void type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::CAddr> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::CAddr());
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
inline shared_ptr<expression::Constant> create(int i, const Location& l=Location::None)
{
    auto c = _sptr(new constant::Integer(i, 64, l));
    return _sptr(new expression::Constant(c, l));
}

/// Instantiates an type::Integer type.
///
/// width: The bit width.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Integer> type(int width, const Location& l=Location::None) {
    return _sptr(new hilti::type::Integer(width, l));
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
inline shared_ptr<expression::Constant> create(const ::string& s, const Location& l=Location::None)
{
    auto c = _sptr(new constant::String(s, l));
    return _sptr(new expression::Constant(c, l));
}

/// Instantiates a type::String type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::String> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::String(l));
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
inline shared_ptr<expression::Constant> create(bool b, const Location& l=Location::None)
{
    auto c = _sptr(new constant::Bool(b, l));
    return _sptr(new expression::Constant(c, l));
}

/// Instantiates a type::Bool type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Bool> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Bool(l));
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
inline shared_ptr<expression::Constant> create(double d, const Location& l=Location::None)
{
    auto c = _sptr(new constant::Double(d, l));
    return _sptr(new expression::Constant(c, l));
}

/// Instantiates a type::Double type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Double> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Double(l));
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
inline shared_ptr<expression::Constant> create(const element_list& elems, const Location& l=Location::None)
{
    auto c = _sptr(new constant::Tuple(elems, l));
    return _sptr(new expression::Constant(c, l));
}

/// Instantiates a type::Tuple type.
///
/// types: The types of the tuple's elements.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Tuple> type(const type_list& types, const Location& l=Location::None) {
    return _sptr(new hilti::type::Tuple(types, l));
}

/// Instantiates a type::Tuple type that matches any other tuple type (i.e., \c tuple<*>).
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Tuple> typeAny(const Location& l=Location::None) {
    return _sptr(new hilti::type::Tuple(l));
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
    auto c = _sptr(new ctor::Bytes(b, l));
    return _sptr(new hilti::expression::Ctor(c, l));
}

/// Instantiates a type::Bytes type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Bytes> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Bytes(l));
}

}

namespace reference {

/// Instantiates an AST expression node representing a null reference.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<expression::Constant> createNull(const Location& l=Location::None)
{
    auto c = _sptr(new constant::Reference(l));
    return _sptr(new expression::Constant(c, l));
}

/// Instantiates a type::Reference type.
///
/// rtype: The referenced type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Reference> type(shared_ptr<hilti::Type> rtype, const Location& l=Location::None) {
    return _sptr(new hilti::type::Reference(rtype, l));
}

/// Instantiates a type::Reference type that matches any other reference type (i.e., \c ref<*>).
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Reference> typeAny(const Location& l=Location::None) {
    return _sptr(new hilti::type::Reference(l));
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
    return _sptr(new hilti::type::Iterator(l));
}

/// Instantiates a type::iterator::Bytes.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Iterator> typeBytes(const Location& l=Location::None) {
    auto ty = shared_ptr<Type>(new type::Bytes(l));
    return _sptr(new hilti::type::iterator::Bytes(l));
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
inline shared_ptr<expression::Constant> create(const ::string& id, const Location& l=Location::None)
{
    auto c = _sptr(new constant::Label(id, l));
    return _sptr(new expression::Constant(c, l));
}

/// Instantiates a type::Label type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Label> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Label(l));
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
inline shared_ptr<expression::CodeGen> create(shared_ptr<hilti::Type> t, void* cookie, const Location& l=Location::None)
{
    return _sptr(new expression::CodeGen(t, cookie, l));
}

}

namespace module {

/// Instantiates an AST node representing a top-level module.
///
/// id: The name of the module.
///
/// file: The filename associated with the module. This should include the
/// full path to find the corresponding file.
///
/// l: Location associated with the type.
///
/// Returns: The module node.
inline shared_ptr<Module> create(shared_ptr<ID> id, ::string file = "-", const Location& l=Location::None) {
    return _sptr(new Module(id, file, l));
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
    auto var = _sptr(new hilti::variable::Global(id, type, init, l));
    return _sptr(new declaration::Variable(id, var, l));
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
inline shared_ptr<declaration::Type> type(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Scope> scope, const Location& l=Location::None) {
    return _sptr(new declaration::Type(id, type, scope, l));
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
    return _sptr(new hilti::expression::Type(t, l));
}

/// Creates a reference to a type via its name. The named type must be
/// defined elsewhere via a declaration::Type, and the resolver must be run
/// to eventually link the two.
inline shared_ptr<hilti::Type> byName(shared_ptr<ID> id, const Location& l=Location::None) {
    return _sptr(new hilti::type::Unknown(id, l));
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
    auto var = _sptr(new hilti::variable::Local(id, type, init, l));
    return _sptr(new declaration::Variable(id, var, l));
}

}

namespace id {

/// Instantiates an AST node representating an ID.
///
/// s: The name of the ID, either scoped or unscoped.
///
/// l: Location associated with the type.
///
/// Returns: The ID node.
inline shared_ptr<ID> create(::string s, const Location& l=Location::None) {
    return _sptr(new ID(s, l));
}

/// Instantiates an AST expression node representing an ID reference.
///
/// id: The ID being referenced.
///
/// l: Location associated with the type.
///
/// Returns: The ID node.
inline shared_ptr<expression::ID> create(shared_ptr<ID> id, const Location& l=Location::None) {
    return _sptr(new expression::ID(id, l));
}


}

namespace unset {

/// Instantiates an AST expression node representing the constant marking an
/// unset value.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<expression::Constant> create(const Location& l=Location::None)
{
    auto c = _sptr(new constant::Unset(l));
    return _sptr(new expression::Constant(c, l));
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

    return _sptr(new statement::instruction::Unresolved(mnemonic, ops, l));
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
    return _sptr(new statement::instruction::Unresolved(mnemonic, ops, l));
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
/// result: The parameter representing the return value.
///
/// params: The parameters to the functions.
///
/// module: The module will function is declared in.
///
/// cc: The function's calling convention.
///
/// body: The body of the function. Can be null to declare a function with
/// giving it a body (like for prototyping an external function).
///
/// l: Location associated with the type.
///
/// Returns: The function node.
inline shared_ptr<declaration::Function> create(shared_ptr<ID> id,
                                                shared_ptr<hilti::function::Parameter> result,
                                                const hilti::function::parameter_list& params,
                                                shared_ptr<Module> module,
                                                hilti::type::function::CallingConvention cc,
                                                shared_ptr<statement::Block> body,
                                                const Location& l=Location::None) {

    auto ftype = _sptr(new hilti::type::Function(result, params, cc, l));
    auto func = _sptr(new Function(id, ftype, module, body, l));
    return _sptr(new declaration::Function(id, func, l));
}

/// Instantiates an AST node representing a function parameter for its type description.
///
/// id: The name of the parameter.
///
/// type: The type of the parameter.
///
/// constant: True if it's a constant parameter.
///
/// l: Location associated with the type.
///
/// Returns: The parameter node.
inline shared_ptr<hilti::function::Parameter> parameter(shared_ptr<ID> id, shared_ptr<Type> type, bool constant, shared_ptr<Expression> default_value, Location l=Location::None) {
    return _sptr(new hilti::function::Parameter(id, type, constant, default_value, l));
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
inline shared_ptr<hilti::function::Parameter> result(shared_ptr<Type> type, bool constant, Location l=Location::None) {
    return _sptr(new hilti::function::Parameter(type, constant, l));
}

}

namespace block {

typedef hilti::statement::Block::stmt_list statement_list;
typedef hilti::statement::Block::decl_list declaration_list;

/// Instantiates an AST statement node representing a statement block.
///
/// label: A name for the block, or null if non name.
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
    return _sptr(new hilti::statement::Block(decls, stmts, parent, nullptr, l));
}

/// Instantiates an AST statement node representing a statement block.
///
/// parent: The parent scope.
///
/// l: Location associated with the block.
///
/// Returns: The block node.
inline shared_ptr<statement::Block> create(const shared_ptr<Scope> parent, Location l=Location::None) {
    return _sptr(new hilti::statement::Block(parent, nullptr, l));
}

typedef statement::Try::catch_list catch_list;

inline shared_ptr<statement::try_::Catch> catch_(shared_ptr<Type> type, shared_ptr<ID> id, shared_ptr<statement::Block> block, const Location& l)
{
    return _sptr(new statement::try_::Catch(type, id, block, l));
}

inline shared_ptr<statement::Try> try_(shared_ptr<statement::Block> block, const catch_list& catches, const Location& l=Location::None)
{
    return _sptr(new hilti::statement::Try(block, catches, l));
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
    auto c = _sptr(new constant::Address(addr, l));
    return _sptr(new hilti::expression::Constant(c, l));
}

/// Instantiates an type::Address type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Address> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Address(l));
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
    auto c = _sptr(new constant::Network(cidr, l));
    return _sptr(new hilti::expression::Constant(c, l));
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
    auto c = _sptr(new constant::Network(prefix, width, l));
    return _sptr(new hilti::expression::Constant(c, l));
}

/// Instantiates an type::Network type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Network> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Network(l));
}

}

namespace port {

/// Instantiates an AST expression node representing an Port constant.
///
/// port: The port in dotted-quad (v4) or hex (v6) form.
///
/// l: Location associated with the type.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Constant> create(const std::string& port, const Location& l=Location::None)
{
    auto c = _sptr(new constant::Port(port, l));
    return _sptr(new hilti::expression::Constant(c, l));
}

/// Instantiates an type::Port type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Port> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Port(l));
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
    auto c = _sptr(new constant::Interval(secs, l));
    return _sptr(new hilti::expression::Constant(c, l));
}

/// Instantiates an type::Interval type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Interval> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Interval(l));
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
    auto c = _sptr(new constant::Time(secs, l));
    return _sptr(new hilti::expression::Constant(c, l));
}

/// Instantiates an type::Time type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Time> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Time(l));
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
    auto c = _sptr(new constant::Enum(label, etype));
    return _sptr(new hilti::expression::Constant(c, l));
}

/// Instantiates an type::Enum type.
///
/// labels: The labels of the type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Enum> type(const label_list& labels, const Location& l=Location::None) {
    return _sptr(new hilti::type::Enum(labels));
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
    auto c = _sptr(new constant::Bitset(bits, btype));
    return _sptr(new hilti::expression::Constant(c, l));
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
    return _sptr(new hilti::type::Bitset(labels));
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
    return _sptr(new hilti::type::Exception(base, arg, l));
}

}

namespace callable {

/// Instantiates a type::Callable type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Callable> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Callable(l));
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
    return _sptr(new hilti::type::Channel(type, l));
}

/// Instantiates a type::Channel wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Channel> typeAny(const Location& l=Location::None) {
    return _sptr(new hilti::type::Channel(l));
}

}

namespace classifier {

/// Instantiates a type::Classifier type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Classifier> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Classifier(l));
}

}

namespace file {

/// Instantiates a type::File type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::File> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::File(l));
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
    return _sptr(new hilti::type::IOSource(id, l));
}

/// Instantiates a type::IOSource wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::IOSource> typeAny(const Location& l=Location::None) {
    return _sptr(new hilti::type::IOSource(l));
}

}

namespace list {

typedef ctor::List::element_list element_list;

/// Instantiates an AST expression node representing a list ctor.
///
/// elems: The list of elements.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(const element_list& elems, const Location& l=Location::None)
{
    auto c = _sptr(new ctor::List(elems, l));
    return _sptr(new hilti::expression::Ctor(c, l));
}

/// Instantiates a type::List type.
///
/// etype: The type of the elements.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::List> type(shared_ptr<Type> etype, const Location& l=Location::None) {
    return _sptr(new hilti::type::List(etype, l));
}

/// Instantiates a type::List wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::List> typeAny(const Location& l=Location::None) {
    return _sptr(new hilti::type::List(l));
}

}

namespace set {

typedef ctor::Set::element_list element_list;

/// Instantiates an AST expression node representing a set ctor.
///
/// elems: The list of elements.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(const element_list& elems, const Location& l=Location::None)
{
    auto c = _sptr(new ctor::Set(elems, l));
    return _sptr(new hilti::expression::Ctor(c, l));
}

/// Instantiates a type::Set type.
///
/// etype: The type of the elements.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Set> type(shared_ptr<Type> etype, const Location& l=Location::None) {
    return _sptr(new hilti::type::Set(etype, l));
}

/// Instantiates a type::Set wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Set> typeAny(const Location& l=Location::None) {
    return _sptr(new hilti::type::Set(l));
}

}

namespace vector {

typedef ctor::Vector::element_list element_list;

/// Instantiates an AST expression node representing a vector ctor.
///
/// elems: The list of elements.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(const element_list& elems, const Location& l=Location::None)
{
    auto c = _sptr(new ctor::Vector(elems, l));
    return _sptr(new hilti::expression::Ctor(c, l));
}

/// Instantiates a type::Vector type.
///
/// etype: The type of the elements.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Vector> type(shared_ptr<Type> etype, const Location& l=Location::None) {
    return _sptr(new hilti::type::Vector(etype, l));
}

/// Instantiates a type::Vector wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Vector> typeAny(const Location& l=Location::None) {
    return _sptr(new hilti::type::Vector(l));
}

}

namespace expression {

typedef std::list<node_ptr<Expression>> list;

}

namespace map {

typedef ctor::Map::element_list element_list;

/// Instantiates an AST expression node representing a map ctor.
///
/// elems: The list of elements.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(const element_list& elems, const Location& l=Location::None)
{
    auto c = _sptr(new ctor::Map(elems, l));
    return _sptr(new hilti::expression::Ctor(c, l));
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
    return _sptr(new hilti::type::Map(key, value, l));
}

/// Instantiates a type::Map wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Map> typeAny(const Location& l=Location::None) {
    return _sptr(new hilti::type::Map(l));
}

}

namespace overlay {

/// Instantiates a type::Overlay type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
///
/// \todo This needs extension once we have the Type fully defined.
inline shared_ptr<hilti::type::Overlay> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Overlay(l));
}

/// Instantiates a type::Overlay wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Overlay> typeAny(const Location& l=Location::None) {
    return _sptr(new hilti::type::Overlay(l));
}

}

namespace regexp {

typedef hilti::ctor::RegExp::pattern re_pattern;
typedef hilti::ctor::RegExp::pattern_list re_pattern_list;
typedef hilti::type::RegExp::attribute_list attribute_list;

/// Instantiates a RegExp pattern for use with create().
///
/// re: The regular expression itself.
///
/// flags: An optional set of compilation flags. Currently, no flags are
/// supported though.
inline re_pattern pattern(const ::string& re, const ::string& flags, const Location& l=Location::None)
{
    return std::make_tuple(re, flags);
}

/// Instantiates an AST expression node representing a RegExp ctor with a
/// series of patterns.
///
/// patterns: The patterns.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(const re_pattern_list& patterns, const Location& l=Location::None)
{
    auto c = _sptr(new ctor::RegExp(patterns, l));
    return _sptr(new hilti::expression::Ctor(c, l));
}

/// Instantiates an AST expression node representing a RegExp ctor with a single pattern.
///
/// patterns: The pattern. A pattern is a tuple of two ::strings.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(const re_pattern& pattern, const Location& l=Location::None)
{
    re_pattern_list patterns;
    patterns.push_back(pattern);
    return create(patterns, l);
}

/// Instantiates a type::RegExp type.
///
/// attrs: List of optional attributes controlling specifics of the regular
/// expression. These must include the ampersand, e.g., \c &nosub.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::RegExp> type(const attribute_list& attrs, const Location& l=Location::None) {
    return _sptr(new hilti::type::RegExp(attrs, l));
}

/// Instantiates a type::RegExp wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::RegExp> typeAny(const Location& l=Location::None) {
    return _sptr(new hilti::type::RegExp(l));
}

}

namespace timer {

/// Instantiates a type::Timer type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Timer> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::Timer(l));
}

}

namespace timer_mgr {

/// Instantiates a type::TimerMgr type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::TimerMgr> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::TimerMgr(l));
}

}

namespace match_token_state {

/// Instantiates a type::MatchTokenState type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::MatchTokenState> type(const Location& l=Location::None) {
    return _sptr(new hilti::type::MatchTokenState(l));
}

}

namespace struct_ {

#if 0
// Turns out we don't have a struct ctor but coerce tuples into structs ... Need to decide whether we want to keep it that way.

typedef ctor::Struct::element_list element_list;

/// Instantiates an AST expression node representing a struct ctor.
///
/// elems: The elements to initialize the struct with.
///
/// l: Location associated with the instance.
///
/// Returns: The expression node.
inline shared_ptr<hilti::expression::Ctor> create(const element_list& elems, const Location& l=Location::None)
{
    auto c = _sptr(new ctor::Struct(elems, l));
    return _sptr(new hilti::expression::Ctor(c, l));
}

typedef hilti::ctor::struct_::field_list field_list;

#endif

typedef hilti::type::Struct::field_list field_list;

/// Instantiates a type::Struct type.
///
/// fields: The struct's fields.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Struct> type(const field_list& fields, const Location& l=Location::None) {
    return _sptr(new hilti::type::Struct(fields, l));
}

/// Instantiates a type::Struct wildcard type.
///
/// l: Location associated with the type.
///
/// Returns: The type node.
inline shared_ptr<hilti::type::Struct> typeAny(const Location& l=Location::None) {
    return _sptr(new hilti::type::Struct(l));
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
    return _sptr(new hilti::type::struct_::Field(id, type, default_, internal, l));
}


}

/////

}

}

#endif

