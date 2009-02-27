; $Id$
; 
; LLVM prototypes for the functions defined in hilti_intern.h

%__hlt_string = type < { i32, [0 x i8] } >
%__hlt_string_len = type i32

declare ccc %__hlt_string_len @__hlt_string_len(%__hlt_string*)

