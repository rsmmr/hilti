
#include <util/util.h>

#include "../id.h"

#include "util.h"
#include "codegen.h"

using namespace hilti;
using namespace codegen;

void codegen::util::IRInserter::InsertHelper(llvm::Instruction *I, const llvm::Twine &Name, llvm::BasicBlock *BB, llvm::BasicBlock::iterator InsertPt) const
{
    // Do the default stuff first.
    llvm::IRBuilderDefaultInserter<>::InsertHelper(I, Name, BB, InsertPt);

    if ( ! _cg )
        return;

    // Add comment if the codegen has one for us.
    const string& comment = _cg->nextComment();
    if ( _cg && comment.size() ) {
        auto cmt = llvm::MDString::get(_cg->llvmContext(), comment);
        auto md = codegen::util::llvmMdFromValue(_cg->llvmContext(), cmt);
        I->setMetadata(symbols::MetaComment, md);
        _cg->clearNextComment();
    }
}

string codegen::util::mangle(const string& name, bool global, shared_ptr<ID> parent, string prefix, bool internal)
{
    static char const* const hex = "0123456789abcdef";

    string normalized = name;

    normalized = ::util::strreplace(normalized, "::", "_");
    normalized = ::util::strreplace(normalized, "<", "_");
    normalized = ::util::strreplace(normalized, ">", "_");
    normalized = ::util::strreplace(normalized, ",", "_");
    normalized = ::util::strreplace(normalized, " ", "_");
    normalized = ::util::strreplace(normalized, "-", "_");
    normalized = ::util::strreplace(normalized, "__", "_");
    normalized = ::util::strreplace(normalized, "@", "l_");
    normalized = ::util::strreplace(normalized, "*", "any");

    while ( ::util::endsWith(normalized, "_") )
        normalized = normalized.substr(0, normalized.size() - 1);

    string s = "";

    for ( auto c : normalized ) {
        if ( isalnum(c) || c == '_' ) {
            s += c;
            continue;
        }

        s += "x";
        s += hex[c >> 4];
        s += hex[c % 0x0f];
    }

    s = ::util::strreplace(s, "__", "x5fx5f");

    if ( parent ) {
        auto mangled_parent = mangle(parent, global, nullptr, "", internal);
        s = ::util::strtolower(mangled_parent) + "_" + s;
    }

    if ( prefix.size() )
        s = prefix + "." + s;

    if ( global && internal && ! parent )
        s = ".hlt." + s;

    return s;
}

string codegen::util::mangle(shared_ptr<ID> id, bool global, shared_ptr<ID> parent, string prefix, bool internal)
{
    return mangle(id->pathAsString(), global, parent, prefix, internal);
}

static void _dumpCall(llvm::Function* func, llvm::ArrayRef<llvm::Value *> args, const string& where, const string& msg)
{
    auto ftype = func->getFunctionType();

    llvm::raw_os_ostream os(std::cerr);

    os << "\n";
    os << "=== Function call mismatch in " << where << ": " << msg << "\n";
    os << "\n";
    os << "-- Prototype:\n";
    func->print(os);
    os << "\n";

    for ( int i = 0; i < ftype->getNumParams(); ++i ) {
        os << "   [" << i+1 << "] ";
        ftype->getParamType(i)->print(os);
        os << "\n";
    }

    os << "\n";

    os << "-- Arguments:\n";
    os << "\n";

    if ( ! args.size() )
        os << "   None given.\n";

    for ( int i = 0; i < args.size(); ++i ) {
        os << "   [" << i+1 << "] ";
        args[i]->getType()->print(os);
        os << "\n";
    }

    os << "\n";
    os.flush();

    ::util::abort_with_backtrace();
}

llvm::CallInst* codegen::util::checkedCreateCall(IRBuilder* builder, const string& where, llvm::Value *callee, llvm::ArrayRef<llvm::Value *> args, const llvm::Twine &name)
{
    assert(callee);
    assert(llvm::isa<llvm::Function>(callee));

    auto func = llvm::cast<llvm::Function>(callee);
    auto ftype = func->getFunctionType();

    if ( ftype->getNumParams() != args.size() )
        _dumpCall(func, args, where, ::util::fmt("argument mismatch, LLVM function expects %d but got %d",
                                                 ftype->getNumParams(), args.size()));

    for ( int i = 0; i < args.size(); ++i ) {
        auto t1 = ftype->getParamType(i);
        auto t2 = args[i]->getType();

        if ( t1 != t2 )
            _dumpCall(func, args, where, ::util::fmt("type of parameter %d does not match prototype", i+1));
    }

    return builder->CreateCall(callee, args, name);
}

llvm::MDNode* codegen::util::llvmMdFromValue(llvm::LLVMContext& ctx, llvm::Value* v)
{
    return llvm::MDNode::get(ctx, llvm::ArrayRef<llvm::Value*>(&v, 1));
}

IRBuilder* codegen::util::newBuilder(CodeGen* cg, llvm::BasicBlock* block, bool insert_at_beginning)
{
    llvm::ConstantFolder folder;

    auto builder = new IRBuilder(cg->llvmContext(), folder, util::IRInserter(cg));

    if ( ! (insert_at_beginning && block->getFirstInsertionPt() != block->end() ) )
        builder->SetInsertPoint(block);
    else
        builder->SetInsertPoint(block->getFirstInsertionPt());

    return builder;
}

IRBuilder* codegen::util::newBuilder(llvm::LLVMContext& ctx, llvm::BasicBlock* block, bool insert_at_beginning)
{
    llvm::ConstantFolder folder;

    auto builder = new IRBuilder(ctx, folder, util::IRInserter(0));

    if ( ! insert_at_beginning )
        builder->SetInsertPoint(block);
    else
        builder->SetInsertPoint(block->getFirstInsertionPt());

    return builder;
}

#define _flip(x) \
    ((((x) & 0xff00000000000000LL) >> 56) | \
    (((x) & 0x00ff000000000000LL) >> 40) | \
    (((x) & 0x0000ff0000000000LL) >> 24) | \
    (((x) & 0x000000ff00000000LL) >> 8) | \
    (((x) & 0x00000000ff000000LL) << 8) | \
    (((x) & 0x0000000000ff0000LL) << 24) | \
    (((x) & 0x000000000000ff00LL) << 40) | \
    (((x) & 0x00000000000000ffLL) << 56))

uint64_t codegen::util::hton64(uint64_t v)
{
#if ! __BIG_ENDIAN__
    return _flip(v);
#else
    return v;
#endif
}

uint32_t codegen::util::hton32(uint32_t v)
{
    return ntohl(v);
}

uint16_t codegen::util::hton16(uint16_t v)
{
    return ntohs(v);
}

uint64_t codegen::util::ntoh64(uint64_t v)
{
#if ! __BIG_ENDIAN__
    return _flip(v);
#else
    return v;
#endif
}

uint32_t codegen::util::ntoh32(uint32_t v)
{
    return ntohl(v);
}

uint16_t codegen::util::ntoh16(uint16_t v)
{
    return ntohs(v);
}
