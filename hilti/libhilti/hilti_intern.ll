; $Id$
;
; LLVM code that will be implicitly added to every module.

; This function takes it's iterator<bytes> parameter as a pointer and modified
; it in place, which does not fit normal C_HILTI calling convention.
declare i8 @__hlt_bytes_extract_one(i8*, {i8*, i8*}, i8**)

; This functions takes an int8_t parameter, which does not fit normal C_HILTI
; calling convention.
declare i8* @__hlt_bytes_new_from_data(i8*, i32, i8**)

%__hlt_continuation = type { i8*, i8* }
%__hlt_basic_frame = type { %__hlt_continuation, %__hlt_continuation, i8* }

declare fastcc void @__hlt_standard_cont_normal(i8*)
declare fastcc void @__hlt_standard_cont_except(i8*)

@__hlt_standard_frame = external global %__hlt_basic_frame
