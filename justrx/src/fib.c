// Headers required by LLVM
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>


// General stuff
#include <stdlib.h>
#include <stdio.h>

extern void hello() {
	printf("Hello world");
}

int fib (int argc, char const *argv[])
{
  char *error = NULL; // Used to retrieve messages from functions

  LLVMLinkInJIT();
  LLVMInitializeNativeTarget();

  LLVMModuleRef mod = LLVMModuleCreateWithName("fac_module");
  LLVMTypeRef fac_args[] = { LLVMInt32Type() };
  LLVMValueRef fac = LLVMAddFunction(mod, "fac", LLVMFunctionType(LLVMInt32Type(), fac_args, 1, 0));
  LLVMSetFunctionCallConv(fac, LLVMCCallConv);
  LLVMValueRef n = LLVMGetParam(fac, 0);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(fac, "entry");

//  LLVMBasicBlockRef iftrue = LLVMAppendBasicBlock(fac, "iftrue");
 // LLVMBasicBlockRef iffalse = LLVMAppendBasicBlock(fac, "iffalse");
  //LLVMBasicBlockRef end = LLVMAppendBasicBlock(fac, "end");
  LLVMBuilderRef builder = LLVMCreateBuilder();

  LLVMPositionBuilderAtEnd(builder, entry);
/*
  LLVMValueRef If = LLVMBuildICmp(builder, LLVMIntEQ, n, LLVMConstInt(LLVMInt32Type(), 0, 0), "n == 0");
  LLVMBuildCondBr(builder, If, iftrue, iffalse);

  LLVMPositionBuilderAtEnd(builder, iftrue);
  LLVMValueRef res_iftrue = LLVMConstInt(LLVMInt32Type(), 1, 0);
  LLVMBuildBr(builder, end);

  LLVMPositionBuilderAtEnd(builder, iffalse);
  LLVMValueRef n_minus = LLVMBuildSub(builder, n, LLVMConstInt(LLVMInt32Type(), 1, 0), "n - 1");
  LLVMValueRef call_fac_args[] = {n_minus};
  LLVMValueRef call_fac = LLVMBuildCall(builder, fac, call_fac_args, 1, "fac(n - 1)");
  LLVMValueRef res_iffalse = LLVMBuildMul(builder, n, call_fac, "n * fac(n - 1)");
  LLVMBuildBr(builder, end);

  LLVMPositionBuilderAtEnd(builder, end);
  LLVMValueRef res = LLVMBuildPhi(builder, LLVMInt32Type(), "result");
  LLVMValueRef phi_vals[] = {res_iftrue, res_iffalse};
  LLVMBasicBlockRef phi_blocks[] = {iftrue, iffalse};
  LLVMAddIncoming(res, phi_vals, phi_blocks, 2);
  LLVMBuildRet(builder, res);
*/

  LLVMTypeRef args[] = { };
  LLVMValueRef fn = LLVMAddFunction(mod, "hello", LLVMFunctionType(LLVMVoidType(), fac_args, 0, 0));
  LLVMSetFunctionCallConv(fn, LLVMCCallConv);
  LLVMValueRef call_args[] = {};
  LLVMBuildCall(builder, fn, call_args, 0, "");
  LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));


  LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);
  LLVMDisposeMessage(error); // Handler == LLVMAbortProcessAction -> No need to check errors


  LLVMExecutionEngineRef engine;
  LLVMModuleProviderRef provider = LLVMCreateModuleProviderForExistingModule(mod);
  error = NULL;
  LLVMCreateJITCompiler(&engine, provider, 0, &error);
  if(error) {
    fprintf(stderr, "%s\n", error);
    LLVMDisposeMessage(error);
    abort();
  }

  LLVMPassManagerRef pass = LLVMCreatePassManager();
  LLVMAddTargetData(LLVMGetExecutionEngineTargetData(engine), pass);
  LLVMAddConstantPropagationPass(pass);
  LLVMAddInstructionCombiningPass(pass);
  LLVMAddPromoteMemoryToRegisterPass(pass);
  // LLVMAddDemoteMemoryToRegisterPass(pass); // Demotes every possible value to memory
  LLVMAddGVNPass(pass);
  LLVMAddCFGSimplificationPass(pass);
  LLVMRunPassManager(pass, mod);
  LLVMDumpModule(mod);

  LLVMGenericValueRef exec_args[] = {LLVMCreateGenericValueOfInt(LLVMInt32Type(), 10, 0)};
  LLVMGenericValueRef exec_res = LLVMRunFunction(engine, fac, 1, exec_args);
  fprintf(stderr, "\n");
  fprintf(stderr, "; Running fac(10) with JIT...\n");
  fprintf(stderr, "; Result: %d\n", LLVMGenericValueToInt(exec_res, 0));

  LLVMDisposePassManager(pass);
  LLVMDisposeBuilder(builder);
  LLVMDisposeExecutionEngine(engine);
  return 0;
}

