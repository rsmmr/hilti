///
/// \type iosrc<Hilti::IOSrc:*>
///
/// The *iosrc* data type represents a source of external input coming in for
/// processing. It transparently supports a set of different sources
/// (currently only ``libpcap``-based, but in the future potentially other's
/// as well.). Elements of an IOSrc are ``bytes`` objects and come with a
/// timestamp.
///
/// \cproto hlt_iosrc*
///
/// \type ``iter<iosrc>``
///
/// \default An iterator over the elements retrieved from an *iosrc*.
///
/// \cproto hlt_bytes_iter
///

iBegin(iterIOSource, Begin, "begin")
    iTarget(optype::iterIOSource);
    iOp1(optype::refIOSource, true);

    iValidate {
        equalTypes(iteratedType(target), referencedType(op1));
    }

    iDoc(R"(
        Returns an iterator for iterating over all elements of a packet source
        *op1*. The instruction will block until the first element becomes
        available.
    )")
iEnd

iBegin(iterIOSource, End, "end")
    iTarget(optype::iterIOSource);
    iOp1(optype::refIOSource, true);

    iValidate {
        equalTypes(iteratedType(target), referencedType(op1));
    }

    iDoc(R"(
         Returns an iterator representing an exhausted packet source *op1*.
    )")
iEnd

iBegin(iterIOSource, Incr, "incr")
    iTarget(optype::iterIOSource);
    iOp1(optype::iterIOSource, true);

    iValidate {
        equalTypes(target, op1);
    }

    iDoc(R"(
        Advances the iterator to the next element, or the end position if
        already exhausted. The instruction will block until the next element
        becomes available.
    )")
iEnd

iBegin(iterIOSource, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::iterIOSource, true);
    iOp2(optype::iterIOSource, true);

    iValidate {
        equalTypes(target, op1);
    }

    iDoc(R"(
       Returns true if *op1* and *op2* reference the same element.
    )")

iEnd

iBegin(iterIOSource, Deref, "deref")
    iTarget(optype::int8)
    iOp1(optype::iterIOSource, true);

    iValidate {
    }

    iDoc(R"(
       Returns the element the iterator is pointing at as a tuple ``(double,
       ref<bytes>)``.
    )")

iEnd


iBegin(ioSource, New, "new")
    iTarget(optype::refIOSource)
    iOp1(optype::typeIOSource, true)

    iValidate {
        hasType(target, typedType(op1));
    }

    iDoc(R"(
        Instantiates a new *iosrc* instance, and initializes it for reading.
        The format of string *op2* is depending on the kind of ``iosrc``. For
        :hlt:glob:`PcapLive`, it is the name of the local interface to IOSourceen
        on. For :hlt:glob:`PcapLive`, it is the name of the trace file.

        Raises: :hlt:type:`IOSrcError` if the packet source cannot be opened.
    )")

iEnd


iBegin(ioSource, Close, "iosrc.close")
    iOp1(optype::refIOSource, false)

    iValidate {
    }

    iDoc(R"(    
        Closes the packet source *op1*. No further packets can then be read
        (if tried, ``pkrsrc.read`` will raise a ~~IOSrcError exception).
    )")

iEnd

iBegin(ioSource, Read, "iosrc.read")
    iTarget(optype::tuple)
    iOp1(optype::refIOSource, false)

    iValidate {
        // auto ty_target = as<type::(time,refbytes)>(target->type());
        // auto ty_op1 = as<type::refIOSource>(op1->type());

        // TODO:  Check tuple.
    }

    iDoc(R"(    
        Returns the next element from the I/O source *op1*. If currently no
        element is available, the instruction blocks until one is.  Raises:
        ~~IOSrcExhausted if the source has been exhausted.  Raises:
        ~~IOSrcError if there is any other problem with returning the next
        element.
    )")

iEnd
