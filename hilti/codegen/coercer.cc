
#include "../hilti.h"

#include "codegen.h"
#include "coercer.h"

using namespace hilti;
using namespace codegen;

codegen::Coercer::Coercer(CodeGen* cg)
    : CGVisitor<llvm::Value*, llvm::Value*, shared_ptr<hilti::Type>>(cg, "codegen::Coercer")
{
}

codegen::Coercer::~Coercer()
{
}

llvm::Value* codegen::Coercer::llvmCoerceTo(llvm::Value* value, shared_ptr<hilti::Type> src,
                                            shared_ptr<hilti::Type> dst, bool cctor)
{
    _cctor = cctor;

    if ( src->equal(dst) || dst->equal(src) ) {
        if ( cctor )
            cg()->llvmCctor(value, src, false, "Coercer::llvmCoerceTo/equal");

        return value;
    }

    if ( ast::rtti::isA<type::OptionalArgument>(dst) )
        return llvmCoerceTo(value, src, ast::rtti::tryCast<type::OptionalArgument>(dst)->argType(),
                            cctor);

    setArg1(value);
    setArg2(dst);

    llvm::Value* result;
    bool success = processOne(src, &result);
    assert(success);
    _UNUSED(success);

    if ( cctor )
        cg()->llvmDtor(value, src, false, "Coercer::llvmCoerceTo");

    return result;
}

void codegen::Coercer::visit(type::Integer* t)
{
    auto val = arg1();
    auto dst = arg2();

    shared_ptr<type::Bool> dst_b = ast::rtti::tryCast<type::Bool>(dst);

    if ( dst_b ) {
        auto width = llvm::cast<llvm::IntegerType>(val->getType())->getBitWidth();
        auto result = builder()->CreateICmpNE(val, cg()->llvmConstInt(0, width));
        setResult(result);
        return;
    }

    assert(false);
}

void codegen::Coercer::visit(type::Tuple* t)
{
    auto val = arg1();
    auto dst = arg2();

    auto rtype = ast::rtti::tryCast<type::Reference>(dst);

    if ( rtype ) {
        auto stype = ast::rtti::tryCast<type::Struct>(rtype->argType());
        assert(stype);

        auto sval = cg()->llvmStructNew(stype, _cctor);
        setResult(sval);

        // If the tuple is empty, we just return the newly created struct.
        if ( t->typeList().empty() )
            return;

        assert(t->typeList().size() == stype->typeList().size());

        auto idx = 0;
        for ( auto i : ::util::zip2(t->typeList(), stype->typeList()) ) {
            if ( ast::rtti::isA<type::Unset>(i.first) ) {
                ++idx;
                continue;
            }

            auto v = cg()->llvmExtractValue(val, idx);
            v = cg()->llvmCoerceTo(v, i.first, i.second, false);
            cg()->llvmStructSet(stype, sval, idx++, v);
        }

        return;
    }

    assert(false); // Need to implement other coercions.

    setResult(val);
}

void codegen::Coercer::visit(type::Address* t)
{
    auto val = arg1();
    auto dst = arg2();

    shared_ptr<type::Network> dst_net = ast::rtti::tryCast<type::Network>(dst);

    if ( dst_net ) {
        auto a = cg()->llvmExtractValue(val, 0);
        auto b = cg()->llvmExtractValue(val, 1);
        auto w = cg()->llvmConstInt(128, 8);

        CodeGen::value_list vals = {a, b, w};
        auto net = cg()->llvmValueStruct(vals);

        setResult(net);
        return;
    }

    assert(false);
}

void codegen::Coercer::visit(type::Reference* r)
{
    auto val = arg1();
    auto dst = arg2();

    shared_ptr<type::Reference> dst_ref = ast::rtti::tryCast<type::Reference>(dst);

    if ( dst_ref ) {
        if ( ast::rtti::isA<type::RegExp>(r->argType()) &&
             ast::rtti::isA<type::RegExp>(dst_ref->argType()) ) {
            auto top = dst_ref->argType();
            CodeGen::expr_list args = {builder::type::create(top),
                                       builder::codegen::create(dst, val)};
            auto func = _cctor ? "hlt::regexp_new_from_regexp" : "hlt::regexp_new_from_regexp";
            auto nregexp = cg()->llvmCall(func, args);
            setResult(nregexp);
            return;
        }

        // Case void* pointers to their destination type.
        if ( val->getType() == llvm::Type::getInt8PtrTy(cg()->llvmContext()) ) {
            auto casted = cg()->builder()->CreateBitCast(val, cg()->llvmType(dst));
            setResult(casted);
            return;
        }
        assert(false);
    }

    auto dst_bool = ast::rtti::tryCast<type::Bool>(dst);

    if ( dst_bool ) {
        auto non_null = cg()->builder()->CreateICmpNE(val, cg()->llvmConstNull(val->getType()));
        setResult(non_null);
        return;
    }

    assert(false);
}

void codegen::Coercer::visit(type::CAddr* r)
{
    auto val = arg1();
    auto dst = arg2();

    auto dst_bool = ast::rtti::tryCast<type::Bool>(dst);

    if ( dst_bool ) {
        auto non_null = cg()->builder()->CreateICmpNE(val, cg()->llvmConstNull(val->getType()));
        setResult(non_null);
        return;
    }

    assert(false);
}

void codegen::Coercer::visit(type::Unset* r)
{
    auto dst = arg2();

    assert(! ast::rtti::tryCast<type::Unset>(dst));
    setResult(cg()->llvmInitVal(dst));
}

void codegen::Coercer::visit(type::Union* t)
{
    auto val = arg1();
    auto dst = arg2();

    auto type = t->sharedPtr<type::Union>();

    auto dst_bool = ast::rtti::tryCast<type::Bool>(dst);

    if ( dst_bool ) {
        auto addr = cg()->llvmAddTmp("u", cg()->llvmType(type), val, false);
        CodeGen::value_list args = {cg()->llvmRtti(type),
                                    cg()->builder()->CreateBitCast(addr, cg()->llvmTypePtr())};

        auto utype = cg()->llvmCallC("__hlt_union_type", args, false, false);
        auto non_null = cg()->builder()->CreateICmpNE(utype, cg()->llvmConstNull(utype->getType()));
        setResult(non_null);
        return;
    }

    auto dst_union = ast::rtti::tryCast<type::Union>(dst);

    if ( dst_union && t->wildcard() ) {
        auto t = cg()->llvmType(dst_union);
        auto result = cg()->llvmConstNull(t);
        cg()->llvmInsertValue(result, cg()->llvmConstInt(-1, 32), 0);
        setResult(result);
        return;
    }

    assert(false);
}
