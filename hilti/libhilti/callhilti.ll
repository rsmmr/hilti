; This file contains trampolines to enable a C function to call a HILTI function or continuation.

%__hilti_function_pointer = type void(i8*)*

; Enables a C program to call a HILTI continuation.
; The first parameter is a pointer to the HILTI function to be called.
; The second parameter is a pointer to the function's frame.
define ccc void @__hlt_call_hilti(%__hilti_function_pointer, i8*)
{
    tail call fastcc void %0(i8* %1)
    ret void
}       
