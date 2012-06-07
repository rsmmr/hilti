
#include "../hilti.h"

#include "field-builder.h"
#include "codegen.h"
#include "builder.h"

using namespace hilti;
using namespace hilti::codegen;

FieldBuilder::FieldBuilder(CodeGen* cg) : CGVisitor<llvm::Value* , shared_ptr<Type>, llvm::Value*>(cg, "codegen::FieldBuilder")
{
}

FieldBuilder::~FieldBuilder()
{
}

llvm::Value* FieldBuilder::llvmClassifierField(shared_ptr<Type> field_type, shared_ptr<Type> src_type, llvm::Value* src_val, const Location& l)
{
    assert(type::hasTrait<type::trait::Classifiable>(field_type));

    _location = l;

    llvm::Value* result = nullptr;
    setArg1(src_type);
    setArg2(src_val);
    processOne(field_type, &result);
    assert(result);

    return result;
}

void FieldBuilder::visit(type::Reference* t)
{
    call(t->argType());
}

void FieldBuilder::visit(type::Address* t)
{
    auto src_type = arg1();
    auto src_val = arg2();

    auto a1 = cg()->llvmExtractValue(src_val, 0);
    a1 = cg()->llvmHtoN(a1);
    auto a2 = cg()->llvmExtractValue(src_val, 1);
    a2 = cg()->llvmHtoN(a2);

    auto tmp = cg()->llvmAddTmp("addr-field", src_val->getType(), nullptr, true);
    auto addr = cg()->llvmGEP(tmp, cg()->llvmGEPIdx(0), cg()->llvmGEPIdx(0));
    cg()->llvmCreateStore(a1, addr);

    tmp = cg()->llvmAddTmp("addr-field", src_val->getType(), nullptr, true);
    addr = cg()->llvmGEP(tmp, cg()->llvmGEPIdx(0), cg()->llvmGEPIdx(1));
    cg()->llvmCreateStore(a2, addr);

    // len = cg.llvmSizeOf(self.llvmType(cg).elements[0]);
    // len = cg.builder().add(len, cg.llvmSizeOf(self.llvmType(cg).elements[1]));

    auto len = cg()->llvmSizeOf(src_val->getType());
    auto result = cg()->llvmClassifierField(tmp, len);

    setResult(result);
}

void FieldBuilder::visit(type::Network* t)
{
    auto src_type = arg1();
    auto src_val = arg2();

    auto a1 = cg()->llvmExtractValue(src_val, 0);
    a1 = cg()->llvmHtoN(a1);
    auto a2 = cg()->llvmExtractValue(src_val, 1);
    a2 = cg()->llvmHtoN(a2);

    auto tmp = cg()->llvmAddTmp("net-field", src_val->getType(), nullptr, true);
    auto addr = cg()->llvmGEP(tmp, cg()->llvmGEPIdx(0), cg()->llvmGEPIdx(0));
    cg()->llvmCreateStore(a1, addr);

    tmp = cg()->llvmAddTmp("net-field", src_val->getType(), nullptr, true);
    addr = cg()->llvmGEP(tmp, cg()->llvmGEPIdx(0), cg()->llvmGEPIdx(1));
    cg()->llvmCreateStore(a2, addr);

    // len = cg.llvmSizeOf(self.llvmType(cg).elements[0]);
    // len = cg.builder().add(len, cg.llvmSizeOf(self.llvmType(cg).elements[1]));

    auto len = cg()->llvmSizeOf(src_val->getType());
    auto bits = cg()->llvmExtractValue(src_val, 2);
    bits = cg()->builder()->CreateZExt(bits, cg()->llvmTypeInt(64));

    auto result = cg()->llvmClassifierField(tmp, len, bits);
    setResult(result);
}

void FieldBuilder::visit(type::Port* t)
{
    auto src_type = arg1();
    auto src_val = arg2();

    auto a1 = cg()->llvmExtractValue(src_val, 0);
    a1 = cg()->llvmHtoN(a1);

    auto tmp = cg()->llvmAddTmp("port-field", src_val->getType(), nullptr, true);
    auto addr = cg()->llvmGEP(tmp, cg()->llvmGEPIdx(0), cg()->llvmGEPIdx(0));
    cg()->llvmCreateStore(a1, addr);

    auto len = cg()->llvmSizeOf(src_val->getType());
    auto result = cg()->llvmClassifierField(tmp, len);

    setResult(result);
}

void FieldBuilder::visit(type::Bool* t)
{
    auto src_type = arg1();
    auto src_val = arg2();

    auto tmp = cg()->llvmAddTmp("port-field", src_val->getType(), nullptr, true);
    cg()->llvmCreateStore(src_val, tmp);

    auto len = cg()->llvmSizeOf(src_val->getType());
    auto result = cg()->llvmClassifierField(tmp, len);

    setResult(result);
}

void FieldBuilder::visit(type::Integer* t)
{
    auto src_type = arg1();
    auto src_val = arg2();

    auto tmp = cg()->llvmAddTmp("port-field", src_val->getType(), nullptr, true);
    cg()->llvmCreateStore(cg()->llvmHtoN(src_val), tmp);

    auto len = cg()->llvmSizeOf(src_val->getType());
    auto result = cg()->llvmClassifierField(tmp, len);

    setResult(result);
}

void FieldBuilder::visit(type::Bytes* t)
{
    auto src_type = arg1();
    auto src_val = arg2();

    CodeGen::expr_list args = { builder::codegen::create(src_type, src_val) };
    auto data = cg()->llvmCall("hlt::bytes_to_raw", args);
    auto len = cg()->llvmCall("hlt::bytes_len", args);

    auto result = cg()->llvmClassifierField(data, len);

    cg()->llvmFree(data, "rule-builder-bytes", t->location());

    setResult(result);
}


