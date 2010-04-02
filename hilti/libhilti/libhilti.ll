; $Id$
;
; Internal LLVM code accessible only to the code generator.

;;; Type definitions.

%__hlt_void = type i8
%__hlt_cchar = type i8
%__hlt_eoss = type i8
%__hlt_func = type void (%__hlt_bframe*, %__hlt_eoss*, %__hlt_execution_context*)

; A basic frame
%__hlt_bframe = type { 
    %__hlt_continuation, ; Normal continuation.
    %__hlt_continuation, ; Exceptional continuation.
    %__hlt_exception*    ; Current exception.
}

; A continuation. 
; Must match hlt_continuation in continuation.h. See there for fields.
%__hlt_continuation = type { 
    %__hlt_func*,
    %__hlt_bframe*,
    %__hlt_eoss*
}

; Run-time type information. 
; Must match __hlt_type_info in rtti.h. See there for fields.
%__hlt_type_info = type { 
    i16,
    i16,
    %__hlt_cchar*,
    i16,
    %__hlt_void*,
    %__hlt_func*,
    %__hlt_func*,
    %__hlt_func*,
    %__hlt_func*,
    %__hlt_func*
}

; An exception type.
; Must match hlt_exception_type in exceptions.h. See there for fields.
%__hlt_exception_type = type { 
    %__hlt_cchar*,
    %__hlt_exception_type*,
    %__hlt_type_info*
}
    
; An exception instance.
; Must match hlt_exception in exceptions.h. See there for fields.
%__hlt_exception = type { 
    %__hlt_exception_type*,
    %__hlt_continuation*,
    %__hlt_void*,
    %__hlt_cchar*,
    %__hlt_bframe*,
    %__hlt_eoss*,
    %__hlt_vthread_id
}

; Identifier unique across virtual threads.
%__hlt_vthread_id = type i64

; The common header of all per-thread execution contexts. This
; structure is then followed with the set of all global variables. 
; Must match hlt_exception in context.h. See there for fields.
%__hlt_execution_context = type { 
    %__hlt_vthread_id,
    %__hlt_void*,
    %__hlt_continuation*,
    %__hlt_continuation*
}

; A global function to be run at startup.
; This must match with what LLVM expects for the llvm.global_ctorsarray.
%__hlt_global_ctor = type { i32, void() * }

;;; The following libhilti functions do not fit normal C_HILTI calling
;;; conventions and thus declared here directly.  
%__hlt_bytes_pos = type {i8*, i8*}
declare i8 @__hlt_bytes_extract_one(%__hlt_bytes_pos*, %__hlt_bytes_pos, %__hlt_exception**)

;;; Exception management.
declare %__hlt_exception* @__hlt_exception_new(%__hlt_exception_type*, %__hlt_void*, %__hlt_cchar*)
declare %__hlt_exception* @__hlt_exception_new_yield(%__hlt_continuation*, i32, i8*)
declare void @__hlt_exception_print_uncaught_abort(%__hlt_exception*)
declare i1 @__hlt_exception_match_type(%__hlt_exception*, %__hlt_exception_type*)

;;; Predefined exceptions.
@hlt_exception_unspecified = external constant %__hlt_exception_type
@hlt_exception_value_error = external constant %__hlt_exception_type
@hlt_exception_assertion_error = external constant %__hlt_exception_type
@hlt_exception_internal_error = external constant %__hlt_exception_type
@hlt_exception_division_by_zero = external constant %__hlt_exception_type
@hlt_exception_overlay_not_attached = external constant %__hlt_exception_type
@hlt_exception_undefined_value = external constant %__hlt_exception_type
@hlt_exception_iosrc_exhausted = external constant %__hlt_exception_type
@hlt_exception_yield = external constant %__hlt_exception_type
@hlt_exception_would_block = external constant %__hlt_exception_type

;;; Memory management.
declare void @__hlt_init_gc()
declare %__hlt_void* @hlt_gc_malloc_non_atomic(i64)
declare %__hlt_void* @hlt_gc_calloc_non_atomic(i64, i64)

;;; Bytes.
; This functions takes an int8_t parameter, which does not fit normal C_HILTI
; calling convention.
declare i8* @hlt_bytes_new_from_data(i8*, i32, %__hlt_exception**)

;;; Timers
declare %__hlt_void* @__hlt_timer_new_function(%__hlt_continuation*, %__hlt_exception**)

;;; Threads
declare void @__hlt_thread_mgr_schedule(%__hlt_void*, %__hlt_vthread_id, %__hlt_continuation*, %__hlt_execution_context*, %__hlt_exception**)

;;; A function with the standard header that simply aborts. 
declare void @__hlt_abort(%__hlt_bframe*, %__hlt_eoss*, %__hlt_execution_context*)

;;; Debugging.
declare void @__hlt_debug_print_str(i8*)
declare void @__hlt_debug_print_ptr(i8*, i8*)
declare void @__hlt_debug_push_indent()
declare void @__hlt_debug_pop_indent()

;;; Globals defined in globals.h
@__hlt_global_execution_context = external global %__hlt_execution_context*
@__hlt_global_thread_mgr = external global %__hlt_void*

;;;;;;;;;;;;;;;;;; OLD - Need to update.
; Thread scheduling functions.
;declare void @__hlt_global_schedule_job(i32, %__hlt_func*, i8*)
;declare void @__hlt_local_schedule_job(i8*, i8*)

; Standard handlers for normal execution and exception handling; used when
; constructing the frame for a scheduled job.
;declare fastcc void @__hlt_standard_cont_normal(i8*)
;declare fastcc void @__hlt_standard_cont_except(i8*)
