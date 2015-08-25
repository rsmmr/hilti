
Runtime Library
---------------

This document summarizes the types and function defined in Spicy's
standard runtime library. These can be accesses by importating the
``binpac`` module.

Types
~~~~~

.. pac2:type:: binpac::AddrFamily

.. literalinclude:: /../libbinpac/binpac.pac2
    :start-after: %begin-AddrFamily
    :end-before:  %end-AddrFamily

.. pac2:type:: binpac::ByteOrder

.. literalinclude:: /../libbinpac/binpac.pac2
    :start-after: %begin-ByteOrder
    :end-before:  %end-ByteOrder

.. pac2:type:: binpac::Filter

.. literalinclude:: /../libbinpac/binpac.pac2
    :start-after: %begin-Filter
    :end-before:  %end-Filter

Functions
~~~~~~~~~

.. pac2:function:: binpac::base64_encode

.. literalinclude:: /../libbinpac/binpac.pac2
    :start-after: %begin-base64_encode
    :end-before:  %end-base64_encode

.. pac2:function:: binpac::base64_decode

.. literalinclude:: /../libbinpac/binpac.pac2
    :start-after: %begin-base64_decode
    :end-before:  %end-base64_decode
