
#ifndef HILTI_CODEGEN_ASM_ANNOTATOR_H
#define HILTI_CODEGEN_ASM_ANNOTATOR_H

#include "common.h"

namespace hilti {
namespace codegen {

/// Custom LLVM IR annotation writer that adds comments to printed IR with
/// descriptive information inserted by the HILTI code generator.
class AssemblyAnnotationWriter : public llvm::AssemblyAnnotationWriter {
public:
   void emitInstructionAnnot(const llvm::Instruction *, llvm::formatted_raw_ostream &) override;

   // We don't use these currently.
   //
   // void emitFunctionAnnot(const Function *, formatted_raw_ostream &) override;
   // void emitBasicBlockStartAnnot(const BasicBlock *, formatted_raw_ostream &) override;
   // void emitBasicBlockEndAnnot(const BasicBlock *, formatted_raw_ostream &) override;
   // void printInfoComment(const Value &, formatted_raw_ostream &) override;
};

}
}

#endif
