// Headers required by LLVM
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>

// General stuff
#include <stdlib.h>
#include <stdio.h>

// State Machine
#include "ccl.h"
#include "dfa.h"

// Match State
#include "jrx.h"

// _isword
#include "jlocale.h"


/** Compiler meta */
typedef struct jrx_jit jrx_jit;
struct jrx_jit
{
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMPassManagerRef pass_mgr;
    LLVMExecutionEngineRef exec_engine;
    jrx_dfa *dfa;
};


/** LLVM Types */
#define JIT_BOOL_LLVM_TYPE      LLVMInt1Type()
#define JRX_ACCEPT_ID_LLVM_TYPE LLVMInt16Type()
#define JRX_OFFSET_LLVM_TYPE    LLVMInt32Type()
#define JRX_CHAR_LLVM_TYPE      LLVMInt32Type()
#define JRX_ASSERTION_LLVM_TYPE LLVMInt16Type()
#define JRX_STATE_ID_LLVM_TYPE  LLVMInt32Type()

#define JIT_MATCH_STATE_STRUCT_LLVM_TYPE JITMatchStateStructType()
LLVMTypeRef JITMatchStateStructType()
{
    // Turns out that opaque types are not equivelant???
    static LLVMTypeRef type = NULL;
    if (!type)
        type = LLVMStructType(NULL, 0, 0);
    return type;
}

#define JIT_STATE_FN_LLVM_TYPE JITDFAStateFnType()
LLVMTypeRef JITDFAStateFnType()
{
    /*
    Prototype:
    jrx_accept_id stateX (char* string, jrx_offset length, jrx_char prev_cp,
                          jrx_accept_id last_accept_id, jrx_offset last_accept_offset,
                          jrx_assertion first, jrx_assertion last,
                          opaque *match_state)
    */
    LLVMTypeRef arg_types[] = {
            LLVMPointerType(LLVMInt8Type(), 0),
            JRX_OFFSET_LLVM_TYPE,
            JRX_CHAR_LLVM_TYPE,
            JRX_ACCEPT_ID_LLVM_TYPE,
            JRX_OFFSET_LLVM_TYPE,
            JRX_ASSERTION_LLVM_TYPE,
            JRX_ASSERTION_LLVM_TYPE,
            LLVMPointerType(JIT_MATCH_STATE_STRUCT_LLVM_TYPE, 0)
        };
    return LLVMFunctionType(JRX_ACCEPT_ID_LLVM_TYPE, arg_types, 8, 0);
}

#define JIT_SAVE_FN_LLVM_TYPE JITSaveStateFnType()
LLVMTypeRef JITSaveStateFnType()
{
    /* Prototype:
       void save_state (opaque *ms,
                        jrx_accept_id accept_id, jrx_offset accept_offset, jrx_char prev_cp,
                        jrx_dfa_state_id state_id, JIT_STATE_FN_TYPE state_fn)
    */
    LLVMTypeRef arg_types[] = {
        LLVMPointerType(JIT_MATCH_STATE_STRUCT_LLVM_TYPE, 0),
        JRX_ACCEPT_ID_LLVM_TYPE,
        JRX_OFFSET_LLVM_TYPE,
        JRX_CHAR_LLVM_TYPE,
        JRX_STATE_ID_LLVM_TYPE,
        LLVMPointerType(JIT_STATE_FN_LLVM_TYPE, 0)
    };
    return LLVMFunctionType(LLVMVoidType(), arg_types, 6, 0);
}

/** LLVM Constants */
#define JIT_NEG_ONE LLVMConstInt(LLVMInt32Type(), -1, 1)
#define JIT_ZERO    LLVMConstInt(LLVMInt32Type(), 0, 0)
#define JIT_ONE     LLVMConstInt(LLVMInt32Type(), 1, 0)

#define JIT_TRUE    LLVMConstInt(JIT_BOOL_LLVM_TYPE, 1, 0)
#define JIT_FALSE   LLVMConstInt(JIT_BOOL_LLVM_TYPE, 0, 0)

/** JRX->LLVM Type Conversions */
#define JIT_CHAR(c)      LLVMConstInt(JRX_CHAR_LLVM_TYPE, c, 0)
#define JIT_ASSERTION(a) LLVMConstInt(JRX_ASSERTION_LLVM_TYPE, a, 0)
#define JIT_ACCEPT_ID(i) LLVMConstInt(JRX_ACCEPT_ID_LLVM_TYPE, i, 0)
#define JIT_STATE_ID(s)  LLVMConstInt(JRX_STATE_ID_LLVM_TYPE, s, 0)

/** Helper Macros */
#define JIT_ENTER_BLOCK(builder, function, block_name)                  \
    do {                                                                \
        LLVMBasicBlockRef _block =                                      \
            LLVMAppendBasicBlock(function, block_name);                 \
        LLVMBuildBr(builder, _block);                                   \
        LLVMPositionBuilderAtEnd(builder, _block);                      \
    } while(0)



/* Function references */

/** Get reference to ccl match function, creating the function if necessary */
LLVMValueRef jit_ccl_fn_ref(jrx_jit *jit, jrx_ccl_id id)
{
    char *name;
    asprintf(&name, "ccl_%u", id);

    LLVMValueRef func = LLVMGetNamedFunction(jit->module, name);
    if (!func) {
        /* Prototype:
           bool cclX (jrx_char cp, jrx_assertion assertions)
        */
        LLVMTypeRef arg_types[] = { JRX_CHAR_LLVM_TYPE, JRX_ASSERTION_LLVM_TYPE };
        func = LLVMAddFunction(jit->module, name,
                               LLVMFunctionType(JIT_BOOL_LLVM_TYPE, arg_types, 2, 0));
        LLVMSetFunctionCallConv(func, LLVMFastCallConv);
    }

    free(name);
    return func;
}

/** Get reference to dfa state function, creating the function if necessary */
LLVMValueRef jit_dfa_state_fn_ref(jrx_jit *jit, jrx_dfa_state_id id)
{
    char *name;
    asprintf(&name, "state_%u", id);

    LLVMValueRef func = LLVMGetNamedFunction(jit->module, name);
    if (!func) {
        func = LLVMAddFunction(jit->module, name, JIT_STATE_FN_LLVM_TYPE);
        LLVMSetFunctionCallConv(func, LLVMFastCallConv);
    }

    free(name);
    return func;
}

LLVMValueRef jit_dfa_state_jammed_fn_ref(jrx_jit *jit)
{
    char *name = "_jit_ext_state_jammed";
    LLVMValueRef func = LLVMGetNamedFunction(jit->module, name);
    if (!func) {
        func = LLVMAddFunction(jit->module, name, JIT_STATE_FN_LLVM_TYPE);
        LLVMSetFunctionCallConv(func, LLVMCCallConv);
    }
    return func;
}

LLVMValueRef jit_local_word_boundary_fn_ref(jrx_jit *jit)
{
    char *name = "_jit_ext_local_word_boundary";
    LLVMValueRef func = LLVMGetNamedFunction(jit->module, name);
    if (!func) {
        LLVMTypeRef arg_types[] = { JRX_CHAR_LLVM_TYPE, JRX_CHAR_LLVM_TYPE };
        func = LLVMAddFunction(jit->module, name,
                LLVMFunctionType(JIT_BOOL_LLVM_TYPE, arg_types, 2, 0));
        LLVMSetFunctionCallConv(func, LLVMCCallConv);
    }
    return func;
}

LLVMValueRef jit_save_match_state_fn_ref(jrx_jit *jit)
{
    char *name = "_jit_ext_save_match_state";
    LLVMValueRef func = LLVMGetNamedFunction(jit->module, name);
    if (!func) {
        func = LLVMAddFunction(jit->module, name, JIT_SAVE_FN_LLVM_TYPE);
        LLVMSetFunctionCallConv(func, LLVMCCallConv);
    }
    return func;
}


/* Code generation */

/* CCL Match  */

LLVMValueRef _jit_codegen_ccl_fn (jrx_jit *jit, jrx_ccl_id id)
{
    jrx_ccl *ccl = vec_ccl_get(jit->dfa->ccls->ccls, id);
    assert(ccl);
    LLVMValueRef fn = jit_ccl_fn_ref(jit, id);
    assert(LLVMCountBasicBlocks(fn) == 0);

    LLVMBuilderRef b = jit->builder;

    LLVMValueRef cp = LLVMGetParam(fn, 0);
    LLVMValueRef have_assertions = LLVMGetParam(fn, 1);

    LLVMBasicBlockRef check_assertions_block = LLVMAppendBasicBlock(fn, "check assertions");
    LLVMBasicBlockRef failed_block = LLVMAppendBasicBlock(fn, "failed");
    LLVMBasicBlockRef success_block = LLVMAppendBasicBlock(fn, "success");
    LLVMBasicBlockRef next_block = LLVMAppendBasicBlock(fn, "range check");

    // Only generate code to check assertions if necessary [see if this is optimized out]
    LLVMPositionBuilderAtEnd(b, check_assertions_block);
    LLVMValueRef want_assertions = JIT_ASSERTION(ccl->assertions);
    LLVMValueRef applicable_assertions =
        LLVMBuildAnd(b, want_assertions, have_assertions, "want & have");
    LLVMValueRef assertions_match =
        LLVMBuildICmp(b, LLVMIntEQ, want_assertions, applicable_assertions, "applicable == wanted");

    LLVMBuildCondBr(b, assertions_match, next_block, failed_block);

    // Check to see if code point is within any of our character ranges
    set_for_each(char_range, ccl->ranges, r) {
        LLVMPositionBuilderAtEnd(b, next_block);

        LLVMValueRef inside_lower_bound =
            LLVMBuildICmp(b, LLVMIntUGE, cp, JIT_CHAR(r.begin), "");
        LLVMValueRef inside_upper_bound =
            LLVMBuildICmp(b, LLVMIntULT, cp, JIT_CHAR(r.end), "");
        LLVMValueRef inside_range =
            LLVMBuildAnd(b, inside_lower_bound, inside_upper_bound, "");

        next_block = LLVMAppendBasicBlock(fn, "range check");
        LLVMBuildCondBr(b, inside_range, success_block, next_block);
    }
    LLVMPositionBuilderAtEnd(b, next_block);
    LLVMBuildBr(b, failed_block);

    // Return values
    LLVMPositionBuilderAtEnd(b, failed_block);
    LLVMBuildRet(b, JIT_FALSE);

    LLVMPositionBuilderAtEnd(b, success_block);
    LLVMBuildRet(b, JIT_TRUE);

    LLVMVerifyFunction(fn, LLVMAbortProcessAction);
    LLVMRunFunctionPassManager(jit->pass_mgr, fn);
    return fn;
}


/* DFA State */

/** Collect all possible assertions a state's CLLs require */
jrx_assertion necessary_assertions(jrx_dfa_state* state, jrx_ccl_group* with_ccls)
{
    jrx_assertion assertions = JRX_ASSERTION_NONE;
    vec_for_each(dfa_transition, state->trans, trans) {
            jrx_ccl* ccl = vec_ccl_get(with_ccls->ccls, trans.ccl);
            assertions |= ccl->assertions;
    }
    return assertions;
}

LLVMValueRef _jit_codegen_dfa_state_fn(jrx_jit *jit, jrx_dfa_state_id id)
{
    jrx_dfa_state* state = vec_dfa_state_get(jit->dfa->states, id);
    assert(state);
    LLVMValueRef fn = jit_dfa_state_fn_ref(jit, id);
    assert(LLVMCountBasicBlocks(fn) == 0);

    LLVMBuilderRef b = jit->builder;

    // Parameters
    LLVMValueRef str = LLVMGetParam(fn, 0);
    LLVMValueRef len = LLVMGetParam(fn, 1);
    LLVMValueRef prev_cp = LLVMGetParam(fn, 2);
    LLVMValueRef last_accept_id = LLVMGetParam(fn, 3);
    LLVMValueRef last_accept_offset = LLVMGetParam(fn, 4);
    LLVMValueRef assert_first = LLVMGetParam(fn, 5);
    LLVMValueRef assert_last = LLVMGetParam(fn, 6);
    LLVMValueRef match_state = LLVMGetParam(fn, 7);

    // Make entry
    LLVMPositionBuilderAtEnd(b, LLVMAppendBasicBlock(fn, "entry"));
    LLVMValueRef cp = LLVMBuildZExt(b, LLVMBuildLoad(b, str, ""),
                                       JRX_CHAR_LLVM_TYPE, "cp");

    // If this is an Accept state, set last_accept to my aid
    if (state->accepts) {
        jrx_accept_id aid = vec_dfa_accept_get(state->accepts, 0).aid;
        last_accept_id = JIT_ACCEPT_ID(aid);
        last_accept_offset = len;
    }

    // If len==0:
    // set this fcn as the cur state
    // set prev to last_cp
    // return last_accept. this might be -1 to indicate partial match
    {
        JIT_ENTER_BLOCK(b, fn, "base_case");
        LLVMBasicBlockRef save_state_block = LLVMAppendBasicBlock(fn, "save_state");
        LLVMBasicBlockRef skip_block = LLVMAppendBasicBlock(fn, "");

        LLVMValueRef if_past_end = LLVMBuildICmp(b, LLVMIntEQ, len, JIT_ZERO, "len==0");
        LLVMBuildCondBr(b, if_past_end, save_state_block, skip_block);

        LLVMPositionBuilderAtEnd(b, save_state_block);
        LLVMValueRef save_state_fn_args[] = { match_state,
                                              last_accept_id,
                                              last_accept_offset,
                                              prev_cp,
                                              JIT_STATE_ID(id),
                                              fn };
        LLVMBuildCall(b, jit_save_match_state_fn_ref(jit), save_state_fn_args, 6 , "");
        LLVMBuildRet(b, last_accept_id);

        LLVMPositionBuilderAtEnd(b, skip_block);
    }

    // If any transition ccl requires an assertion,
    // determine what assertions apply here, otherwise don't bother
    LLVMValueRef current_assertions = JIT_ASSERTION(JRX_ASSERTION_NONE);
    {
        JIT_ENTER_BLOCK(b, fn, "assertions");

        jrx_assertion requested_assertions = necessary_assertions(state, jit->dfa->ccls);

        if ((requested_assertions & JRX_ASSERTION_BOD) ||
            (requested_assertions & JRX_ASSERTION_BOL)) {
            // assert_first will only be valid for the call to the first state,
            // afterwards it is cleared
            current_assertions =
                LLVMBuildOr(b, current_assertions, assert_first, "");
        }

        if ((requested_assertions & JRX_ASSERTION_EOD) ||
            (requested_assertions & JRX_ASSERTION_EOL)) {
            current_assertions =
                LLVMBuildSelect(b, LLVMBuildICmp(b, LLVMIntEQ, len, JIT_ONE, "len==1"),
                                LLVMBuildOr(b, current_assertions, assert_last, ""),
                                current_assertions,
                                "");
        }

        LLVMValueRef local_word_boundary_fn_args[] = { prev_cp, cp };
        LLVMValueRef on_word_boundary =
            LLVMBuildCall(b, jit_local_word_boundary_fn_ref(jit),
                          local_word_boundary_fn_args, 2,
                          "on_local_word_boundary");

        if (requested_assertions & JRX_ASSERTION_WORD_BOUNDARY) {
            current_assertions =
                LLVMBuildSelect(b, on_word_boundary,
                                LLVMBuildOr(b, current_assertions,
                                            JIT_ASSERTION(JRX_ASSERTION_WORD_BOUNDARY), ""),
                                current_assertions,
                                "");
        }

        if (requested_assertions & JRX_ASSERTION_NOT_WORD_BOUNDARY) {
            current_assertions =
                LLVMBuildSelect(b, on_word_boundary,
                                current_assertions,
                                LLVMBuildOr(b, current_assertions,
                                            JIT_ASSERTION(JRX_ASSERTION_NOT_WORD_BOUNDARY), ""),
                                "");
        }
    }

    // Increment offset for next call
    {
        JIT_ENTER_BLOCK(b, fn, "increment");

        LLVMValueRef idx[] = { JIT_ONE };
        str = LLVMBuildGEP(b, str, idx, 1, "str++");
        len = LLVMBuildSub(b, len, JIT_ONE, "len--");
    }

    // For each transition
    // Call jited match_cclX
    //   if true, return jited state_successor() -- this should be a tail call
    {
        JIT_ENTER_BLOCK(b, fn, "check_ccl");

        vec_for_each(dfa_transition, state->trans, trans) {
            LLVMBasicBlockRef transition_block = LLVMAppendBasicBlock(fn, "transition");
            LLVMBasicBlockRef next_block = LLVMAppendBasicBlock(fn, "check_ccl");

            LLVMValueRef ccl_fn_args[] = { cp, current_assertions };
            LLVMValueRef ccl_match = LLVMBuildCall(b, jit_ccl_fn_ref(jit, trans.ccl),
                                                   ccl_fn_args, 2, "");
            LLVMBuildCondBr(b, ccl_match, transition_block, next_block);

            LLVMPositionBuilderAtEnd(b, transition_block);
            LLVMValueRef trans_fn_args[] = { str, len, cp,
                                             last_accept_id, last_accept_offset,
                                             JIT_ASSERTION(JRX_ASSERTION_NONE), assert_last,
                                             match_state };
            LLVMValueRef trans_result = LLVMBuildCall(b, jit_dfa_state_fn_ref(jit, trans.succ),
                                                      trans_fn_args, 8, "");
            LLVMSetTailCall(trans_result, 1); // do i need to use LLVMGetLastInstruction?
            LLVMBuildRet(b, trans_result);

            LLVMPositionBuilderAtEnd(b, next_block);
        }
    }

    // At this point, no further transition possible,
    // set cur state to JAMMED
    // return last_accept id OR, if last_accept is -1 , return 0
    //  to indicate failure (since we can't transition past this point)  [ use SELECT instruction ]
    {
        JIT_ENTER_BLOCK(b, fn, "jammed");

        last_accept_id =
            LLVMBuildSelect(b, LLVMBuildICmp(b, LLVMIntEQ, last_accept_id, JIT_ACCEPT_ID(-1), ""),
                            JIT_ACCEPT_ID(0),
                            last_accept_id,
                            "");

        LLVMValueRef save_state_fn_args[] = { match_state,
                                              last_accept_id,
                                              last_accept_offset,
                                              prev_cp,
                                              JIT_STATE_ID(-1),
                                              jit_dfa_state_jammed_fn_ref(jit) };
        LLVMBuildCall(b, jit_save_match_state_fn_ref(jit),
                      save_state_fn_args, 6 , "");

        LLVMBuildRet(b, last_accept_id);
    }

    LLVMVerifyFunction(fn, LLVMAbortProcessAction);
    LLVMRunFunctionPassManager(jit->pass_mgr, fn);
    return fn;
}


/* Externally defined functions that are called from JIT code */

extern jrx_accept_id _jit_ext_state_jammed()
{
    // We can no longer match
    return 0;
}

extern int _jit_ext_local_word_boundary(jrx_char prev_cp, jrx_char cp)
{
    return _isword(cp) ? (prev_cp ? ! _isword(prev_cp) : 1) : 0;
}

extern void _jit_ext_save_match_state(jrx_match_state *ms,
                                      jrx_accept_id accept_id,
                                      jrx_offset accept_offset,
                                      jrx_char prev_cp,
                                      jrx_dfa_state_id state_id,
                                      int (*state_fn)() )
{
    ms->acc       = accept_id;
    ms->offset    = accept_offset;
    ms->previous  = prev_cp;
    ms->state     = state_id;
    //ms->jit_state = state_fn;
}


/* Compilation */

jrx_jit* _jit_init(jrx_dfa *dfa)
{
    char *err;

    jrx_jit *jit = malloc(sizeof(jrx_jit));
    assert(jit && "_jit_init: malloc failed");

    LLVMModuleRef module = LLVMModuleCreateWithName("is this name important?");
    LLVMBuilderRef builder = LLVMCreateBuilder();

    LLVMExecutionEngineRef exec_engine;
    if (LLVMCreateExecutionEngineForModule(&exec_engine, module, &err)) {
        assert(0 && "_jit_init: execution engine creation failed");

        fprintf(stderr, "error: %s\n", err);
        LLVMDisposeMessage(err);
        //jit_free_compile_info(jit);
        return NULL;
    }

    LLVMPassManagerRef pass_mgr = LLVMCreateFunctionPassManagerForModule(module);
    LLVMAddTargetData (LLVMGetExecutionEngineTargetData (exec_engine), pass_mgr);
    /* Optimization passes
    LLVMAddConstantPropagationPass(pass_mgr);
    LLVMAddPromoteMemoryToRegisterPass(pass_mgr);
    LLVMAddInstructionCombiningPass(pass_mgr);
    LLVMAddReassociatePass (pass_mgr);
    LLVMAddGVNPass(pass_mgr);
    LLVMAddCFGSimplificationPass(pass_mgr);
    */
    LLVMInitializeFunctionPassManager (pass_mgr);

    jit->dfa = dfa;
    jit->module = module;
    jit->builder = builder;
    jit->exec_engine = exec_engine;
    jit->pass_mgr = pass_mgr;

    return jit;
}

void jit_delete(jrx_jit *jit)
{
    LLVMDisposePassManager(jit->pass_mgr);
    LLVMDisposeBuilder(jit->builder);
    LLVMDisposeModule(jit->module);
    LLVMDisposeExecutionEngine(jit->exec_engine);
    free(jit);
}

jrx_jit* jit_from_dfa(jrx_dfa *dfa)
{
    LLVMInitializeNativeTarget();
    LLVMLinkInJIT();

    jrx_jit *jit = _jit_init(dfa);
    unsigned id = 0;

    // Codegen all ccls
    // TODO: CCLs should be codegened on creation
    for (id = 0; id < vec_ccl_size(dfa->ccls->ccls); id++) {
        _jit_codegen_ccl_fn(jit, id);
    }

    // Codegen all states
    // TODO: Lazy codegen
    for (id = 0; id < vec_dfa_state_size(dfa->states); id++) {
        _jit_codegen_dfa_state_fn(jit, id);
    }

    // Generate dispatch fn
    //

    // Dump module
    LLVMDumpModule(jit->module);

    return jit;
}


/* Execution */
int jit_regexec_partial_min(const jrx_regex_t *preg, const char *buffer, unsigned int len, jrx_assertion first, jrx_assertion last, jrx_match_state* ms, int find_partial_matches)
{
    // Save orig offset
    //lookup state fn based on ms->state
    //Call w/ necessary params
    // Update offset = orig_offset + (len - faux offset)
    //Return result

    return 0;
}


void jit_regfree(jrx_regex_t *preg)
{

}
