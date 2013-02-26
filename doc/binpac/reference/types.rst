
Data Types
----------

.. contents::

.. pac2:type:: address

Address
~~~~~~~

* Type: ``addr``
* Constants: ``192.168.1.1``,  ``[2001:db8:85a3:8d3:1319:8a2e:370:7348]``, ``[::1]``
* Addresses are passed by value.

The ``addr`` type stores IP addresses. It handles IPv4 and IPv6
addresses transparently. Note that IPv6 constants need to be enclosed
in brackets. For a given ``addr`` instance,
:pac2:method:`address::family` retrieves the family.

.. include:: /build/autogen/pac2-address.rst

.. pac2:type:: bool

Bool
~~~~

* Type: ``bool``
* Constants: ``True``, ``False``
* Booleans are passed by values.

[TODO: Overview]

.. include:: /build/autogen/pac2-bool.rst


.. pac2:type:: bytes

Bytes
~~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-bytes.rst


.. pac2:type:: double

Double
~~~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-double.rst

.. pac2:type:: enum

Enum
~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-enum.rst


.. pac2:type:: function

Function
~~~~~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-function.rst


.. pac2:type:: integer

Integer
~~~~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-integer.rst


.. pac2:type:: interval

Interval
~~~~~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-interval.rst


.. pac2:type:: iterator

Iterator
~~~~~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-iterator.rst


.. pac2:type:: list

List
~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-list.rst


.. pac2:type:: sink

Sink
~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-sink.rst


.. pac2:type:: time

Time
~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-time.rst


.. pac2:type:: tuple

Tuple
~~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-tuple.rst


.. pac2:type:: unit

Unit
~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-unit.rst


.. pac2:type:: vector

Vector
~~~~~~

[TODO: Overview]

.. include:: /build/autogen/pac2-vector.rst


