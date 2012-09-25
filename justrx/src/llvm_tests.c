/*
1) Want to see if we can export a fn ptr directly
2) Want to see if we can pass an opaque struct pointer outside
3) Want to see if we can manipulate things in the struct ptr w/ offsets
4) Want to see if I can add my own binding to directly get fn ptr from ee
*/

// Headers required by LLVM
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>


// General stuff
#include <stdlib.h>
#include <stdio.h>

// offsetof
#include <stddef.h>


extern void hello(int n) {
    printf("Hello world: %d\n",n);
}

void export_fn_ptr ()
{
  char *error = NULL; // Used to retrieve messages from functions

  LLVMLinkInJIT();
  LLVMInitializeNativeTarget();

  LLVMModuleRef mod = LLVMModuleCreateWithName("fn_module");
  LLVMBuilderRef builder = LLVMCreateBuilder();

  LLVMTypeRef fn_arg_types[] = { LLVMInt32Type() };
  LLVMTypeRef fn_type = LLVMFunctionType(LLVMVoidType(), fn_arg_types, 1, 0);

  /* fn1 : call hello */
  LLVMValueRef fn1 = LLVMAddFunction(mod, "fn1", fn_type);
  LLVMSetFunctionCallConv(fn1, LLVMCCallConv);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(fn1, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);
  LLVMValueRef param = LLVMGetParam(fn1, 0);
  LLVMValueRef hello = LLVMAddFunction(mod, "hello", fn_type);
  LLVMSetFunctionCallConv(hello, LLVMCCallConv);
  LLVMValueRef args_hello[] = { param };
  LLVMBuildCall(builder, hello, args_hello, 1, "");
  LLVMBuildRetVoid(builder);

  /* fn2 : return ptr to fn1 */
  LLVMValueRef fn2 = LLVMAddFunction(mod, "fn2", LLVMFunctionType(LLVMPointerType(fn_type, 0), NULL, 0, 0));
  LLVMSetFunctionCallConv(fn2, LLVMCCallConv);

  entry = LLVMAppendBasicBlock(fn2, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);
  LLVMBuildRet(builder, fn1);

  /* compile */
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

  /* View compiled result */
  LLVMDumpModule(mod);

  LLVMGenericValueRef exec_args[] = {LLVMCreateGenericValueOfInt(LLVMInt32Type(), 10, 0)};
  fprintf(stderr, "\n");
  fprintf(stderr, "; Running fn1 through JIT\n");
  LLVMGenericValueRef exec_res = LLVMRunFunction(engine, fn1, 1, exec_args);

  fprintf(stderr, "; Running fn2 through JIT to get fn address\n");
  exec_res = LLVMRunFunction(engine, fn2, 0, NULL);
  void (*fnptr)(int) = LLVMGenericValueToPointer(exec_res);

  fprintf(stderr, "; Calling function pointer...\n");
  fnptr(20);

  LLVMDisposePassManager(pass);
  LLVMDisposeBuilder(builder);
  LLVMDisposeExecutionEngine(engine);
  return;
}

/***********************************************************/

void export_fn_ptr_directly ()
{
  char *error = NULL; // Used to retrieve messages from functions

  LLVMLinkInJIT();
  LLVMInitializeNativeTarget();

  LLVMModuleRef mod = LLVMModuleCreateWithName("fn_module");
  LLVMBuilderRef builder = LLVMCreateBuilder();

  LLVMTypeRef fn_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);

  /* fn1 : return 100 */
  LLVMValueRef fn1 = LLVMAddFunction(mod, "fn1", fn_type);
  LLVMSetFunctionCallConv(fn1, LLVMCCallConv);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(fn1, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);
  LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 100, 0));

  /* compile */
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

  /* View compiled result */
  LLVMDumpModule(mod);


  fprintf(stderr, "; LLVMGetPointerToGlobal(fn)\n");
  int (*fnptr)() = LLVMGetPointerToGlobal(engine, fn1);

  fprintf(stderr, "; Calling function pointer: %d\n", fnptr());

  LLVMDisposePassManager(pass);
  LLVMDisposeBuilder(builder);
  LLVMDisposeExecutionEngine(engine);
  return;
}

/***********************************************************/

typedef struct opaque {
    char c;
    int n;
    void (*fn)();
} opaque_t;

void disp_opaque(opaque_t* o) {
    printf("opaque->n = %d\n", o->n);
}

void opaque_ptr () {
  char *error = NULL; // Used to retrieve messages from functions

  LLVMLinkInJIT();
  LLVMInitializeNativeTarget();

  LLVMModuleRef mod = LLVMModuleCreateWithName("fn_module");
  LLVMBuilderRef builder = LLVMCreateBuilder();

  LLVMTypeRef fn_arg_types[] = { LLVMPointerType(LLVMStructType(NULL, 0, 0), 0) };
  LLVMTypeRef fn_type = LLVMFunctionType(LLVMVoidType(), fn_arg_types, 1, 0);

  /* fn1 : pass opaque value */
  LLVMValueRef fn1 = LLVMAddFunction(mod, "fn1", fn_type);
  LLVMSetFunctionCallConv(fn1, LLVMCCallConv);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(fn1, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);
  LLVMValueRef param = LLVMGetParam(fn1, 0);

  LLVMValueRef ext = LLVMAddFunction(mod, "disp_opaque", fn_type);
  LLVMSetFunctionCallConv(ext, LLVMCCallConv);
  LLVMValueRef ext_args[] = { param };

  LLVMBuildCall(builder, ext, ext_args, 1, "");
  LLVMBuildRetVoid(builder);

  /* compile */
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

  /* View compiled result */
  LLVMDumpModule(mod);

  opaque_t o = {'a', 100, NULL};
  LLVMGenericValueRef exec_args[] = {LLVMCreateGenericValueOfPointer(&o)};
  fprintf(stderr, "\n");
  fprintf(stderr, "; Passing &o through JIT\n");
  LLVMRunFunction(engine, fn1, 1, exec_args);

  LLVMDisposePassManager(pass);
  LLVMDisposeBuilder(builder);
  LLVMDisposeExecutionEngine(engine);
  return;
}


/*****************************************************/

void disp_ptr(void *ptr) {
    printf("ptr=%p\n",ptr);
}

void modify_opaque () {
  char *error = NULL; // Used to retrieve messages from functions

  LLVMLinkInJIT();
  LLVMInitializeNativeTarget();

  LLVMModuleRef mod = LLVMModuleCreateWithName("fn_module");
  LLVMBuilderRef builder = LLVMCreateBuilder();

  LLVMTypeRef fn_arg_types[] = { LLVMPointerType(LLVMInt32Type(), 0) };
  LLVMTypeRef fn_type = LLVMFunctionType(LLVMVoidType(), fn_arg_types, 1, 0);

  /* fn1 : modify and pass opaque value */
  LLVMValueRef fn1 = LLVMAddFunction(mod, "fn1", fn_type);
  LLVMSetFunctionCallConv(fn1, LLVMCCallConv);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(fn1, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);
  LLVMValueRef param = LLVMGetParam(fn1, 0);

  LLVMValueRef as_int = LLVMBuildPtrToInt(builder, param, LLVMInt64Type(), "as_int");
  LLVMValueRef offset = LLVMBuildAdd(builder, as_int, LLVMConstInt(LLVMInt64Type(), offsetof(opaque_t, n), 0), "offset_n");
  LLVMValueRef as_ptr = LLVMBuildIntToPtr(builder, offset, LLVMPointerType(LLVMInt32Type(), 0), "as_ptr");

  LLVMValueRef ext = LLVMAddFunction(mod, "disp_ptr", fn_type);
  LLVMSetFunctionCallConv(ext, LLVMCCallConv);
  LLVMValueRef ext_args[] = { param };
  
  LLVMBuildCall(builder, ext, ext_args, 1, "");
  ext_args[0] = as_ptr;
  LLVMBuildCall(builder, ext, ext_args, 1, "");
  
  LLVMBuildRetVoid(builder);
  
  /* compile */
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

  /* View compiled result */
  LLVMDumpModule(mod);

  opaque_t o = {'a', 100, NULL};
  LLVMGenericValueRef exec_args[] = {LLVMCreateGenericValueOfPointer(&o)};
  fprintf(stderr, "\n");
  fprintf(stderr, "; Passing &o through JIT\n");
  LLVMRunFunction(engine, fn1, 1, exec_args);

  LLVMDisposePassManager(pass);
  LLVMDisposeBuilder(builder);
  LLVMDisposeExecutionEngine(engine);
  return;
}
