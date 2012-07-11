
#include "instructions/define-instruction.h"

#include "iosrc.h"
#include "module.h"

iBeginCC(iterIOSource)
    iValidateCC(Begin) {
        equalTypes(iteratedType(target), referencedType(op1));
    }

    iDocCC(Begin, R"(
        Returns an iterator for iterating over all elements of a packet source
        *op1*. The instruction will block until the first element becomes
        available.
    )")
iEndCC

iBeginCC(iterIOSource)
    iValidateCC(End) {
        equalTypes(iteratedType(target), referencedType(op1));
    }

    iDocCC(End, R"(
         Returns an iterator representing an exhausted packet source *op1*.
    )")
iEndCC

iBeginCC(iterIOSource)
    iValidateCC(Incr) {
        equalTypes(target, op1);
    }

    iDocCC(Incr, R"(
        Advances the iterator to the next element, or the end position if
        already exhausted. The instruction will block until the next element
        becomes available.
    )")
iEndCC

iBeginCC(iterIOSource)
    iValidateCC(Equal) {
        equalTypes(op1, op2);
    }

    iDocCC(Equal, R"(
       Returns true if *op1* and *op2* reference the same element.
    )")

iEndCC

iBeginCC(iterIOSource)
    iValidateCC(Deref) {
        // TODO:  Check tuple.
    }

    iDocCC(Deref, R"(
       Returns the element the iterator is pointing at as a tuple ``(double,
       ref<bytes>)``.
    )")

iEndCC


iBeginCC(ioSource)
    iValidateCC(New) {
        equalTypes(referencedType(target), typedType(op1));
    }

    iDocCC(New, R"(
        Instantiates a new *iosrc* instance, and initializes it for reading.
        The format of string *op2* is depending on the kind of ``iosrc``. For
        :hlt:glob:`PcapLive`, it is the name of the local interface to IOSourceen
        on. For :hlt:glob:`PcapLive`, it is the name of the trace file.

        Raises: :hlt:type:`IOSrcError` if the packet source cannot be opened.
    )")

iEndCC


iBeginCC(ioSource)
    iValidateCC(Close) {
    }

    iDocCC(Close, R"(    
        Closes the packet source *op1*. No further packets can then be read
        (if tried, ``pkrsrc.read`` will raise a ~~IOSrcError exception).
    )")

iEndCC

iBeginCC(ioSource)
    iValidateCC(Read) {
        // TODO:  Check tuple.
    }

    iDocCC(Read, R"(    
        Returns the next element from the I/O source *op1*. If currently no
        element is available, the instruction blocks until one is.  Raises:
        ~~IOSrcExhausted if the source has been exhausted.  Raises:
        ~~IOSrcError if there is any other problem with returning the next
        element.
    )")

iEndCC
