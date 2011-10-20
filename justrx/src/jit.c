// Headers required by LLVM
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>


// General stuff
#include <stdlib.h>
#include <stdio.h>

//
#include "ccl.h"
#include "dfa.h"

// match state
#include "jrx.h"


/** Compiler meta */

typedef struct jit_compile_info jit_compile_info;
struct jit_compile_info
{
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    jrx_dfa *dfa;
};


/** LLVM Types */

#define JIT_BOOL_LLVM_TYPE      LLVMInt1Type()
#define JRX_ACCEPT_ID_LLVM_TYPE LLVMInt16Type()
#define JRX_OFFSET_LLVM_TYPE    LLVMInt32Type()
#define JRX_CHAR_LLVM_TYPE      LLVMInt32Type()
#define JRX_ASSERTION_LLVM_TYPE LLVMInt16Type()
#define JRX_STATE_ID_LLVM_TYPE  LLVMInt32Type()

#define JIT_NEG_ONE LLVMConstInt(LLVMInt32Type(), -1, 1)
#define JIT_ZERO    LLVMConstInt(LLVMInt32Type(), 0, 0)
#define JIT_ONE     LLVMConstInt(LLVMInt32Type(), 1, 0)

#define JIT_TRUE    LLVMConstInt(JIT_BOOL_LLVM_TYPE, 1, 0)
#define JIT_FALSE   LLVMConstInt(JIT_BOOL_LLVM_TYPE, 0, 0)

#define JIT_DFA_FN_LLVM_TYPE JITDFAStateFnType()


LLVMTypeRef JITDFAStateFnType()
{
    // Prototype jrx_accept_id stateX (char* string, jrx_offset length, jrx_char prev_cp, jrx_accept_id last_accept_id, jrx_offset last_acc_offset, jrx_assertion first, jrx_assertion last, opaque *match_state)
    LLVMTypeRef arg_types[] = {
            LLVMArrayType(LLVMInt8Type(), 0),
            JRX_OFFSET_LLVM_TYPE,
            JRX_CHAR_LLVM_TYPE,
            JRX_ACCEPT_ID_LLVM_TYPE,
            JRX_OFFSET_LLVM_TYPE,
            JRX_ASSERTION_LLVM_TYPE,
            JRX_ASSERTION_LLVM_TYPE,
            LLVMPointerType(LLVMOpaqueType(), 0)
        };
    return LLVMFunctionType(JRX_ACCEPT_ID_LLVM_TYPE, arg_types, 6, 0);
}

LLVMTypeRef JITSaveStateFnType()
{
    // Prototype void save_state (opaque *ms, jrx_accept_id accept_id, jrx_offset acc_offset, jrx_char prev_cp, jrx_dfa_state_id state_id, JIT_DFA_FN_LLVM_TYPE state_fn)
    LLVMTypeRef arg_types[] = {
        LLVMPointerType(LLVMOpaqueType(), 0),
        JRX_ACCEPT_ID_LLVM_TYPE,
        JRX_OFFSET_LLVM_TYPE,
        JRX_CHAR_LLVM_TYPE,
        JRX_STATE_ID_LLVM_TYPE,
        JIT_DFA_FN_LLVM_TYPE
    };
    return LLVMFunctionType(LLVMVoidType(), arg_types, 5, 0);
}

/* Function references */

/** Get reference to ccl match function, creating the function if necessary */
LLVMValueRef jit_ccl_fn_ref(jit_compile_info *ci, jrx_ccl_id id)
{
    char *name;
    asprintf(&name, "ccl_%u", id);

    LLVMValueRef func = LLVMGetNamedFunction(ci->module, name);
    if (!func) {
        // Prototype: bool cclX (jrx_char cp, jrx_assertion assertions)
        LLVMTypeRef arg_types[] = { JRX_CHAR_LLVM_TYPE, JRX_ASSERTION_LLVM_TYPE };
        LLVMValueRef func = LLVMAddFunction(ci->module, name,
                LLVMFunctionType(JIT_BOOL_LLVM_TYPE, arg_types, 2, 0));
        LLVMSetFunctionCallConv(func, LLVMFastCallConv);
    }

    free(name);
    return func;
}

/** Get reference to dfa state function, creating the function if necessary */
LLVMValueRef jit_dfa_state_fn_ref(jit_compile_info *ci, jrx_dfa_state_id id)
{
    char *name;
    asprintf(&name, "state_%u", id);

    LLVMValueRef func = LLVMGetNamedFunction(ci->module, name);
    if (!func) {
        LLVMValueRef func = LLVMAddFunction(ci->module, name, JIT_DFA_FN_LLVM_TYPE);
        LLVMSetFunctionCallConv(func, LLVMFastCallConv);
    }

    free(name);
    return func;
}

LLVMValueRef jit_dfa_state_jammed_fn_ref(jit_compile_info *ci)
{
    char *name = "_jit_ext_state_jammed";
    LLVMValueRef func = LLVMGetNamedFunction(ci->module, name);
    if (!func) {
        LLVMValueRef func = LLVMAddFunction(ci->module, name, JIT_DFA_FN_LLVM_TYPE);
        LLVMSetFunctionCallConv(func, LLVMCCallConv);
    }
    return func;
}

LLVMValueRef jit_local_word_boundary_fn_ref(jit_compile_info *ci)
{
    char *name = "_jit_ext_local_word_boundary";
    LLVMValueRef func = LLVMGetNamedFunction(ci->module, name);
    if (!func) {
        LLVMTypeRef arg_types[] = { JRX_CHAR_LLVM_TYPE, JRX_CHAR_LLVM_TYPE };
        LLVMValueRef func = LLVMAddFunction(ci->module, name,
                LLVMFunctionType(JIT_BOOL_LLVM_TYPE, arg_types, 2, 0));
        LLVMSetFunctionCallConv(func, LLVMCCallConv);
    }
    return func;
}

LLVMValueRef jit_save_match_state_fn_ref(jit_compile_info *ci)
{
    char *name = "_jit_ext_save_match_state";
    LLVMValueRef func = LLVMGetNamedFunction(ci->module, name);
    if (!func) {
        LLVMValueRef func = LLVMAddFunction(ci->module, name, JITSaveStateFnType());
        LLVMSetFunctionCallConv(func, LLVMCCallConv);
    }
    return func;
}

/** Code generation */

/* ccl match  */
LLVMValueRef _jit_codegen_ccl_fn (jit_compile_info *ci, jrx_ccl_id id)
{
    jrx_ccl *ccl = vec_ccl_get(ci->dfa->ccls->ccls, id);
    assert(ccl);

    LLVMValueRef fn = jit_ccl_fn_ref(ci, id);
    assert(LLVMCountBasicBlocks(fn) == 0);

    LLVMBuilderRef builder = ci->builder;

    LLVMValueRef cp = LLVMGetParam(fn, 0);
    LLVMValueRef have_assertions = LLVMGetParam(fn, 1);

    LLVMBasicBlockRef check_assertions_block = LLVMAppendBasicBlock(fn, "check assertions");
    LLVMBasicBlockRef failed_block = LLVMAppendBasicBlock(fn, "failed");
    LLVMBasicBlockRef success_block = LLVMAppendBasicBlock(fn, "success");
    LLVMBasicBlockRef next_block = LLVMAppendBasicBlock(fn, "");

    // Only generate code to check assertions if necessary [see if this is optimized out]
    LLVMPositionBuilderAtEnd(builder, check_assertions_block);
    LLVMValueRef want_assertions = LLVMConstInt(JRX_ASSERTION_LLVM_TYPE,
            ccl->assertions, 0);
    LLVMValueRef applicable_assertions = LLVMBuildAnd(builder,
            want_assertions, have_assertions, "want & have");
    LLVMValueRef assertions_match = LLVMBuildICmp(builder,
            LLVMIntEQ, want_assertions, applicable_assertions, "applicable == wanted");
    LLVMBuildCondBr(builder, assertions_match, next_block, failed_block);

    // Check to see if code point is within any of our character ranges
    set_for_each(char_range, ccl->ranges, r) {
        LLVMPositionBuilderAtEnd(builder, next_block);

        LLVMValueRef inside_lower_bound = LLVMBuildICmp(builder,
                LLVMIntUGE, cp, LLVMConstInt(LLVMInt32Type(), r.begin, 0), "");
        LLVMValueRef inside_upper_bound = LLVMBuildICmp(builder,
                LLVMIntULT, cp, LLVMConstInt(LLVMInt32Type(), r.end, 0), "");
        LLVMValueRef inside_range = LLVMBuildAnd(builder,
                inside_lower_bound, inside_upper_bound, "");
        LLVMBuildCondBr(builder, inside_range, success_block, next_block);

        next_block = LLVMAppendBasicBlock(fn, "");
    }
    LLVMPositionBuilderAtEnd(builder, next_block);
    LLVMBuildBr(builder, failed_block);

    // Return values
    LLVMPositionBuilderAtEnd(builder, failed_block);
    LLVMBuildRet(builder, JIT_FALSE);

    LLVMPositionBuilderAtEnd(builder, success_block);
    LLVMBuildRet(builder, JIT_TRUE);

    LLVMVerifyFunction(fn, LLVMAbortProcessAction);
    return fn;
}


/* dfa state */

jrx_assertion necessary_assertions(jrx_dfa_state* state, jrx_ccl_group* with_ccls)
{
    jrx_assertion assertions = JRX_ASSERTION_NONE;
    vec_for_each(dfa_transition, state->trans, trans) {
            jrx_ccl* ccl = vec_ccl_get(with_ccls->ccls, trans.ccl);
            assertions |= ccl->assertions;
    }
    return assertions;
}


LLVMValueRef _jit_codegen_dfa_state_fn(jit_compile_info *ci, jrx_dfa_state_id id)
{
    jrx_dfa_state* state = vec_dfa_state_get(ci->dfa->states, id);
    assert(state);

    LLVMValueRef fn = jit_dfa_state_fn_ref(ci, id);
    assert(LLVMCountBasicBlocks(fn) == 0);

    LLVMBuilderRef builder = ci->builder;

    // Parameters
    LLVMValueRef str = LLVMGetParam(fn, 0);
    LLVMValueRef len = LLVMGetParam(fn, 1);
    LLVMValueRef prev_cp = LLVMGetParam(fn, 3);
    LLVMValueRef last_acc_id = LLVMGetParam(fn, 4);
    LLVMValueRef last_acc_offset = LLVMGetParam(fn, 5);
    LLVMValueRef assert_first = LLVMGetParam(fn, 6);
    LLVMValueRef assert_last = LLVMGetParam(fn, 7);
    LLVMValueRef match_state = LLVMGetParam(fn, 8);

    // Make entry
    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(fn, "entry");
    LLVMPositionBuilderAtEnd(builder, entry_block);
    LLVMValueRef cp = LLVMBuildLoad(builder, str, "cp");

    // If this is a Accept state, set last_accept to my aid
    if (state->accepts) {
        jrx_accept_id aid = vec_dfa_accept_get(state->accepts, 0).aid;
        last_acc_id = LLVMConstInt(JRX_ACCEPT_ID_LLVM_TYPE, aid, 0);
        last_acc_offset = len;
    }

    // If len==0:
    // set this fcn as the cur state
    // set prev to last_cp
    // return last_accept. this might be -1 to indicate partial match
    {
        LLVMBasicBlockRef save_state_block = LLVMAppendBasicBlock(fn, "save_state");
        LLVMBasicBlockRef assertions_block = LLVMAppendBasicBlock(fn, "assertions");

        LLVMValueRef if_past_end = LLVMBuildICmp(builder, LLVMIntEQ, len, JIT_ZERO, "len==0");
        LLVMBuildCondBr(builder, if_past_end, save_state_block, assertions_block);

        LLVMPositionBuilderAtEnd(builder, save_state_block);
        LLVMValueRef save_state_fn_args[] = { match_state,
                                              last_acc_id,
                                              last_acc_offset,
                                              prev_cp,
                                              LLVMConstInt(JRX_STATE_ID_LLVM_TYPE, id, 0),
                                              fn };
        LLVMBuildCall(builder, jit_save_match_state_fn_ref(ci), save_state_fn_args, 6 , "");
        LLVMBuildRet(builder, last_acc_id);

        LLVMPositionBuilderAtEnd(builder, assertions_block);
    }

    // If any transition ccl requires an assertion,
    // determine what assertions apply here, otherwise don't bother
    LLVMValueRef current_assertions = LLVMConstInt(JRX_ASSERTION_LLVM_TYPE, JRX_ASSERTION_NONE, 0);
    {
        jrx_assertion requested_assertions = necessary_assertions(state, ci->dfa->ccls);

        if ((requested_assertions & JRX_ASSERTION_BOD) || (requested_assertions & JRX_ASSERTION_BOL)) {
            // assert_first will only be valid for the call to the first state,
            // afterwards it is cleared
            current_assertions = LLVMBuildOr(builder, current_assertions, assert_first, "");
        }

        if ((requested_assertions & JRX_ASSERTION_EOD) || (requested_assertions & JRX_ASSERTION_EOL)) {
            current_assertions = LLVMBuildSelect(builder,
                    LLVMBuildICmp(builder, LLVMIntEQ, len, JIT_ONE, "len==1"),
                    LLVMBuildOr(builder, current_assertions, assert_last, ""),
                    current_assertions,
                    "");
        }

        if ((requested_assertions & JRX_ASSERTION_WORD_BOUNDARY) ||
            (requested_assertions & JRX_ASSERTION_NOT_WORD_BOUNDARY)) {

            LLVMValueRef local_word_boundary_fn_args[] = { prev_cp, cp };
            LLVMValueRef on_word_boundary = LLVMBuildCall(builder, jit_local_word_boundary_fn_ref(ci), local_word_boundary_fn_args, 2, "on_local_word_boundary");

            if (requested_assertions & JRX_ASSERTION_WORD_BOUNDARY) {
                current_assertions = LLVMBuildSelect(builder,
                        on_word_boundary,
                        LLVMBuildOr(builder, current_assertions,
                                LLVMConstInt(JRX_ASSERTION_LLVM_TYPE, JRX_ASSERTION_WORD_BOUNDARY, 0), ""),
                        current_assertions,
                        "");
            }

            if (requested_assertions & JRX_ASSERTION_NOT_WORD_BOUNDARY) {
                current_assertions = LLVMBuildSelect(builder,
                        on_word_boundary,
                        current_assertions,
                        LLVMBuildOr(builder, current_assertions,
                                LLVMConstInt(JRX_ASSERTION_LLVM_TYPE, JRX_ASSERTION_NOT_WORD_BOUNDARY, 0), ""),
                        "");
            }
        }
    }

    // Increment offset for next call
    {
        LLVMValueRef idx[] = { JIT_ONE };
        str = LLVMBuildGEP(builder, str, idx, 1, "str++");
        len = LLVMBuildSub(builder, len, JIT_ONE, "len--");
    }

    // For each transition
    // Call jited match_cclX
    //   if true, return jited state_successor() -- this should be a tail call
    {
        //LLVMBasicBlockRef next_block = LLVMAppendBasicBlock(fn, "check_ccl");
        vec_for_each(dfa_transition, state->trans, trans) {
            LLVMBasicBlockRef transition_block = LLVMAppendBasicBlock(fn, "transition");
            LLVMBasicBlockRef next_block = LLVMAppendBasicBlock(fn, "check_ccl");

            LLVMValueRef ccl_fn_args[] = { cp, current_assertions };
            LLVMValueRef ccl_match = LLVMBuildCall(builder, jit_ccl_fn_ref(ci, trans.ccl), ccl_fn_args, 2, "");
            LLVMBuildCondBr(builder, ccl_match, transition_block, next_block);

            LLVMPositionBuilderAtEnd(builder, transition_block);
            LLVMValueRef trans_fn_args[] = { str, len, cp, last_acc_id, last_acc_offset, assert_first, assert_last, match_state };
            LLVMValueRef trans_result = LLVMBuildCall(builder, jit_dfa_state_fn_ref(ci, trans.succ), trans_fn_args, 6, "");
            LLVMSetTailCall(trans_result, 1); // do i need to use LLVMGetLastInstruction?
            LLVMBuildRet(builder, trans_result);

            LLVMPositionBuilderAtEnd(builder, next_block);
        }
    }

    // no further transition possible,
    // set cur state to JAMMED
    // return last_accept id OR, if last_accept is -1 , return 0
    //  to indicate failure (since we can't transition past this point)  [ use SELECT instruction ]
    {
        last_acc_id = LLVMBuildSelect(builder,
                LLVMBuildICmp(builder, LLVMIntEQ, last_acc_id, JIT_NEG_ONE, ""),
                JIT_ZERO,
                last_acc_id,
                "");

        LLVMValueRef save_state_fn_args[] = { match_state,
                                              last_acc_id,
                                              last_acc_offset,
                                              prev_cp,
                                              LLVMConstInt(JRX_STATE_ID_LLVM_TYPE, -1, 0),
                                              jit_dfa_state_jammed_fn_ref(ci) };
        LLVMBuildCall(builder, jit_save_match_state_fn_ref(ci), save_state_fn_args, 6 , "");

        LLVMBuildRet(builder, last_acc_id);
    }

    LLVMVerifyFunction(fn, LLVMAbortProcessAction);
    return fn;
}

// don't need to worry abt partial matches or capture group offsets


/** Externally defined functions that are called from JIT code */

extern jrx_accept_id _jit_ext_state_jammed()
{
    // We can no longer match
    return 0;
}


extern LLVMBool _jit_ext_local_word_boundary(jrx_char prev_cp, jrx_char cp)
{
    // TODO
    return 0;
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

/** Compilation */
void COMPILE(jrx_dfa *dfa) {

}
