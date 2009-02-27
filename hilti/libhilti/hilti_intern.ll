; $Id$
; 
; LLVM prototypes for the functions defined in hilti_intern.h
;
; See hilti_intern.h for more information about these functions.


%__hlt_exception = type i8*

%__hlt_string = type < { i32, [0 x i8] } >
%__hlt_string_len = type i32

declare ccc %__hlt_string_len @__hlt_string_len(%__hlt_string*, %__hlt_exception*)
declare ccc %__hlt_string* @__hlt_string_concat(%__hlt_string*, %__hlt_string*, %__hlt_exception*)


