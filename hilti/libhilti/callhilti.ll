; This file contains trampolines to enable a C function to call a HILTI function or continuation,
; and related functionality.

%__hlt_function_pointer = type void(i8*)*
%__hlt_continuation = type { i8*, i8* }
%__hlt_basic_frame = type { %__hlt_continuation, %__hlt_continuation, i8* }

; Enables a C program to call a HILTI continuation.
; The first parameter is a pointer to the HILTI function to be called.
; The second parameter is a pointer to the function's frame.
define ccc void @__hlt_call_hilti(%__hlt_function_pointer, i8*)
{
    tail call fastcc void %0(i8* %1)
    ret void
}

; Provides access to the exception field of a HILTI continuation.
define ccc i8* @__hlt_get_hilti_exception(i8*)
{
    %frame = bitcast i8* %0 to %__hlt_basic_frame*
    %except_addr = getelementptr %__hlt_basic_frame* %frame, i32 0, i32 2
    %exception = load i8** %except_addr
    ret i8* %exception
}

define fastcc void @__hlt_standard_cont_normal(i8*)
{
    ret void
}

define fastcc void @__hlt_standard_cont_except(i8*)
{
    ret void
}

%struct.string___XUB = type { i32, i32 }
@C.168.1155 = internal constant %struct.string___XUB { i32 1, i32 12 }

;@hlt_standard_continuation = constant %hlt_continuation { i8* null, i8* null }
;@__hlt_standard_frame = constant %__hlt_basic_frame { %__hlt_continuation @__hlt_standard_continuation, %__hlt_continuation @__hlt_standard_continuation, i8* 0 } ;
@__hlt_standard_frame = constant %__hlt_basic_frame zeroinitializer
