
#ifndef HILTI_CODEGEN_COMMON_H
#define HILTI_CODEGEN_COMMON_H

#include <llvm/Config/llvm-config.h>

#if (LLVM_VERSION_MAJOR == 3) && (LLVM_VERSION_MINOR == 3)
#define HAVE_LLVM_33
#elif (LLVM_VERSION_MAJOR == 3) && (LLVM_VERSION_MINOR == 4)
#define HAVE_LLVM_34
#else
#error LLVM version not supported.
#endif

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Linker.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/Parser.h>
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/Host.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Assembly/AssemblyAnnotationWriter.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/system_error.h>
#include <llvm/Analysis/TargetTransformInfo.h>

#ifdef HAVE_LLVM_33
#include <llvm/Support/PathV1.h>
#else
#include <llvm/Support/Path.h>
#endif

#include "../common.h"

#endif
