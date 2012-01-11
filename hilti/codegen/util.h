
#ifndef HILTI_CODEGEN_UTIL_H
#define HILTI_CODEGEN_UTIL_H

#include <list>

#include "common.h"

namespace hilti {

class ID;

namespace codegen {
namespace util {

/// Mangles an ID into an internal LLVM-level.
///
/// id: The ID to mangle.
///
/// global: True if the ID corresponds to a module-level entity.
///
/// parent: True if the ID is interpreted relative to a parent scope ID.
///
/// prefix: An optional prefix to prepend to mangeled name.
///
/// internal: True if the resultin mangled name is not visible across LLVM
/// module boundaries.
extern string mangle(shared_ptr<ID> id, bool global, shared_ptr<ID> parent = nullptr, string prefix = "", bool internal = true);

/// Mangles an ID name into an internal LLVM-level.
///
/// name: The name to mangle.
///
/// global: True if the ID corresponds to a module-level entity.
///
/// parent: True if the ID is interpreted relative to a parent scope ID.
///
/// prefix: An optional prefix to prepend to mangeled name.
///
/// internal: True if the resultin mangled name is not visible across LLVM
/// module boundaries.
extern string mangle(const string& name, bool global, shared_ptr<ID> parent = nullptr, string prefix = "", bool internal = true);

/// Checked version of llvm::IRBuilder::CreateCall. Arguments and result are
/// the same, but the function checks that their types match what the function
/// declares.
extern llvm::CallInst* checkedCreateCall(llvm::IRBuilder<>* builder, const string& where, llvm::Value *callee, llvm::ArrayRef<llvm::Value *> args, const llvm::Twine &name="");

/// Returns true if LLVM's module verification indicates a well-formed module.
///
/// module: The module to check.
inline bool llvmVerifyModule(llvm::Module* module) {
    return ! llvm::verifyModule(*module, llvm::PrintMessageAction);
}

/// Returns true if LLVM's module verification indicates a well-formed
/// module.
///
/// module: The module to check.
inline bool llvmVerifyModule(shared_ptr<llvm::Module> module) {
    return llvmVerifyModule(module.get());
}

}
}
}

#endif
