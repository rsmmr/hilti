
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

static llvm::Value* _makeIterator(CodeGen* cg, llvm::Value* src, llvm::Value* elem)
{
    llvm::Value* time = nullptr;
    llvm::Value* pkt = nullptr;

    if ( ! src )
        src = cg->llvmConstNull(cg->llvmTypePtr(cg->llvmLibType("hlt.iosrc")));

    if ( elem ) {
        time = cg->llvmExtractValue(elem, 0);
        pkt = cg->llvmExtractValue(elem, 1);
    }

    else {
        time = cg->llvmConstInt(0, 64);
        pkt = cg->llvmConstNull(cg->llvmTypePtr(cg->llvmLibType("hlt.bytes")));
    }

    CodeGen::value_list vals = { src, time, pkt };
    return cg->llvmValueStruct(vals);
}

static llvm::Value* _checkExhausted(CodeGen* cg, statement::Instruction* i, llvm::Value* src, llvm::Value* result, bool make_iters)
{
    auto data = cg->llvmExtractValue(result, 1);
    auto exhausted = cg->llvmCreateIsNull(data);

    auto builder_exhausted = cg->newBuilder("excpt");
    auto builder_not_exhausted = cg->newBuilder("no-excpt");
    auto builder_cont= cg->newBuilder("cont");

    cg->llvmCreateCondBr(exhausted, builder_exhausted, builder_not_exhausted);

    cg->pushBuilder(builder_exhausted);

    llvm::Value* result_exhausted = nullptr;
    llvm::Value* result_not_exhausted = nullptr;

    if ( make_iters )
        result_exhausted = _makeIterator(cg, nullptr, nullptr);

    else
        cg->llvmRaiseException("Hilti::IOSrcExhausted", i->location());

    builder_exhausted = cg->builder();

    cg->llvmCreateBr(builder_cont);
    cg->popBuilder();

    cg->pushBuilder(builder_not_exhausted);

    if ( make_iters )
        result_not_exhausted = _makeIterator(cg, src, result);

    builder_not_exhausted = cg->builder();

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
            return cg->llvmCall("hlt::iosrc_new_live", args);
        }
    ));

    cases.push_back(CodeGen::SwitchCase(
        "pcap-offline",
        cg()->llvmEnum("Hilti::IOSrc::PcapOffline"),
        [&] (CodeGen* cg) -> llvm::Value* {
            return cg->llvmCall("hlt::iosrc_new_offline", args);
        }
    ));

    auto result = cg()->llvmSwitchEnumConst(cg()->llvmEnum(kind), cases, true, i->location());
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::ioSource::Close* i)
{
    CodeGen::expr_list args = { i->op1() };
    cg()->llvmCall("hlt::iosrc_close", args);
}

static llvm::Value* _readTry(CodeGen* cg, statement::Instruction* i, llvm::Value* src)
{
    if ( ! src )
        src = cg->llvmValue(i->op1());

    CodeGen::expr_list args = {
        builder::codegen::create(builder::reference::type(builder::iosource::typeAny()), src),
        builder::boolean::create(false)
    };

    return cg->llvmCall("hlt::iosrc_read_try", args, false, false);
}

static void _readFinish(CodeGen* cg, statement::Instruction* i, llvm::Value* result, bool make_iters, llvm::Value* src)
{
    if ( ! src )
        src = cg->llvmValue(i->op1());

    result = _checkExhausted(cg, i, src, result, make_iters);
    cg->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::ioSource::Read* i)
{
    cg()->llvmBlockingInstruction(i,
                                  [&] (CodeGen* cg, statement::Instruction* i) -> llvm::Value* { return _readTry(cg, i, nullptr); },
                                  [&] (CodeGen* cg, statement::Instruction* i, llvm::Value* result) { _readFinish(cg, i, result, false, nullptr); }
                                 );
}

void StatementBuilder::visit(statement::instruction::iterIOSource::Begin* i)
{
    cg()->llvmBlockingInstruction(i,
                                  [&] (CodeGen* cg, statement::Instruction* i) -> llvm::Value* { return _readTry(cg, i, nullptr); },
                                  [&] (CodeGen* cg, statement::Instruction* i, llvm::Value* result) { _readFinish(cg, i, result, true, nullptr); }
                                 );
}

void StatementBuilder::visit(statement::instruction::iterIOSource::End* i)
{
    auto rty = ast::as<type::Reference>(i->op1()->type());
    assert(rty);

    auto ty = rty->argType();
    auto result = _makeIterator(cg(), nullptr, nullptr);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterIOSource::Incr* i)
{
    auto src = cg()->llvmExtractValue(cg()->llvmValue(i->op1()), 0);

    cg()->llvmBlockingInstruction(i,
                                  [&] (CodeGen* cg, statement::Instruction* i) -> llvm::Value* { return _readTry(cg, i, src); },
                                  [&] (CodeGen* cg, statement::Instruction* i, llvm::Value* result) { _readFinish(cg, i, result, true, src); }
                                 );
}

void StatementBuilder::visit(statement::instruction::iterIOSource::Equal* i)
{
    auto val1 = cg()->llvmValue(i->op1());
    auto val2 = cg()->llvmValue(i->op2());

    auto pkt1 = cg()->llvmExtractValue(val1, 2);
    auto pkt2 = cg()->llvmExtractValue(val2, 2);

    auto result = cg()->builder()->CreateICmpEQ(pkt1, pkt2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterIOSource::Deref* i)
{
    auto val = cg()->llvmValue(i->op1());
    auto time = cg()->llvmExtractValue(val, 1);
    auto pkt = cg()->llvmExtractValue(val, 2);

    CodeGen::value_list vals = { time, pkt };
    auto result = cg()->llvmValueStruct(vals);
    cg()->llvmStore(i, result);
}
