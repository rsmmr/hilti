
#include "../module.h"
#include "codegen.h"
#include "abi.h"

using namespace hilti;
using namespace codegen;

unique_ptr<ABI> ABI::createABI(CodeGen* cg)
{
    string striple = llvm::sys::getDefaultTargetTriple();
    llvm::Triple triple(striple);

    unique_ptr<ABI> abi;

    if ( triple.getArch() == llvm::Triple::x86_64 ) {
        abi::X86_64::Flavor flavor = abi::X86_64::DEFAULT;

        if ( triple.isOSDarwin() )
            flavor = abi::X86_64::DARWIN;

        abi = unique_ptr<ABI>(new abi::X86_64(flavor));
    }


    if ( ! abi ) {
        fprintf(stderr, "unsupported platform %s\n", striple.c_str());
        exit(1);
    }

    abi->_triple = striple;
    abi->_cg = cg;

    return abi;
}

ABI::ByteOrder ABI::byteOrder() const
{
    if ( llvm::sys::isLittleEndianHost() )
        return LittleEndian;

    if ( llvm::sys::isBigEndianHost() )
        return BigEndian;

    cg()->internalError("unknown endianess of target arch");
    assert(false);
    return BigEndian; // Cannot be reached but make compiler happy.
}

// Converts LLVM type into libffi's description.
static ffi_type* _llvmToCif(CodeGen* cg, llvm::Type* type)
{
    switch ( type->getTypeID() ) {

     case llvm::Type::VoidTyID:
        return &ffi_type_void;

     case llvm::Type::DoubleTyID:
        return &ffi_type_double;

     case llvm::Type::PointerTyID:
        return &ffi_type_pointer;

     case llvm::Type::IntegerTyID: {
         int w = llvm::cast<llvm::IntegerType>(type)->getBitWidth();

         if ( w <= 8 )
             return &ffi_type_sint8;

         if ( w <= 16 )
             return &ffi_type_sint16;

         if ( w <= 32 )
             return &ffi_type_sint32;

         if ( w <= 64 )
             return &ffi_type_sint64;

         cg->internalError("integer bitwidth >64 in llvmToCif");
         break;
     }

     case llvm::Type::StructTyID: {
         auto sty = llvm::cast<llvm::StructType>(type);
         int n = sty->getNumElements();

         ffi_type** elems_ffi = new ffi_type*[n+1];

         for ( int i = 0; i < n; i++ )
             elems_ffi[i] = _llvmToCif(cg, sty->getElementType(i));
         elems_ffi[n] = 0;

         auto cif = new ffi_type;
         cif->size = cif->alignment = 0;
         cif->type = FFI_TYPE_STRUCT;
         cif->elements = elems_ffi;

         return cif;
     }

     case llvm::Type::HalfTyID:
     case llvm::Type::FloatTyID:
     case llvm::Type::X86_FP80TyID:
     case llvm::Type::FP128TyID:
     case llvm::Type::PPC_FP128TyID:
     case llvm::Type::LabelTyID:
     case llvm::Type::MetadataTyID:
     case llvm::Type::X86_MMXTyID:
     case llvm::Type::FunctionTyID:
     case llvm::Type::ArrayTyID:
     case llvm::Type::VectorTyID:
     default:
        cg->internalError("unsupport argument type in llvmToCif");
    }

    assert(false); // cannot be reached.
}

ffi_cif* ABI::getCIF(const string& name, llvm::Type* rtype, const argument_type_list& args)
{
    auto i = _cifs.find(name);

    if ( i != _cifs.end() )
        return (*i).second;

    assert(rtype);

    auto rtype_ffi = _llvmToCif(cg(), rtype);

    auto nargs = args.size();

    ffi_type** args_ffi = new ffi_type*[nargs];

    for ( int i = 0; i < nargs; ++i )
        args_ffi[i] = _llvmToCif(cg(), args[i]);

    auto cif = new ffi_cif;
    auto rc = ffi_prep_cif(cif, FFI_DEFAULT_ABI, nargs, rtype_ffi, args_ffi);
    assert(rc == FFI_OK);

    _cifs.insert(std::make_pair(name, cif));

    return cif;
}

bool abi::X86_64::returnInMemory(ffi_cif* cif)
{
    // From libffi/src/x86/ffi64.c.
    return (cif->rtype->type == FFI_TYPE_STRUCT
            && (cif->flags & 0xff) == FFI_TYPE_VOID);
}

/// X86_64 ABI.

#include "libffi/src/x86/ffi64.h"

/// FIXME: We currently just generally pass structures in memory for HILTI
/// calling convention. For HILTI_C cc we leave them untouched.
llvm::Function* abi::X86_64::createFunction(const string& name, llvm::Type* rtype, const ABI::arg_list& args, llvm::GlobalValue::LinkageTypes linkage, llvm::Module* module, type::function::CallingConvention cc)
{
    std::vector<string> arg_names;
    std::vector<llvm::Type*> arg_types;

    for ( auto a : args ) {
        arg_names.push_back(a.first);
        arg_types.push_back(a.second);
    }

    auto cif = getCIF(name, rtype, arg_types);
    auto ret_in_mem = returnInMemory(cif);

    auto ffi_avn = cif->nargs;
    auto ffi_arg_types = cif->arg_types;

    std::vector<llvm::Type*> ntypes;
    std::vector<string> nnames;
    std::vector<int> byvals;

    if ( ret_in_mem ) {
        // Move return type to new first paramter.
        ntypes.push_back(rtype->getPointerTo());
        nnames.push_back("agg.sret");
        rtype = llvm::Type::getVoidTy(cg()->llvmContext());
    }

    for ( int i = 0; i < ffi_avn; ++i ) {

        auto name = arg_names[i];
        auto llvm_type = arg_types[i];
        auto ffi_type = ffi_arg_types[i];

        if ( ffi_type->type == FFI_TYPE_STRUCT && cc == type::function::HILTI ) {
            // FIXME: We cheat for now and pass all structs in memory. We
            // should mimic the code from libffi/src/x86/ffi64.c here instead
            // (if we can?!).
            byvals.push_back(ntypes.size());
            ntypes.push_back(llvm_type->getPointerTo());
            nnames.push_back(name);
        }

        else {
            ntypes.push_back(llvm_type);
            nnames.push_back(name);
        }
    }

    auto ftype = llvm::FunctionType::get(rtype, ntypes, false);
    auto func = llvm::Function::Create(ftype, linkage, name, module);

    if ( ret_in_mem ) {
        func->addAttribute(1, llvm::Attribute::StructRet);
        func->addAttribute(1, llvm::Attribute::NoAlias);
    }

    for ( auto i : byvals ) {
        func->addAttribute(i+1, llvm::Attribute::ByVal);
        func->addAttribute(i+1, llvm::Attribute::NoAlias);
    }

    auto i = nnames.begin();
    for ( auto a = func->arg_begin(); a != func->arg_end(); ++a )
        a->setName(*i++);

    return func;
}

llvm::Value* abi::X86_64::createCall(llvm::Function *callee, std::vector<llvm::Value *> args, type::function::CallingConvention cc)
{
    ABI::argument_type_list dummy;

    // Returns cached copy.
    auto cif = getCIF(callee->getName(), 0, dummy);
    auto ret_in_mem = returnInMemory(cif);

    auto ffi_avn = cif->nargs;
    auto ffi_arg_types = cif->arg_types;

    std::vector<llvm::Value*> nargs;

    llvm::Value* agg_ret = nullptr;

    if ( ret_in_mem ) {
        // Add initial parameter for return value.
        auto rtype = llvm::cast<llvm::PointerType>(callee->arg_begin()->getType())->getElementType();
        agg_ret = cg()->llvmAddTmp("agg.sret", rtype, nullptr, true);
        nargs.push_back(agg_ret);
    }

    for ( int i = 0; i < ffi_avn; ++i ) {

        auto llvm_val = args[i];
        auto ffi_type = ffi_arg_types[i];

        if ( ffi_type->type == FFI_TYPE_STRUCT && cc == type::function::HILTI ) {
            // FIXME: We cheat for now and pass all structs in memory. We
            // should mimic the code from libffi/src/x86/ffi64.c here instead
            // (if we can?!).
            auto agg = cg()->llvmAddTmp("agg.arg", llvm_val->getType(), llvm_val);
            nargs.push_back(agg);
        }

        else
            nargs.push_back(llvm_val);
    }

    // Add hidden argument at the front that will receive the return value.
    llvm::Value* result = cg()->llvmCreateCall(callee, nargs);

    if ( ret_in_mem )
        result = cg()->builder()->CreateLoad(agg_ret);

    return result;
}

