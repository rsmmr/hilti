
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

static void _freeFields(CodeGen* cg, shared_ptr<Type> rtype, llvm::Value* fields, const Location& l)
{
    auto ftypes = ast::as<type::trait::TypeList>(rtype)->typeList();
    for ( int i = 0; i < ftypes.size(); i++ ) {
        auto addr = cg->llvmGEP(fields, cg->llvmGEPIdx(0), cg->llvmGEPIdx(i));
        cg->llvmFree(addr, "classifier-free-one-field", l);
    }

    cg->llvmFree(fields, "classifier-free-fields", l);
}

static llvm::Value* _matchAllField(CodeGen* cg)
{
    return cg->llvmClassifierField(cg->llvmConstNull(cg->llvmTypePtr()), cg->llvmConstInt(0, 64));
}

static llvm::Value* _llvmFields(CodeGen* cg, shared_ptr<Type> rtype, shared_ptr<Type> stype, llvm::Value* val, const Location& l)
{
    auto ftypes = ast::as<type::trait::TypeList>(rtype)->typeList();
    auto stypes = ast::as<type::trait::TypeList>(stype)->typeList();

    auto atype = llvm::ArrayType::get(cg->llvmTypePtr(), ftypes.size());
    auto fields = cg->llvmMalloc(atype, "hlt.classifier", l);

    // Convert the fields into the internal hlt_classifier_field representation.

    auto ft = ftypes.begin();
    auto st = stypes.begin();

    for ( int i = 0; i < ftypes.size(); i++ ) {
        auto field = cg->llvmStructGet(stype, val, i,
                                         [&] (CodeGen* cg) -> llvm::Value* {
                                             return _matchAllField(cg);
                                         },
                                         [&] (CodeGen* cg, llvm::Value* v) -> llvm::Value* {
                                             return cg->llvmClassifierField(*ft, *st, v, l);
                                         }, l);

        auto addr = cg->llvmGEP(fields, cg->llvmGEPIdx(0), cg->llvmGEPIdx(i));
        cg->llvmCreateStore(field, addr);

        ++ft;
        ++st;
    }

    fields = cg->builder()->CreateBitCast(fields, cg->llvmTypePtr());
    return fields;
}


void StatementBuilder::visit(statement::instruction::classifier::New* i)
{
    auto ctype = ast::as<type::Classifier>(typedType(i->op1()));
    auto op1 = builder::integer::create(ast::as<type::trait::TypeList>(ctype->ruleType())->typeList().size());
    auto op2 = builder::type::create(ctype->ruleType());
    auto op3 = builder::type::create(ctype->valueType());

    CodeGen::expr_list args = { op1, op2, op3};
    auto result = cg()->llvmCall("hlt::classifier_new", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::classifier::Add* i)
{
    auto rtype = ast::as<type::Classifier>(referencedType(i->op1()))->ruleType();

    // op2 can be a tuple (ref<struct>, prio) or a just a rule ref<struct>
    auto ttype = ast::as<type::Tuple>(i->op2()->type());

    if ( ttype ) {
        auto op2 = cg()->llvmValue(i->op2());
        auto rule = cg()->llvmExtractValue(op2, 0);
        auto prio = cg()->builder()->CreateZExt(cg()->llvmExtractValue(op2, 1), cg()->llvmTypeInt(64));
        auto stype = ast::as<type::Reference>(ttype->typeList().front())->argType();
        auto fields = _llvmFields(cg(), rtype, stype, rule, i->location());
        CodeGen::expr_list args = { i->op1(), builder::codegen::create(builder::any::type(), fields),
            builder::codegen::create(builder::integer::type(64), prio), i->op3() };
        cg()->llvmCall("hlt::classifier_add", args);
    }

    else {
        auto val = i->op2()->coerceTo(builder::reference::type(rtype));
        auto fields = _llvmFields(cg(), rtype, rtype, cg()->llvmValue(val), i->location());
        CodeGen::expr_list args = { i->op1(), builder::codegen::create(builder::any::type(), fields), i->op3() };
        cg()->llvmCall("hlt::classifier_add_no_prio", args);
    }
}

void StatementBuilder::visit(statement::instruction::classifier::Compile* i)
{
    CodeGen::expr_list args = { i->op1() };
    cg()->llvmCall("hlt::classifier_compile", args);
}

void StatementBuilder::visit(statement::instruction::classifier::Get* i)
{
    auto rtype = ast::as<type::Classifier>(referencedType(i->op1()))->ruleType();
    auto vtype = ast::as<type::Classifier>(referencedType(i->op1()))->valueType();

    auto op2 = i->op2()->coerceTo(builder::reference::type(rtype));
    auto fields = _llvmFields(cg(), rtype, op2->type(), cg()->llvmValue(op2), i->location());

    CodeGen::expr_list args = { i->op1(), builder::codegen::create(builder::any::type(), fields) };
    auto voidp = cg()->llvmCall("hlt::classifier_get", args);
    auto casted = builder()->CreateBitCast(voidp, cg()->llvmTypePtr(cg()->llvmType(vtype)));
    auto result = builder()->CreateLoad(casted);

    cg()->llvmStore(i, result);

    _freeFields(cg(), rtype, fields, i->location());
}

void StatementBuilder::visit(statement::instruction::classifier::Matches* i)
{
    auto rtype = ast::as<type::Classifier>(referencedType(i->op1()))->ruleType();

    auto op2 = i->op2()->coerceTo(builder::reference::type(rtype));
    auto fields = _llvmFields(cg(), rtype, op2->type(), cg()->llvmValue(op2), i->location());

    CodeGen::expr_list args = { i->op1(), builder::codegen::create(builder::any::type(), fields) };
    auto result = cg()->llvmCall("hlt::classifier_matches", args);

    cg()->llvmStore(i, result);

    _freeFields(cg(), rtype, fields, i->location());
}

