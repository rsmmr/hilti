
#include "abi.h"

using namespace hilti;
using namespace codegen;


unique_ptr<ABI> ABI::createABI(llvm::Module* module)
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
    return abi;
}

void abi::X86_64::adaptFunctionType(argument_type* rtype, argument_type_list* args)
{
}

void abi::X86_64::adaptFunctionCall(argument_list* args)
{
}

llvm::Value* abi::X86_64::getFunctionResult(llvm::CallInst* call)
{
    return call;
}

