
#include <hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

static llvm::Value* _makeIterator(CodeGen* cg, shared_ptr<Type> ty, llvm::Value* src, llvm::Value* elem)
{
    auto ioty = ast::as<type::IOSource>(ty);
    assert(ioty);

    if ( ! src )
        src = cg->llvmConstNull(cg->llvmType(ioty));

    if ( ! elem ) {
        auto ity = type::asTrait<type::trait::Iterable>(ty.get());
        elem = cg->llvmConstNull(cg->llvmType(ity->elementType()));
    }

    CodeGen::value_list vals = { src, elem };
    return cg->llvmValueStruct(vals);
}

static llvm::Value* _checkExhausted(CodeGen* cg, statement::Instruction* i, shared_ptr<Expression> src, llvm::Value* result, bool make_iters)
{
    auto rty = ast::as<type::Reference>(src->type());
    assert(rty);

    auto ty = rty->argType();

    auto data = cg->llvmExtractValue(result, 1);
    auto exhausted = cg->builder()->CreateIsNull(data);

    auto builder_exhausted = cg->newBuilder("excpt");
    auto builder_not_exhausted = cg->newBuilder("no-excpt");
    auto builder_cont= cg->newBuilder("cont");

    cg->llvmCreateCondBr(exhausted, builder_exhausted, builder_not_exhausted);

    cg->pushBuilder(builder_exhausted);

    llvm::Value* result_exhausted = nullptr;
    llvm::Value* result_not_exhausted = nullptr;

    if ( make_iters )
        result_exhausted = _makeIterator(cg, ty, nullptr, nullptr);

    else
        cg->llvmRaiseException("Hilti::IOSrcExhausted", i->location());

    cg->llvmCreateBr(builder_cont);
    cg->popBuilder();

    cg->pushBuilder(builder_not_exhausted);

    if ( make_iters )
        result_not_exhausted = _makeIterator(cg, ty, cg->llvmValue(src), result);

    cg->llvmCreateBr(builder_cont);
    cg->popBuilder();

    cg->pushBuilder(builder_cont);

    if ( make_iters ) {
        auto result = cg->builder()->CreatePHI(result_exhausted->getType(), 2);
        result->addIncoming(result_exhausted, builder_exhausted->GetInsertBlock());
        result->addIncoming(result_not_exhausted, builder_not_exhausted->GetInsertBlock());
        return result;
    }

    return result;
}

void StatementBuilder::visit(statement::instruction::ioSource::New* i)
{
    auto ptype = type::asTrait<type::trait::Parameterized>(typedType(i->op1()).get());
    assert(ptype);

    auto params = ptype->parameters();
    assert(params.size() == 1);

    auto param = std::dynamic_pointer_cast<type::trait::parameter::Enum>(params.front());
    assert(param);

    auto kind = param->label()->pathAsString();
    CodeGen::expr_list args = { i->op2() };

    CodeGen::case_list cases;

    cases.push_back(CodeGen::SwitchCase(
        "pcap-live",
        cg()->llvmEnum("Hilti::IOSrc::PcapLive"),
        [&] (CodeGen* cg) -> llvm::Value* {
            return cg->llvmCall("hlt::iosrc_pcap_new_live", args);
        }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "pcap-offline",
        cg()->llvmEnum("Hilti::IOSrc::PcapOffline"),
        [&] (CodeGen* cg) -> llvm::Value* {
            return cg->llvmCall("hlt::iosrc_pcap_new_offline", args);
        }
    ));

    auto result = cg()->llvmSwitchEnumConst(cg()->llvmEnum(kind), cases, true, i->location());
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::ioSource::Close* i)
{
    CodeGen::expr_list args = { i->op1() };
    cg()->llvmCall("hlt::iosrc_pcap_close", args);
}

static llvm::Value* _readTry(CodeGen* cg, statement::Instruction* i)
{
    CodeGen::expr_list args = { i->op1(), builder::boolean::create(false) };
    return cg->llvmCall("hlt::iosrc_pcap_read_try", args, false);
}

static void _readFinish(CodeGen* cg, statement::Instruction* i, llvm::Value* result, bool make_iters)
{
    result = _checkExhausted(cg, i, i->op1(), result, false);
    cg->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::ioSource::Read* i)
{
    cg()->llvmBlockingInstruction(i,
                                  _readTry,
                                  [&] (CodeGen* cg, statement::Instruction* i, llvm::Value* result) { _readFinish(cg, i, result, false); }
                                 );
}

void StatementBuilder::visit(statement::instruction::iterIOSource::Begin* i)
{
    cg()->llvmBlockingInstruction(i,
                                  _readTry,
                                  [&] (CodeGen* cg, statement::Instruction* i, llvm::Value* result) { _readFinish(cg, i, result, true); }
                                 );
}

void StatementBuilder::visit(statement::instruction::iterIOSource::End* i)
{
    auto rty = ast::as<type::Reference>(i->op1()->type());
    assert(rty);

    auto ty = rty->argType();
    auto result = _makeIterator(cg(), ty, nullptr, nullptr);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterIOSource::Incr* i)
{
    cg()->llvmBlockingInstruction(i,
                                  _readTry,
                                  [&] (CodeGen* cg, statement::Instruction* i, llvm::Value* result) { _readFinish(cg, i, result, true); }
                                 );
}

void StatementBuilder::visit(statement::instruction::iterIOSource::Equal* i)
{
    auto elem1 = cg()->llvmExtractValue(cg()->llvmValue(i->op1()), 1);
    auto elem2 = cg()->llvmExtractValue(cg()->llvmValue(i->op2()), 1);

    auto payload1 = cg()->llvmExtractValue(elem1, 1);
    auto payload2 = cg()->llvmExtractValue(elem2, 1);

    auto result = cg()->builder()->CreateICmpEQ(payload1, payload2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterIOSource::Deref* i)
{
    auto elem = cg()->llvmExtractValue(cg()->llvmValue(i->op1()), 1);
    cg()->llvmStore(i, elem);
}
