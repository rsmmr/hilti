; $Id$
;
; LLVM code that will be implicitly added to every module.

; HILTI type declarations.
%__hlt_continuation = type { i8*, i8* }
%__hlt_basic_frame = type { %__hlt_continuation, %__hlt_continuation, i8* }
%__hlt_function_pointer = type void(%__hlt_basic_frame*)*

; This function takes it's iterator<bytes> parameter as a pointer and modified
; it in place, which does not fit normal C_HILTI calling convention.
declare i8 @__hlt_bytes_extract_one(i8*, {i8*, i8*}, i8**)

; This functions takes an int8_t parameter, which does not fit normal C_HILTI
; calling convention.
declare i8* @__hlt_bytes_new_from_data(i8*, i32, i8**)

; Thread scheduling functions.
declare void @__hlt_global_schedule_job(i32, %__hlt_function_pointer, i8*)
declare void @__hlt_local_schedule_job(%__hlt_function_pointer, i8*)

; Standard handlers for normal execution and exception handling; used when
; constructing the frame for a scheduled job.
declare fastcc void @__hlt_standard_cont_normal(i8*)
declare fastcc void @__hlt_standard_cont_except(i8*)

; A standard frame, sharable between any continuations which do not access or
; write to their frames. Used as the frame for __hlt_standard_cont_normal().
@__hlt_standard_frame = external global %__hlt_basic_frame
