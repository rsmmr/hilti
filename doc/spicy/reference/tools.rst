
Tools [Missing]
---------------

.. _spicy_spicy-driver:

spicy-driver [Missing]
~~~~~~~~~~~~~~~~~~~~

.. _spicy_spicyc:

.. note::

    Some old text to reuse later::

        The *incremental* mode is primarily for testing the correct
        functioning of the generated parsers: |spicy| analyzers are fully
        incremental in the sense that you can feed them the input in arbitrary
        pieces; a parser parses as much as it can and then yields back to the
        host application until further input becomes available. By default,
        ``spicy-driver`` first reads any data available on standard input completely and
        then passes it into the parser in a *single* chunk of bytes. In
        incremental mode, however, passes consecutive chunks of the given size
        to the parser. 

        Note that there should be no *difference* in output/results between
        standard and incremental parsing of any chunk size. If there is,
        that's a bug. Running with ``-i 1`` is a good stress test for
        generated parsers. 

spicyc [Missing]
~~~~~~~~~~~~~~~~~~
