
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
inline shared_ptr<expression::Ctor> create(const ::string& b, const Location& l=Location::None)
{
    auto c = _sptr(new ctor::Bytes(b, l));
    return _sptr(new expression::Ctor(c, l));
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

/// Instantiates an AST expression node representing a null reference..
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
/// Returns: The type node.
inline shared_ptr<hilti::type::Iterator> type(shared_ptr<Type> type, const Location& l=Location::None) {
    return _sptr(new hilti::type::Iterator(type, l));
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
inline shared_ptr<hilti::type::Iterator> bytes(const Location& l=Location::None) {
    auto ty = shared_ptr<Type>(new type::Bytes(l));
    return _sptr(new hilti::type::Iterator(ty, l));
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
inline shared_ptr<declaration::Variable> create(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, const Location& l=Location::None) {
    auto var = _sptr(new hilti::variable::Global(id, type, init, l));
    return _sptr(new declaration::Variable(id, var, l));
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
inline shared_ptr<declaration::Variable> create(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, const Location& l=Location::None) {
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
    return _sptr(new expression::ID(id));
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
inline shared_ptr<statement::Block> create(const shared_ptr<Scope>& parent, Location l=Location::None) {
    return _sptr(new hilti::statement::Block(parent, nullptr, l));
}

}

}

}

#endif

