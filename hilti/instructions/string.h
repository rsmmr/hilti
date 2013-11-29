/// \type String
///
/// Strings are sequences of characters and are intended to be used primarily
/// for text that at some point might be presented to a human in one way or
/// the other. They are inmutable, copied by value on assignment, and
/// internally stored in UTF-8 encoding. Don't use it for larger amounts of
/// binary data, performance won't be good (use *bytes* instead).
///
/// String constants are written in the usual form ``"Foo"``. They are
/// assumed to be in 7-bit ASCII encoding; usage of 8-bit characters is
/// undefined at the moment.
///
/// .. todo:: Explain control sequences in string constants.
///
/// .. todo:: Add some way to define the encoding for constants.
///
/// \default Empty string
///
/// \ctor "foo", ""
///
/// \cproto hlt_string

#include "define-instruction.h"

iBegin(string, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::string, true);
    iOp2(optype::string, true);

    iValidate {
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )")

iEnd

iBegin(string, Cmp, "string.cmp")
    iTarget(optype::boolean)
    iOp1(optype::string, true);
    iOp2(optype::string, true);

    iValidate {
    }

    iDoc(R"(    
        Compares *op1* with *op2* and returns True if their characters match.
        Returns False otherwise.
    )")

iEnd

iBegin(string, Concat, "string.concat")
    iTarget(optype::string)
    iOp1(optype::string, true);
    iOp2(optype::string, true);

    iValidate {
    }

    iDoc(R"(    
        Concatenates *op1* with *op2* and returns the result.
    )")

iEnd

iBegin(string, Decode, "string.decode")
    iTarget(optype::string)
    iOp1(optype::refBytes, true);
    iOp2(optype::enum_, true);

    iValidate {
    }

    iDoc(R"(    
        Converts *bytes op1* into a string, assuming characters are encoded in
        character set *op2*. Supported character sets are currently:
        ``ASCII``, ``UTF8``, ``UTF16LE``, ``UTF16BE``, ``UTF32LE``, ``UTF32BE``.  
	If the string cannot be decoded with the given character set, the instruction 
	throws an ~~DecodingError exception. If the character set given is not supported,
       	the instruction throws a ~~ValueError exception.

        .. todo:: Support further character sets.
    )")

iEnd

iBegin(string, Encode, "string.encode")
    iTarget(optype::refBytes)
    iOp1(optype::string, true);
    iOp2(optype::enum_, true);

    iValidate {
    }

    iDoc(R"(    
        Converts *op1* into bytes, encoding characters using the character set
        *op2*. Supported character sets are currently: ``ASCII``, ``UTF8``.
        If the any characters cannot be encoded with the given character set,
        they will be replaced with place-holders. If the character set given
        is not supported, the instruction throws a ~~ValueError exception.  Todo:
        We need to figure out how to support more character sets.
    )")

iEnd

iBegin(string, Find, "string.find")
    iTarget(optype::int64)
    iOp1(optype::string, true);
    iOp2(optype::string, true);

    iValidate {
    }

    iDoc(R"(    
        Searches *op2* in *op1*, returning the start index if it find it.
        Returns -1 if it does not find *op2* in *op1*.
    )")

iEnd

iBegin(string, Length, "string.length")
    iTarget(optype::int64)
    iOp1(optype::string, true);

    iValidate {
    }

    iDoc(R"(    
        Returns the number of characters in the string *op1*.
    )")

iEnd

iBegin(string, Lt, "string.lt")
    iTarget(optype::boolean)
    iOp1(optype::string, true);
    iOp2(optype::string, true);

    iValidate {
    }

    iDoc(R"(    
        Compares *op1* with *op2* and returns True if *op1* is
        lexicographically smaller than *op2*. Returns False otherwise.
    )")

iEnd

iBegin(string, Substr, "string.substr")
    iTarget(optype::string)
    iOp1(optype::string, true)
    iOp2(optype::int64, true)
    iOp3(optype::int64, true)

    iValidate {
    }

    iDoc(R"(    
        Extracts the substring of length *op3* from *op1* that starts at
        position *op2*.
    )")

iEnd

