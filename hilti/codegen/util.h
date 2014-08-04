
#ifndef HILTI_CODEGEN_UTIL_H
#define HILTI_CODEGEN_UTIL_H

#include <list>

#include "common.h"

namespace hilti {

class ID;

namespace codegen {

class CodeGen;

namespace util {

/// Custom LLVM Inserter that can insert comments into the code in the form
/// of metadata that our AssemblyAnnotationWriter understands.
class IRInserter : public llvm::IRBuilderDefaultInserter<> {
public:
   /// Creates a new Inserter.
   ///
   /// cg: The code generator to associate with the inserter.
   IRInserter(CodeGen* cg) { _cg = cg; }

protected:
   void InsertHelper(llvm::Instruction *I, const llvm::Twine &Name, llvm::BasicBlock *BB, llvm::BasicBlock::iterator InsertPt) const;

private:
   CodeGen* _cg;
};

/// The IRBuilder used by the code generator.
typedef llvm::IRBuilder<true, llvm::ConstantFolder, IRInserter> IRBuilder;

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
extern llvm::CallInst* checkedCreateCall(IRBuilder* builder, const string& where, llvm::Value *callee, llvm::ArrayRef<llvm::Value *> args, const llvm::Twine &name="");

/// Returns true if LLVM's module verification indicates a well-formed module.
///
/// module: The module to check.
inline bool llvmVerifyModule(llvm::Module* module) {
#ifdef HAVE_LLVM_35
    llvm::raw_os_ostream out(std::cerr);
    return ! llvm::verifyModule(*module, &out);
#else
    return ! llvm::verifyModule(*module, llvm::PrintMessageAction);
#endif    
}

/// Returns true if LLVM's module verification indicates a well-formed
/// module.
///
/// module: The module to check.
inline bool llvmVerifyModule(shared_ptr<llvm::Module> module) {
    return llvmVerifyModule(module.get());
}

/// Returns a LLVM MDNode for a given value.
///
/// ctx: The LLVM context to use.
///
/// v: The value to insert into the meta data node.
extern llvm::MDNode* llvmMdFromValue(llvm::LLVMContext& ctx, llvm::Value* v);

/// Creates a new LLVM builder with a given block as its insertion point.
/// It does however not push it onto stack of builders associated with the
/// current function.
///
/// ctx: The LLVM context to use.
///
/// block: The block to set as insertion point.
///
/// insert_at_beginning: If true, the insertion point is set to the
/// beginning of the block; otherwise to the end.
///
/// Returns: The LLVM IRBuilder.
extern IRBuilder* newBuilder(llvm::LLVMContext& ctx, llvm::BasicBlock* block, bool insert_at_beginning = false);

/// Creates a new LLVM builder with a given block as its insertion point.
/// It does however not push it onto stack of builders associated with the
/// current function.
///
/// cg: The code generator to associate with the builder.
///
/// block: The block to set as insertion point.
///
/// insert_at_beginning: If true, the insertion point is set to the
/// beginning of the block; otherwise to the end.
///
/// Returns: The LLVM IRBuilder.
extern IRBuilder* newBuilder(CodeGen* cg, llvm::BasicBlock* block, bool insert_at_beginning = false);

// Print a string directly to stderr, without any further reliance on
// a HILTI context.
extern void llvmDebugPrintStderr(IRBuilder* builder, const std::string& s);

// Inserts a debu trap, without any further reliance on a HILTI context.
extern void llvmDebugTrap(IRBuilder* builder);

/// Converts a 64-bit value from host-order to network order.
///
/// v: The value to convert.
extern uint64_t hton64(uint64_t v);

/// Converts a 32-bit value from host-order to network order.
///
/// v: The value to convert.
extern uint32_t hton32(uint32_t v);

/// Converts a 16-bit value from host-order to network order.
///
/// v: The value to convert.
extern uint16_t hton16(uint16_t v);

/// Converts a 64-bit value from network order to host order.
///
/// v: The value to convert.
extern uint64_t ntoh64(uint64_t v);

/// Converts a 32-bit value from network order to host order.
///
/// v: The value to convert.
extern uint32_t ntoh32(uint32_t v);

/// Converts a 16-bit value from network order to host order.
///
/// v: The value to convert.
extern uint16_t ntoh16(uint16_t v);


}
}
}

#endif
