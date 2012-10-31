
#include "instruction.h"
#include "statement.h"
#include "variable.h"
#include "module.h"

#include "passes/validator.h"

using namespace hilti;

const int _num_ops = 4; // Number of operands we support, including target.

hilti::InstructionRegistry* InstructionRegistry::_registry = nullptr;

InstructionRegistry* InstructionRegistry::globalRegistry()
{
    if ( ! _registry )
        _registry = new InstructionRegistry();

    return _registry;
}

shared_ptr<Type> InstructionHelper::typedType(shared_ptr<Expression> op) const
{
    auto t = ast::as<type::TypeType>(op->type());
    assert(t);
    return t->typeType();
}

shared_ptr<Type> InstructionHelper::referencedType(shared_ptr<Expression> op) const
{
    auto t = ast::as<type::Reference>(op->type());
    assert(t);
    return t->argType();
}

shared_ptr<Type> InstructionHelper::referencedType(shared_ptr<Type> ty) const
{
    auto t = ast::as<type::Reference>(ty);
    assert(t);
    return t->argType();
}

shared_ptr<Type> InstructionHelper::elementType(shared_ptr<Expression> op) const
{
    return elementType(op->type());
}

shared_ptr<Type> InstructionHelper::elementType(shared_ptr<Type> ty) const
{
    auto r = ast::as<type::Reference>(ty);

    if ( r )
        ty = r->argType();

    auto t = ast::as<type::TypedHeapType>(ty);
    assert(t);

    return t->argType();
}

shared_ptr<Type> InstructionHelper::argType(shared_ptr<Expression> op) const
{
    return argType(op->type());
}

shared_ptr<Type> InstructionHelper::argType(shared_ptr<Type> ty) const
{
    auto r = ast::as<type::Reference>(ty);

    if ( r )
        ty = r->argType();

    auto t1 = ast::as<type::TypedHeapType>(ty);

    if ( t1 )
        return t1->argType();

    auto t2 = ast::as<type::TypedValueType>(ty);

    if ( t2 )
        return t2->argType();

    assert(false);
    return nullptr;
}

shared_ptr<Type> InstructionHelper::iteratedType(shared_ptr<Expression> op) const
{
    auto t = ast::as<type::Iterator>(op->type());
    return t->argType();
}

shared_ptr<Type> InstructionHelper::mapKeyType(shared_ptr<Expression> op) const
{
    return mapKeyType(op->type());
}

shared_ptr<Type> InstructionHelper::mapKeyType(shared_ptr<Type> ty) const
{
    auto r = ast::as<type::Reference>(ty);

    if ( r )
        ty = r->argType();

    auto t = ast::as<type::Map>(ty);
    assert(t);

    return t->keyType();
}

shared_ptr<Type> InstructionHelper::mapValueType(shared_ptr<Expression> op) const
{
    return mapValueType(op->type());
}

shared_ptr<Type> InstructionHelper::mapValueType(shared_ptr<Type> ty) const
{
    auto r = ast::as<type::Reference>(ty);

    if ( r )
        ty = r->argType();

    auto t = ast::as<type::Map>(ty);
    assert(t);

    return t->valueType();
}

Instruction::operator string() const
{
    std::list<string> ops;

    if ( __typeOp1() )
        ops.push_back(__typeOp1()->render());

    if ( __typeOp2() )
        ops.push_back(__typeOp2()->render());

    if ( __typeOp3() )
        ops.push_back(__typeOp3()->render());

    string s = _id->name() + ": " + util::strjoin(ops, ", ");

    if ( __typeOp0() )
        s += " -> " + __typeOp0()->render();

    return s;
}

void InstructionRegistry::addInstruction(shared_ptr<Instruction> ins)
{
    _instructions.insert(instr_map::value_type(ins->id()->name(), ins));
}

InstructionRegistry::instr_list InstructionRegistry::getMatching(shared_ptr<ID> id, const instruction::Operands& ops) const
{
    // We try this twice. First, without coerce, then with. To ambiguity, we
    // prefer matches from the former.

    instr_list matches;

    auto m = _instructions.equal_range(id->name());

    for ( auto n = m.first; n != m.second; ++n ) {
        shared_ptr<Instruction> instr = n->second;

        if ( instr->matchesOperands(ops, false) )
            matches.push_back(instr);
    }

    if ( matches.size() )
        return matches;

    for ( auto n = m.first; n != m.second; ++n ) {
        shared_ptr<Instruction> instr = n->second;

        if ( instr->matchesOperands(ops, true) )
            matches.push_back(instr);
    }

    return matches;
}

InstructionRegistry::instr_list InstructionRegistry::byName(const string& name) const
{
    instr_list matches;

    auto m = _instructions.equal_range(name);

    for ( auto n = m.first; n != m.second; ++n ) {
        shared_ptr<Instruction> instr = n->second;
        matches.push_back(instr);
    }

    return matches;
}

bool Instruction::matchesOperands(const instruction::Operands& ops, bool coerce)
{
#if 0
    fprintf(stderr, "--- %s  coerce %d\n", string(*this).c_str(), coerce);
    fprintf(stderr, "0 %s | %s\n", ops[0] ? ops[0]->render().c_str() : "-", __typeOp0() ? __typeOp0()->render().c_str() : "-");
    fprintf(stderr, "1 %s | %s\n", ops[1] ? ops[1]->render().c_str() : "-", __typeOp1() ? __typeOp1()->render().c_str() : "-");
    fprintf(stderr, "2 %s | %s\n", ops[2] ? ops[2]->render().c_str() : "-", __typeOp2() ? __typeOp2()->render().c_str() : "-");
    fprintf(stderr, "3 %s | %s\n", ops[3] ? ops[3]->render().c_str() : "-", __typeOp3() ? __typeOp3()->render().c_str() : "-");

    fprintf(stderr, "match: 0->%d 1->%d 2->%d 3->%d\n", (int)__matchOp0(ops[0], coerce), (int)__matchOp1(ops[1], coerce), (int)__matchOp2(ops[2], coerce),  (int)__matchOp3(ops[3], coerce));

    if ( ! ops[1] )
        fprintf(stderr, " A %d\n", (int)ast::isA<type::OptionalArgument>(__typeOp1()));

    if ( ! __typeOp1()->equal(ops[1]->type()) )
        fprintf(stderr, " B F\n");

    //if ( ops[1]->isConstant() && ! constant )
    //    fprintf(stderr, " C F\n");

    if ( ! ops[1] && ! __defaultOp1() )
        fprintf(stderr, " D F\n");

    fprintf(stderr, "+++\n");
#endif

    if ( ! __matchOp0(ops[0], coerce) )
        return false;

    if ( ! __matchOp1(ops[1], coerce) )
        return false;

    if ( ! __matchOp2(ops[2], coerce) )
        return false;

    if ( ! __matchOp3(ops[3], coerce) )
        return false;

    return true;
}

void Instruction::error(Node* op, string msg) const
{
    assert(_validator);
    if ( op )
        _validator->error(op, msg);
    else
        _validator->error(msg);
}

bool Instruction::canCoerceTo(shared_ptr<Expression> op, shared_ptr<Type> target) const
{
    if ( op->canCoerceTo(target) )
        return true;

    error(op, util::fmt("operand type %s is not compatible with type %s",
                        op->type()->render().c_str(), target->render().c_str()));
    return false;
}

bool Instruction::canCoerceTo(shared_ptr<Expression> op, shared_ptr<Expression> target) const
{
    if ( op->canCoerceTo(target->type()) )
        return true;

    error(op, util::fmt("operand type %s is not compatible with target type %s",
                        op->type()->render().c_str(), target->type()->render().c_str()));
    return false;
}

bool Instruction::canCoerceTo(shared_ptr<Type> src, shared_ptr<Expression> target) const
{
    if ( Coercer().canCoerceTo(src, target->type()) )
        return true;

    error(target, util::fmt("type %s is not compatible with target type %s",
                        src->render().c_str(), target->type()->render().c_str()));
    return false;
}

bool Instruction::canCoerceTo(shared_ptr<Type> src, shared_ptr<Type> target) const
{
    if ( Coercer().canCoerceTo(src, target) )
        return true;

    error(target, util::fmt("type %s is not compatible with target type %s",
                        src->render().c_str(), target->render().c_str()));
    return false;
}

bool Instruction::canCoerceTypes(shared_ptr<Expression> op1, shared_ptr<Expression> op2) const
{
   if ( op1->canCoerceTo(op2->type()) || op2->canCoerceTo(op1->type()) )
        return true;

    error(op1, util::fmt("operand types %s and %s are not compatible",
                        op1->type()->render().c_str(), op2->type()->render().c_str()));

    return false;
}

bool Instruction::hasType(shared_ptr<Expression> op, shared_ptr<Type> target) const
{
    if ( op->type() == target )
        return true;

    error(op, util::fmt("operand type %s does not match expected type %s",
                        op->type()->render().c_str(), target->render().c_str()));

    return false;
}

bool Instruction::equalTypes(shared_ptr<Type> ty1, shared_ptr<Type> ty2) const
{
    if ( ty1->equal(ty2) )
        return true;

    error(ty1, util::fmt("types %s and %s do not match",
                        ty1->render().c_str(), ty2->render().c_str()));

    return false;
}

bool Instruction::equalTypes(shared_ptr<Expression> op1, shared_ptr<Expression> op2) const
{
    if ( op1->type()->equal(op2->type()) )
        return true;

    error(op1, util::fmt("operand types %s and %s do not match",
                        op1->type()->render().c_str(), op2->type()->render().c_str()));

    return false;
}

bool Instruction::isConstant(shared_ptr<Expression> op) const
{
    if ( ast::as<expression::Constant>(op) )
        return true;

    error(op, "operand must be a constant");

    return false;
}

bool Instruction::checkCallParameters(shared_ptr<type::Function> func, shared_ptr<Expression> args) const
{
    if ( ! args->isConstant() ) {
        // Currently, we don't support non-constasnt tuples, though is in principle, that should work.
        error(args, "deficiency: can only call with constant tuple argument currently");
        return 1;
    }

    auto ops = ast::as<constant::Tuple>(ast::as<expression::Constant>(args)->constant())->value();

    auto proto = func->parameters();

    int i = 1;

    auto p = proto.begin();
    auto o = ops.begin();

    while ( o != ops.end() && p != proto.end() ) {
        if ( ! (*o)->canCoerceTo((*p)->type()) ) {
            auto have = (*o)->type()->render();
            auto want = (*p)->type()->render();
            error(*o, util::fmt("type of parameter %d does not match function, have %s but expected %s", i, have.c_str(), want.c_str()));
            return false;
        }

        ++i;
        ++o;
        ++p;
    }

    if ( p == proto.end() && o == ops.end() )
        return true;

    if ( o != ops.end() ) {
        error(*o, util::fmt("too many arguments given, expected %d but got %d", proto.size(), ops.size()));
        return false;
    }

    // Remaining ones must have defaults.
    while ( p != proto.end() ) {
        if ( ! (*p)->default_() ) {
            error(*p, util::fmt("too few arguments given, expected %d but got %d", proto.size(), ops.size()));
            return false;
        }

        ++p;
    }

    return true;
}

bool Instruction::checkCallResult(shared_ptr<Type> rtype, shared_ptr<Type> ty) const
{
    bool is_void = ast::isA<type::Void>(rtype) || ast::isA<type::OptionalArgument>(rtype);

    if ( ty && is_void ) {
        error(nullptr, "function does not return a value");
        return false;
    }

    if ( ! ty && ! is_void ) {
        error(nullptr, "function value not used");
        return false;
    }

    if ( ! ty )
        return true;

    if ( Coercer().canCoerceTo(ty, rtype) )
        return true;

    auto have = ty->render();
    auto want = rtype->render();

    error(ty, util::fmt("return type does not match function, have %s but expected %s", have.c_str(), want.c_str()));

    return false;
}

bool Instruction::checkCallResult(shared_ptr<Type> rtype, shared_ptr<Expression> op) const
{
    bool is_void = ast::isA<type::Void>(rtype) || ast::isA<type::OptionalArgument>(rtype);

    if ( op && is_void ) {
        error(nullptr, "function does not return a value");
        return false;
    }

    if ( ! op && ! is_void ) {
        error(nullptr, "function value not used");
        return false;
    }

    if ( ! op )
        return true;

    if ( op->canCoerceTo(rtype) )
        return true;

    auto have = op->type()->render();
    auto want = rtype->render();

    error(op, util::fmt("return type does not match function, have %s but expected %s", have.c_str(), want.c_str()));

    return false;
}

Instruction::Info Instruction::info() const
{
    Info info;
    info.mnemonic = _id->name();
    info.namespace_ = _namespace;
    info.class_ = _class;
    info.description = ::util::escapeBytes(__doc());
    info.terminator = __terminator();
    info.type_target = __typeOp0();
    info.type_op1 = __typeOp1();
    info.type_op2 = __typeOp2();
    info.type_op3 = __typeOp3();
    info.default_op1 = __defaultOp1();
    info.default_op2 = __defaultOp2();
    info.default_op3 = __defaultOp3();

    if ( ::util::startsWith(info.mnemonic, ".op.") )
         info.mnemonic = info.mnemonic.substr(4, std::string::npos);

    return info;
}

shared_ptr<statement::instruction::Resolved> InstructionRegistry::resolveStatement(shared_ptr<Instruction> instr, shared_ptr<statement::Instruction> stmt)
{
    auto resolved = resolveStatement(instr, stmt->operands(), stmt->location());

    if ( stmt->internal() )
        resolved->setInternal();

    for ( auto c : stmt->comments() )
        resolved->addComment(c);

    return resolved;
}

shared_ptr<statement::instruction::Resolved> InstructionRegistry::resolveStatement(shared_ptr<Instruction> instr, const instruction::Operands& ops, const Location& l)
{
    instruction::Operands new_ops;
    instruction::Operands defaults;

    defaults.resize(_num_ops);
    defaults[0] = node_ptr<Expression>();
    defaults[1] = instr->__defaultOp1();
    defaults[2] = instr->__defaultOp2();
    defaults[3] = instr->__defaultOp3();

    for ( int i = 0; i < _num_ops; i++ ) {
        auto op = ops[i];

        if ( op && instr ) {

            switch ( i ) {
             case 1:
                if ( instr->__typeOp1() )
                    op = op->coerceTo(instr->__typeOp1());
                break;

             case 2:
                if ( instr->__typeOp2() )
                    op = op->coerceTo(instr->__typeOp2());
                break;

             case 3:
                if ( instr->__typeOp3() )
                    op = op->coerceTo(instr->__typeOp3());
                break;
            }
        }

        new_ops.push_back(op ? op : defaults[i]);
    }

    auto resolved = (*instr->factory())(instr, new_ops, l);

    return resolved;
}

