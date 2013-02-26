
Grammars
--------

.. contents::

Global Definitions
~~~~~~~~~~~~~~~~~~

.. _pac2_global_global:

``global``
    TODO.

.. _pac2_global_constants:

``const``
    TODO.

.. _pac2_global_import:

``import``
    TODO.

.. _pac2_global_types:

``type``
    TODO.

Statements
    Statements at the global level will executed once as a module is
    initialized for the first time by the host application.

Units
~~~~~

TODO.

.. _pac2_unit_variables:

Variables
^^^^^^^^^

TODO.

.. pac2_unit_properties:

Properties
^^^^^^^^^^

``%mime-type``
    TODO.

``%description``
    TODO.

``%port``
    TODO.

.. _pac2_hooks:

Hooks
~~~~~

A hook is code that is will be executed during parsing at certain
points of time. There are three types of hooks, field hooks, unit
hooks, and item hooks.

In all hooks, the ``self`` identifier refers to the unit being parsed.

Unit Hooks
    Unit hooks are not tied to a specific field but apply to the
    parsing process of the unit they are part of. They are always
    defined using the ``on <hook-name>`` syntax, e.g.::

        type Foo = unit {
            ...
            on %init { <code> }
            ...
        }

    Currently, BinPAC++ defines these unit hooks:

    ``%init``
        Executed when the parsing of a unit is about to start. All of
        the fields and variables will have their default values at the
        time the hook's code executes (which may be unset and thus
        trigger an exception when being read). 

    ``%done``
        Executed when the parsing of a unit instance has completed.
        All fields and variables will have their final values.

    ``%debug``
        Executed when the parsing of a unit instance has completed
        *and* debugging is activated (i.e., has been compiled in and
        is enabled at run-time, see :ref:`pac2_debugging`).

Field and Variable Hooks
    Field hooks are associated with a specific field and execute
    whenver that field has been fully parsed. For example, the
    following code snippet adds a hook to the ``uri``, in this case
    simply printing the field's content::

        uri: Token {
            print self.uri;
            }

   Note how ``self`` still refers to the unit, not the field.

   Often, a field hook will store information in other
   :ref:`pac2_unit_variables` , like in this toy example::

        uri : Token {
            if ( self.uri.startswith("http://") )
                self.proxy = True;
            }

        var proxy : bool;

    Field hooks can also be specified at the unit level using the ``on
    <field-name>`` syntax:

        uri: Token;

        ...

        on uri {
            print self.uri;
            }

Item Hooks
    Item hooks are associated with container types (e.g.,
    :pac2:type::`list`, :pac2:type::`vector`) and execute each time
    one containter items has beed parsed. These hooks are marked with
    the ``foreach`` keyword, and they have access to the current item
    via the reserved ``$$`` identifier. Example:

        lines: list<Item> &until($$.line == b"---\n")
                          foreach { print $$; }

    As this parses :pac2:type:`list` elements, each will be printed
    out. (Note how the list's ``&until`` also has access to ``$$``.)


In addition to specifying hooks inside a unit, they can also be
proivded externally at the global level, using again the ``on
<hook-name>`` syntax where ``<hook-name>`` is now the fully qualified
name:

    type RequestLine = unit {
        ...
        uri:     Token;
        ...
    };

    on RequestLine::uri {
        print self.uri;
        }

    on RequestLine::%done {
        ...
        }

Note that parsing will always execute *all* relevant hooks defined
anywhere in the input specification. It's perfectly fine to define a
field hook inline and later add more external hooks to the same field;
all of them will be executed (in an undefined order). This even works
across units: if you :ref:`import <pac2_global_import>` the
``Request`` module, into the another specification, you can add a hook
to it like this::

    on Request::RequestLine::uri {
        print self.uri;
        }


Conditional Parsing
~~~~~~~~~~~~~~~~~~~

    switch
    if

Random Access
~~~~~~~~~~~~~

Sinks
~~~~~

Filters
~~~~~~~

Fields
~~~~~~

Standard attributes
    &default



Types
~~~~~

Bytes
^^^^^

    &length
    &until
    &eod
    regexp

List
^^^^

Vector
^^^^^^

Integer
^^^^^^^

Subunit
^^^^^^^

    

