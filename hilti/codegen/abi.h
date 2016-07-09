
#ifndef HILTI_CODEGEN_ABI_H
#define HILTI_CODEGEN_ABI_H

#include "common.h"
#include "type-builder.h"

#include <ffi.h>
#undef X86_64

namespace llvm {
class Value;
}

namespace hilti {
namespace codegen {

class CodeGen;

/// Nase class providing the ABI interface.
class ABI {
public:
    typedef std::vector<std::pair<string, llvm::Type*>> arg_list;

    /// XXX
    virtual llvm::Function* createFunction(const string& name, llvm::Type* rtype,
                                           const arg_list& args,
                                           llvm::GlobalValue::LinkageTypes linkage,
                                           llvm::Module* module,
                                           type::function::CallingConvention cc) = 0;

    // XXX
    virtual llvm::FunctionType* createFunctionType(llvm::Type* rtype, const ABI::arg_list& args,
                                                   type::function::CallingConvention cc) = 0;

    /// XXX
    virtual llvm::Value* createCall(llvm::Value* callee, std::vector<llvm::Value*> args,
                                    llvm::Type* rtype, const arg_list& targs,
                                    type::function::CallingConvention cc) = 0;

    /// XXX
    CodeGen* cg() const
    {
        return _cg;
    }

    /// XXX
    const string& targetTriple() const
    {
        return _triple;
    }

    /// XXX
    static unique_ptr<ABI> createABI(CodeGen* cg);

    enum ByteOrder { LittleEndian, BigEndian };

    /// XXXX
    virtual ByteOrder byteOrder() const;

    /// XXX
    virtual string dataLayout() const = 0;

protected:
    ABI()
    {
    }

    typedef std::vector<llvm::Type*> argument_type_list;
    ffi_cif* getCIF(llvm::Type* rtype, const ABI::argument_type_list& args);

    llvm::Type* mapToIntType(llvm::StructType* stype);

private:
    string _triple;
    CodeGen* _cg = nullptr;
};

namespace abi {

/// ABI for X86_64.
///
/// \todo The only ABI-specific transformation we currently do is adjusting
/// the return value to be passed via a hidding parameter if it needs to be.
/// We don't handle call arguments yet that need to be passed in memory.
class X86_64 : public ABI {
public:
    enum Flavor { FLAVOR_DEFAULT, FLAVOR_DARWIN };

    X86_64(Flavor flavor)
    {
        _flavor = flavor;
    }

    llvm::Function* createFunction(const string& name, llvm::Type* rtype, const ABI::arg_list& args,
                                   llvm::GlobalValue::LinkageTypes linkage, llvm::Module* module,
                                   type::function::CallingConvention cc) override;
    llvm::FunctionType* createFunctionType(llvm::Type* rtype, const arg_list& args,
                                           type::function::CallingConvention cc) override;
    llvm::Value* createCall(llvm::Value* callee, std::vector<llvm::Value*> args, llvm::Type* rtype,
                            const arg_list& targs, type::function::CallingConvention cc) override;

    string dataLayout() const override;

private:
    struct ClassifiedArguments {
        bool return_in_mem; // True if result is passed via pointer.
        llvm::Type*
            return_type; // ABI type of the return value (not yet a pointer if passed in mem).
        std::vector<bool> args_in_mem; // One per arguments, each true if passed via pointer.
        std::vector<llvm::Type*>
            arg_types; // Final ABI type of argument (not yet a pointer if passed in mem).
    };

    ClassifiedArguments classifyArguments(const string& name, llvm::Type* rtype,
                                          const ABI::arg_list& args,
                                          type::function::CallingConvention cc);
    ClassifiedArguments classifyArguments(
        const string& name); // The other variant must have been called for the same name already.

    std::tuple<llvm::FunctionType*, ClassifiedArguments, std::vector<string>, std::vector<int>, int>
    createFunctionTypeInternal(llvm::Type* rtype, const ABI::arg_list& args,
                               type::function::CallingConvention cc, const string& name);

    Flavor _flavor;

    typedef std::map<string, ClassifiedArguments> ca_map;
    ca_map _classified_arguments;
};
}
}
}

#endif
