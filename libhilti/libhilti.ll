
%hlt.void = type i8
%hlt.char = type i8
%hlt.func = type i8

%hlt.vid = type i64

; The header of garbage collected objects.
%hlt.gchdr = type {
    i64 ;; Reference count.
}

; The per-thread execution context.
; Adapt codegen::hlt::ExecutionContext and hlt_execution_context when making changes here.
%hlt.execution_context = type {
    %hlt.gchdr,
    %hlt.vid,
    i64,
    i8*  ;; Start of globals (right here, pointer content isn't used.)
}

; An exception instance.
; Adapt codegen::hlt::Exception and hlt_exception when making changes here.
%hlt.exception = type {
    i8* ;; Name of exception.
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

; Types we don't specify further at the LLVM level.
%hlt.bytes = type {};

;;; libhilti functions that don't fit the normal calling conventions.

declare void @__hlt_object_ref(%hlt.type_info*, i8 *)
declare void @__hlt_object_unref(%hlt.type_info*, i8 *)
declare void @__hlt_object_dtor(%hlt.type_info*, i8 *, i8*)
declare void @__hlt_object_cctor(%hlt.type_info*, i8 *, i8*)

declare void @__hlt_debug_print(i8*, i8*)

declare %hlt.string* @hlt_string_from_data(i8*, i64, %hlt.exception**, %hlt.execution_context*)
declare %hlt.bytes*  @hlt_bytes_new_from_data_copy(i8*, i64, %hlt.exception**, %hlt.execution_context*)

declare void @hlt_abort()

;;; Exception types used by the code generator.
