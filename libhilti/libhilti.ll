
%hlt.void = type i8
%hlt.char = type i8
%hlt.func = type i8

%hlt.vid = type i64

; The header of garbage collected objects.
%hlt.gchdr = type {
    i64 ;; Reference count.
}

; The per-thread execution context.
; Adapt codegen::hlt::ExecutionContext::Globals and hlt_execution_context when making changes here!
%hlt.execution_context = type {
    %hlt.gchdr,
    %hlt.vid,
    i64,
    %hlt.exception.type*,
    %hlt.fiber*,
    i8*  ;; Start of globals (right here, pointer content isn't used.)
}

; The static description of a callable instance. This is function
; specific but parameter specific.
; Adapt hlt_callable_func when making changes here!
%hlt.callable.func = type {
    i8*, ;; Function pointer for call.
    i8*, ;; Function pointer for call that discards result.
    i8*  ;; Function pointer for dtor.
}

; A callable object.
; Adapt hlt_callable when making changes here!
%hlt.callable = type {
    %hlt.gchdr,
    %hlt.callable.func*
    ;; Arguments will follow here.
}

; Run-time type information. We don't declare the fields here as we
; don't access them from the compiler (other than building record
; instances, for which we don't need the full declaration).
%hlt.type_info = type {}

; A HILTI string object.
%hlt.string = type {
    %hlt.gchdr,
    i64,
    i8  ;; Start of data.
}

; A bytes iterator.
%hlt.iterator.bytes = type {
    i8*,
    i8*
}

; A list iterator.
%hlt.iterator.list = type {
    i8*,
    i8*
}

; A vector iterator.
%hlt.iterator.vector = type {
    i8*,
    i64
}

; A set iterator.
%hlt.iterator.set = type {
    i8*,
    i64
}

; A map iterator.
%hlt.iterator.map = type {
    i8*,
    i64
}


; An exception type. This must match hlt_exception_type in exception.h
; FIXME: We're getting type trouble here if we spell out the names.
%hlt.exception.type = type {
    i8*,
    i8*,
    i8**
}

; Types we don't specify further at the LLVM level.
%hlt.bytes = type {};
%hlt.exception = type {};
%hlt.timer = type {};
%hlt.timer_mgr = type {};
%hlt.list = type {};
%hlt.fiber = type {};

;;; libhilti functions that don't fit the normal calling conventions.

declare void @__hlt_object_ref(%hlt.type_info*, i8 *)
declare void @__hlt_object_unref(%hlt.type_info*, i8 *)
declare void @__hlt_object_dtor(%hlt.type_info*, i8 *, i8*)
declare void @__hlt_object_cctor(%hlt.type_info*, i8 *, i8*)
declare i8* @__hlt_object_new(%hlt.type_info*, i64, i8*)

declare void @__hlt_debug_print(i8*, i8*)

declare %hlt.string* @hlt_string_from_data(i8*, i64, %hlt.exception**, %hlt.execution_context*)
declare %hlt.bytes*  @hlt_bytes_new_from_data_copy(i8*, i64, %hlt.exception**, %hlt.execution_context*)

declare i8 @__hlt_bytes_extract_one(%hlt.iterator.bytes*, %hlt.iterator.bytes, %hlt.exception**, %hlt.execution_context*)

declare void            @__hlt_exception_print_uncaught_abort(%hlt.exception*, %hlt.execution_context*)
declare i8              @__hlt_exception_match(%hlt.exception*, %hlt.exception.type*)
declare %hlt.exception* @hlt_exception_new(%hlt.exception.type*, i8*, i8*)
declare %hlt.exception* @hlt_exception_new_yield(%hlt.fiber*, i8*)
declare i8*             @hlt_exception_arg(%hlt.exception*)
declare %hlt.fiber*     @__hlt_exception_fiber(%hlt.exception*)
declare void            @__hlt_exception_clear_fiber(%hlt.exception*)

declare %hlt.exception* @__hlt_context_get_exception(%hlt.execution_context*)
declare void            @__hlt_context_set_exception(%hlt.execution_context*, %hlt.exception*)
declare void            @__hlt_context_clear_exception(%hlt.execution_context*)
declare %hlt.fiber*     @__hlt_context_get_fiber(%hlt.execution_context*)

declare %hlt.execution_context* @hlt_global_execution_context();

declare %hlt.timer*     @__hlt_timer_new_function(%hlt.callable*, %hlt.exception**, %hlt.execution_context*)

declare i8* @hlt_malloc(i64)

declare void @hlt_abort()

declare %hlt.fiber* @hlt_fiber_create(i8*, %hlt.execution_context*, i8*)
declare void        @hlt_fiber_delete(%hlt.fiber*)
declare i8          @hlt_fiber_start(%hlt.fiber*)
declare void        @hlt_fiber_return(%hlt.fiber*)
declare void        @hlt_fiber_yield(%hlt.fiber*)
declare i8*         @hlt_fiber_get_result_ptr(%hlt.fiber*)
declare void        @hlt_fiber_set_result_ptr(%hlt.fiber*, i8*)
declare %hlt.execution_context* @hlt_fiber_context(%hlt.fiber*)

;;; Exception types used by the code generator.
@hlt_exception_unspecified = external constant %hlt.exception.type
@hlt_exception_value_error = external constant %hlt.exception.type
@hlt_exception_assertion_error = external constant %hlt.exception.type
@hlt_exception_internal_error = external constant %hlt.exception.type
@hlt_exception_division_by_zero = external constant %hlt.exception.type
@hlt_exception_index_error = external constant %hlt.exception.type
@hlt_exception_overlay_not_attached = external constant %hlt.exception.type
@hlt_exception_undefined_value = external constant %hlt.exception.type
@hlt_exception_iosrc_exhausted = external constant %hlt.exception.type
@hlt_exception_yield = external constant %hlt.exception.type
@hlt_exception_would_block = external constant %hlt.exception.type
@hlt_exception_no_thread_context = external constant %hlt.exception.type

;;; libc
declare double @pow(double, double)
