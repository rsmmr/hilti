; This file contains trampolines to enable a C function to call a HILTI function or continuation,
; and related functionality.

%__hlt_continuation = type { i8*, i8* }
%__hlt_basic_frame = type { %__hlt_continuation, %__hlt_continuation, i8* }
%__hlt_function_pointer = type void(%__hlt_basic_frame*)*

declare void @__hlt_report_local_exception(i8*)

; Enables a C program to call a HILTI continuation.
; The first parameter is a pointer to the HILTI function to be called.
; The second parameter is a pointer to the function's frame.
define ccc void @__hlt_call_hilti(%__hlt_function_pointer, %__hlt_basic_frame*)
{
    tail call fastcc void %0(%__hlt_basic_frame* %1)
    ret void
}

; Standard handler for normal execution; used when constructing the
; frame for a scheduled job.
define fastcc void @__hlt_standard_cont_normal(%__hlt_basic_frame*)
{
    ret void
}

; Standard handler for exceptions; used when constructing the frame
; for a scheduled job.
define fastcc void @__hlt_standard_cont_except(%__hlt_basic_frame*)
{
    %except_addr = getelementptr %__hlt_basic_frame* %0, i32 0, i32 2
    %exception = load i8** %except_addr

    call ccc void @__hlt_report_local_exception(i8* %exception)

    ret void
}

; A standard frame, sharable between any continuations which do not access or
; write to their frames. Used as the frame for __hlt_standard_cont_normal().
@__hlt_standard_frame = constant %__hlt_basic_frame zeroinitializer
