
#include "../hilti.h"

#include "../builder/nodes.h"
#include "codegen.h"
#include "field-builder.h"

using namespace hilti;
using namespace hilti::codegen;

FieldBuilder::FieldBuilder(CodeGen* cg)
    : CGVisitor<llvm::Value*, shared_ptr<Type>, llvm::Value*>(cg, "codegen::FieldBuilder")
{
}

FieldBuilder::~FieldBuilder()
{
}

llvm::Value* FieldBuilder::llvmClassifierField(shared_ptr<Type> field_type,
                                               shared_ptr<Type> src_type, llvm::Value* src_val,
                                               const Location& l)
{
    auto r = ast::rtti::tryCast<type::Reference>(field_type);
    auto t = r ? r->argType() : field_type;

    assert(ast::type::hasTrait<type::trait::Classifiable>(t));

    _location = l;

    llvm::Value* result = nullptr;
    setArg1(field_type);
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

    std::vector<llvm::Type*> fields = {a1->getType(), a2->getType()};
    auto packed = cg()->llvmTypeStruct("", fields, true);
    auto tmp = cg()->llvmAddTmp("addr-field", packed, nullptr, true);

    auto addr = cg()->llvmGEP(tmp, cg()->llvmGEPIdx(0), cg()->llvmGEPIdx(0));
    cg()->llvmCreateStore(a1, addr);
    addr = cg()->llvmGEP(tmp, cg()->llvmGEPIdx(0), cg()->llvmGEPIdx(1));
    cg()->llvmCreateStore(a2, addr);

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

    std::vector<llvm::Type*> fields = {a1->getType(), a2->getType()};
    auto packed = cg()->llvmTypeStruct("", fields, true);
    auto tmp = cg()->llvmAddTmp("net-field", packed, nullptr, true);

    auto addr = cg()->llvmGEP(tmp, cg()->llvmGEPIdx(0), cg()->llvmGEPIdx(0));
    cg()->llvmCreateStore(a1, addr);
    addr = cg()->llvmGEP(tmp, cg()->llvmGEPIdx(0), cg()->llvmGEPIdx(1));
    cg()->llvmCreateStore(a2, addr);

    auto len = cg()->llvmSizeOf(packed);
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

    auto tmp = cg()->llvmAddTmp("bool-field", src_val->getType(), nullptr, true);
    cg()->llvmCreateStore(src_val, tmp);

    auto len = cg()->llvmSizeOf(src_val->getType());
    auto result = cg()->llvmClassifierField(tmp, len);

    setResult(result);
}

void FieldBuilder::visit(type::Integer* t)
{
    auto src_type = arg1();
    auto src_val = arg2();

    auto tmp = cg()->llvmAddTmp("int-field", src_val->getType(), nullptr, true);
    cg()->llvmCreateStore(cg()->llvmHtoN(src_val), tmp);

    auto len = cg()->llvmSizeOf(src_val->getType());
    auto result = cg()->llvmClassifierField(tmp, len);

    setResult(result);
}

void FieldBuilder::visit(type::Bytes* t)
{
    auto src_type = arg1();
    auto src_val = arg2();

    auto cgb = builder::codegen::create(src_type, src_val);
    auto len = cg()->llvmCall("hlt::bytes_len", {cgb}, false);

    // Don't used CG's version here, as the length is dynamic and can't be
    // put into the entry block.
    auto buffer = cg()->builder()->CreateAlloca(cg()->llvmTypeInt(8), len);
    auto data = cg()->llvmCallC("hlt_bytes_to_raw", {buffer, len, src_val}, true, true);
    auto result = cg()->llvmClassifierField(data, len);

    setResult(result);
}
