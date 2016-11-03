
.. _data_types:

Data Types
----------

.. contents::

.. spicy:type:: address

Address
~~~~~~~

* Type: ``addr``
* Example constants: ``192.168.1.1``,  ``[2001:db8:85a3:8d3:1319:8a2e:370:7348]``, ``[::1]``
* Addresses are passed by value.

The ``addr`` type stores IP addresses. It handles IPv4 and IPv6
addresses transparently. Note that IPv6 constants need to be enclosed
in brackets. For a given ``addr`` instance,
:spicy:method:`address::family` retrieves the family.

.. include:: /build/autogen/spicy-address.rst

.. spicy:type:: bool

Bool
~~~~

* Type: ``bool``
* Example constants: ``True``, ``False``
* Booleans are passed by values.

[TODO: Overview]

.. include:: /build/autogen/spicy-bool.rst


.. spicy:type:: bytes

Bytes
~~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-bytes.rst


.. spicy:type:: double

Double
~~~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-double.rst

.. spicy:type:: enum

Enum
~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-enum.rst


.. spicy:type:: function

Function
~~~~~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-function.rst


.. spicy:type:: integer

Integer
~~~~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-integer.rst


.. spicy:type:: interval

Interval
~~~~~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-interval.rst


.. spicy:type:: iterator

Iterator
~~~~~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-iterator.rst


.. spicy:type:: list

List
~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-list.rst


.. spicy:type:: map

Map
~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-map.rst


.. spicy:type:: set

Set
~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-set.rst


.. spicy:type:: sink

Sink
~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-sink.rst


.. spicy:type:: time

Time
~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-time.rst


.. spicy:type:: tuple

Tuple
~~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-tuple.rst


.. spicy:type:: unit

Unit
~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-unit.rst


.. spicy:type:: vector

Vector
~~~~~~

[TODO: Overview]

.. include:: /build/autogen/spicy-vector.rst


