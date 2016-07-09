
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

static shared_ptr<type::union_::Field> _opToField(shared_ptr<Type> type,
                                                  shared_ptr<Expression> field)
{
    // The validator makes sure that we can't get expression here which is in
    // fact not a string constant.
    auto cexpr = ast::rtti::checkedCast<expression::Constant>(field);
    ;
    auto cval = ast::rtti::checkedCast<constant::String>(cexpr->constant());
    ;

    return ast::rtti::checkedCast<type::Union>(type)->lookup(cval->value());
}

static shared_ptr<type::union_::Field> _opToField(shared_ptr<Expression> op,
                                                  shared_ptr<Expression> field)
{
    return _opToField(op->type(), field);
}

static shared_ptr<type::union_::Field> _typeToField(shared_ptr<Type> type,
                                                    shared_ptr<Type> field_type)
{
    auto fields = ast::rtti::checkedCast<type::Union>(type)->fields(field_type);
    assert(fields.size() == 1);
    return fields.front();
}

static shared_ptr<type::union_::Field> _typeToField(shared_ptr<Expression> op,
                                                    shared_ptr<Type> field_type)
{
    return _typeToField(op->type(), field_type);
}

static int _fieldIndex(shared_ptr<type::Union> utype, shared_ptr<type::union_::Field> field)
{
    int i = 0;

    for ( auto f : utype->fields() ) {
        if ( f->id()->name() == field->id()->name() )
            return i;

        i++;
    }

    return -1;
}

static void _get(CodeGen* cg, statement::Instruction* i, bool by_type)
{
    auto utype = ast::rtti::tryCast<type::Union>(i->op1()->type());
    auto field =
        by_type ? _typeToField(i->op1(), i->target()->type()) : _opToField(i->op1(), i->op2());
    auto fidx = _fieldIndex(utype, field);
    auto op1 = cg->llvmValue(i->op1());

    auto block_ok = cg->newBuilder("ok");
    auto block_not_set = cg->newBuilder("not_set");

    auto cur = cg->llvmExtractValue(op1, 0);
    auto hidx = cg->llvmConstInt(fidx, 32);
    auto set = cg->builder()->CreateICmpEQ(cur, hidx);
    cg->llvmCreateCondBr(set, block_ok, block_not_set);

    cg->pushBuilder(block_not_set);
    cg->llvmRaiseException("Hilti::UndefinedValue", i->location());
    cg->popBuilder();

    cg->pushBuilder(block_ok);

    auto val = cg->llvmExtractValue(op1, 1);
    auto result = cg->llvmReinterpret(val, cg->llvmType(field->type()));
    cg->llvmStore(i, result);

    // Leave builder on stack.
}

static void _set(CodeGen* cg, statement::Instruction* i, bool by_type)
{
    auto tt = ast::rtti::checkedCast<type::TypeType>(i->op1()->type())->typeType();
    auto utype = ast::rtti::tryCast<type::Union>(tt);
    auto field = by_type ? _typeToField(utype, i->op2()->type()) : _opToField(utype, i->op2());
    auto fidx = _fieldIndex(utype, field);
    auto op = cg->llvmValue(by_type ? i->op2() : i->op3());

    auto data_type = llvm::cast<llvm::StructType>(cg->llvmType(utype))->getElementType(1);
    auto val = cg->llvmReinterpret(op, data_type);

    auto idx = cg->llvmConstInt(fidx, 32);

    CodeGen::value_list elems = {idx, val};
    auto result = cg->llvmValueStruct(elems);
    cg->llvmStore(i, result);
}

static void _is_set(CodeGen* cg, statement::Instruction* i, bool by_type)
{
    auto utype = ast::rtti::tryCast<type::Union>(i->op1()->type());
    auto field =
        by_type ?
            _typeToField(i->op1(),
                         ast::rtti::checkedCast<type::TypeType>(i->op2()->type())->typeType()) :
            _opToField(i->op1(), i->op2());
    auto fidx = _fieldIndex(utype, field);
    auto op1 = cg->llvmValue(i->op1());

    auto cur = cg->llvmExtractValue(op1, 0);
    auto hidx = cg->llvmConstInt(fidx, 32);
    auto result = cg->builder()->CreateICmpEQ(cur, hidx);

    cg->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::union_::InitField* i)
{
    _set(cg(), i, false);
}

void StatementBuilder::visit(statement::instruction::union_::InitType* i)
{
    _set(cg(), i, true);
}

void StatementBuilder::visit(statement::instruction::union_::GetField* i)
{
    _get(cg(), i, false);
}

void StatementBuilder::visit(statement::instruction::union_::GetType* i)
{
    _get(cg(), i, true);
}

void StatementBuilder::visit(statement::instruction::union_::IsSetField* i)
{
    _is_set(cg(), i, false);
}

void StatementBuilder::visit(statement::instruction::union_::IsSetType* i)
{
    _is_set(cg(), i, true);
}
