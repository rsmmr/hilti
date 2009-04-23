; $Id$
;
; LLVM code that will be implicitly added to every module.

; This function takes it's iterator<bytes> parameter as a pointer and modified
; it in place, which does not fit normal C_HILTI calling convention.
declare i8 @__hlt_bytes_extract_one(i8*, {i8*, i8*}, i8**)

; This functions takes an int8_t parameter, which does not fit normal C_HILTI
; calling convention.
declare i8* @__hlt_bytes_new_from_data(i8*, i32, i8**)
