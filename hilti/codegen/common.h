
#ifndef HILTI_CODEGEN_COMMON_H
#define HILTI_CODEGEN_COMMON_H

#include <llvm/LLVMContext.h>
#include <llvm/Metadata.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Linker.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/Parser.h>
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/PathV1.h>
#include <llvm/Assembly/AssemblyAnnotationWriter.h>
#include <llvm/Intrinsics.h>

#include "../common.h"

#endif
