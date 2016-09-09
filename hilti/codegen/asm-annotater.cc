
#include "asm-annotater.h"
#include "llvm-common.h"
#include "symbols.h"

using namespace hilti;
using namespace codegen;

void AssemblyAnnotationWriter::emitInstructionAnnot(const llvm::Instruction* i,
                                                    llvm::formatted_raw_ostream& out)
{
    auto md = i->getMetadata("symbols::MetaComment");

    if ( md ) {
        out << "\n";
        for ( int i = 0; i < md->getNumOperands(); ++i ) {
            auto str = llvm::cast<llvm::MDString>(md->getOperand(i));
            assert(str);
            out << "  ; " << str->getString() << "\n";
        }
    }
}
