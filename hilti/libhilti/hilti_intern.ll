; $Id$
; 
; LLVM prototypes for the functions defined in hilti_intern.h
;
; See hilti_intern.h for more information about these functions.

; Types
%__hlt_exception = type i8*
%__hlt_string = type < { i32, [0 x i8] } >
%__hlt_string_len = type i32

; Integer functions.
declare ccc %__hlt_string* @__hlt_int_fmt(i32, i32, %__hlt_exception*)

; Bool functions.
declare ccc %__hlt_string* @__hlt_bool_fmt(i8, i32, %__hlt_exception*)

; String functions.

declare ccc %__hlt_string* @__hlt_string_fmt(%__hlt_string*, i32, %__hlt_exception*)
declare ccc %__hlt_string_len @__hlt_string_len(%__hlt_string*, %__hlt_exception*)
declare ccc %__hlt_string* @__hlt_string_concat(%__hlt_string*, %__hlt_string*, %__hlt_exception*)


