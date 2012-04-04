
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

    ffi_type* args_ffi[nargs+1];

    for ( int i = 0; i < args.size(); ++i )
        args_ffi[i] = _llvmToCif(cg(), args[i]);

    auto cif = new ffi_cif;
    auto rc = ffi_prep_cif(cif, FFI_DEFAULT_ABI, 1, rtype_ffi, args_ffi);
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

llvm::Function* abi::X86_64::createFunction(const string& name, llvm::Type* rtype, const ABI::arg_list& args, llvm::GlobalValue::LinkageTypes linkage, llvm::Module* module)
{
    std::vector<string> arg_names;
    std::vector<llvm::Type*> arg_types;

    for ( auto a : args ) {
        arg_names.push_back(a.first);
        arg_types.push_back(a.second);
    }

    auto cif = getCIF(name, rtype, arg_types);
    auto ret_in_mem = returnInMemory(cif);


    if ( ret_in_mem ) {
        // Move return type to new first paramter.
        arg_names.insert(arg_names.begin(), ".sret");
        arg_types.insert(arg_types.begin(), rtype->getPointerTo());
        rtype = llvm::Type::getVoidTy(cg()->llvmContext());
    }

    auto ftype = llvm::FunctionType::get(rtype, arg_types, false);
    auto func = llvm::Function::Create(ftype, linkage, name, module);

    if ( ret_in_mem ) {
        func->addAttribute(0, llvm::Attribute::StructRet);
        func->addAttribute(0, llvm::Attribute::NoAlias);
    }

    auto i = arg_names.begin();
    for ( auto a = func->arg_begin(); a != func->arg_end(); ++a )
        a->setName(*i++);

    return func;
}

llvm::Value* abi::X86_64::createCall(llvm::Function *callee, std::vector<llvm::Value *> args)
{
    ABI::argument_type_list dummy;

    // Returns cached copy.
    auto cif = getCIF(callee->getName(), 0, dummy);
    auto ret_in_mem = returnInMemory(cif);

    if ( ret_in_mem ) {
        // Add initial parameter for return value.
        auto rtype = llvm::cast<llvm::PointerType>(callee->arg_begin()->getType())->getElementType();
        auto agg = cg()->llvmAddTmp("agg.sret", rtype, nullptr, true);
        args.insert(args.begin(), agg);
    }

    // Add hidden argument at the front that will receive the return value.
    llvm::Value* result = cg()->llvmCreateCall(callee, args);

    if ( ret_in_mem )
        result = cg()->builder()->CreateLoad(callee->arg_begin());

    return result;
}

