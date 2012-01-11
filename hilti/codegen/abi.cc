
#include "abi.h"

inline static bool isArch(const char* arch) {
    return targetTriple().startswith(arch);
}

inline static bool isDarwin() {
    return targetTriple().find("-darwin-") != string::npos;
}

const string& targetTriple()
{
    static string triple = "";

    if ( triple.size() == 0 )
        triple = llvm::Module::getTargetTriple();

    return triple;
}

bool isLittleEndian()
{
    return llvm::Module::getEndianness() == llvm::Module::LittleEndianess;
}

bool x86_64_applyCallingConvection(shared_ptr<Argument> result, const std::list<shared_ptr<Argument>>& arguments)
{
    // FIXME.
    result.setAction(Argument::DIRECT);

    for ( arg : arguments ) {
        arg.setAction(Argument::DIRECT);
    }

    return true;
}

bool isSupportedArchitecture()
{
    return isArch("x86_64");
}

bool applyCallingConvention(shared_ptr<Argument> result, const std::list<shared_ptr<Argument>>& arguments)
{
    if ( isArch("x86_64") )
        return x86_64_applyCallingConvention(result, arguments);

    assert(false);
}


