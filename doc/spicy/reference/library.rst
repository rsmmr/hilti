
Runtime Library
---------------

This document summarizes the types and function defined in Spicy's
standard runtime library. These can be accesses by importating the
``spicy`` module.

Types
~~~~~

.. spicy:type:: spicy::AddrFamily

.. literalinclude:: /../libspicy/spicy.spicy
    :start-after: %begin-AddrFamily
    :end-before:  %end-AddrFamily

.. spicy:type:: spicy::ByteOrder

.. literalinclude:: /../libspicy/spicy.spicy
    :start-after: %begin-ByteOrder
    :end-before:  %end-ByteOrder

.. spicy:type:: spicy::Filter

.. literalinclude:: /../libspicy/spicy.spicy
    :start-after: %begin-Filter
    :end-before:  %end-Filter

Functions
~~~~~~~~~

.. spicy:function:: spicy::base64_encode

.. literalinclude:: /../libspicy/spicy.spicy
    :start-after: %begin-base64_encode
    :end-before:  %end-base64_encode

.. spicy:function:: spicy::base64_decode

.. literalinclude:: /../libspicy/spicy.spicy
    :start-after: %begin-base64_decode
    :end-before:  %end-base64_decode
