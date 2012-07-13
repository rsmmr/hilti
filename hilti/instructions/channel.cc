
#include "instructions/define-instruction.h"

#include "channel.h"
#include "module.h"

iBeginCC(channel)
    iValidateCC(New) {
        equalTypes(referencedType(target), typedType(op1));
    }

    iDocCC(New, R"(
         Allocates a new instance of a channel storing elements of type *op1*.
         *op2* defines the channel's capacity, i.e., the maximal number of items it
         can store. The capacity defaults to zero, which creates a channel of
         unbounded capacity.
    )")
iEndCC

iBeginCC(channel)
    iValidateCC(Read) {
        canCoerceTo(argType(op1), target);
    }

    iDocCC(Read, R"(    
        Returns the next channel item from the channel referenced by *op1*. If
        the channel is empty, the instruction blocks until an item becomes
        available.
    )")
iEndCC

iBeginCC(channel)
    iValidateCC(ReadTry) {
        canCoerceTo(argType(op1), target);
    }

    iDocCC(ReadTry, R"(    
        Returns the next channel item from the channel referenced by *op1*. If
        the channel is empty, the instruction raises a ``WouldBlock``
        exception.
    )")
iEndCC

iBeginCC(channel)
    iValidateCC(Size) {
    }

    iDocCC(Size, R"(    
        Returns the current number of items in the channel referenced by
        *op1*.
    )")
iEndCC

iBeginCC(channel)
    iValidateCC(Write) {
        canCoerceTo(op2, argType(op1));
    }

    iDocCC(Write, R"(    
        Writes an item into the channel referenced by *op1*. If the channel is
        full, the instruction blocks until a slot becomes available.
    )")
iEndCC

iBeginCC(channel)
    iValidateCC(WriteTry) {
        canCoerceTo(op2, argType(op1));
    }

    iDocCC(WriteTry, R"(    
        Writes an item into the channel referenced by *op1*. If the channel is
        full, the instruction raises a ``WouldBlock`` exception.
    )")
iEndCC
