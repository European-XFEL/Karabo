..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _projects/howto_document:

**************************************
How to contibute to this documentation
**************************************

.. note::

    Always start a file with reference to itself. E.g the first lines of this
    file look like this:

    .. code:: rst

        .. _projects/howto_document:

        **************************************
        How to contibute to this documentation
        **************************************

    In order to allow proper cross-referencing later,
    use one of the following pre-fixes:

    * *framework/* - For all framework related documentation
    * *devices/* - For all device related documentation
    * *projects/* - For documentation about karabo projects or setups

Headers
=======

Only use one top header (section header) per file.

Top headers use stars ("\*") as surrounding.
Sub-headers are underlined by "=", "+", "_" and "^" in this order.

Sub-Header
----------

Sub-Sub Header
++++++++++++++

Sub-Sub-Sub Header
^^^^^^^^^^^^^^^^^^

.. code:: rst

    Headers
    =======

    Sub-Header
    ----------

    Sub-Sub Header
    ++++++++++++++

    Sub-Sub-Sub Header

Paragraphs
==========

This text reflects a regular paragraph. Use the same indentation and separate
text by adding one or more blank lines.

Do not use more than 80 characters per line.

Inline Markup
=============

Inline markup should go like this: *emphasis*, **strong emaphasis**,
``code related``. Escaping is sometimes needed, use a backslash if you want
to show e.g. \*, \\, \`.

Always use double backticks if you want to refer to a class or function, like:
``void foo()``

.. code:: rst

    Inline markup should go like this: *emphasis*, **strong emaphasis**,
    ``code related``. Escaping is sometimes needed, use a backslash if you want
    to show e.g. \*, \\, \`.

    Always use double backticks if you want to refer to a class or function, like:
    ``void foo()``

Lists
=====

* Bulleted item
* Bulleted item

  * Nested bulleted item (watch the blank line)
  * Nested bulleted item

* Bulleted item


#. Numbered item
#. Numbered item

   #. Nested numbered item (watch the blank line)
   #. Nested numbered item

#. Numbered item 3


1. Explicit item
2. Explicit item

   a. Nested explicit item
   b. Nested explicit item

3. Explicit item

.. code:: rst

    * Bulleted item
    * Bulleted item

      * Nested bulleted item (watch the blank line)
      * Nested bulleted item

    * Bulleted item


    #. Numbered item
    #. Numbered item

       #. Nested numbered item (watch the blank line)
       #. Nested numbered item

    #. Numbered item 3


    1. Explicit item
    2. Explicit item

       a. Nested explicit item
       b. Nested explicit item

    3. Explicit item

Links
=====

Use `Link text <http://xfel.eu>`_ for inline web links.

Internal links should look like this ``:ref:`framework/howto_document```,
which refers to the own section.

.. code::

    Use `Link text <http://xfel.eu>`_ for inline web links.

    Internal links should look like this :ref:`framework/howto_document`,
    which refers to the own section.


Special Directives
==================

.. topic:: Topic title

    This is a topic. Something that highlights in a box.

.. seealso::

    This is a seealso.

.. note::

    This is a note.

.. warning::

    This is a warning.

.. todo::

    This is a todo note.

.. ifconfig:: includeDevInfo is True

    This is information that is very detailed and can be switched off during
    rendering.

.. code:: rst

    .. topic:: Topic title

       This is a topic. Something that highlights in a box.

    .. seealso::

       This is a seealso.

    .. note::

       This is a note.

    .. warning::

       This is a warning.

    .. todo::

       This is a todo note.

    .. ifconfig:: includeDevInfo is True

       This is information that is very detailed and can be switched off during
       rendering.

Tables
======

Simple tables are formatted like so:

=====  =====  =======
  A      B    A and B
=====  =====  =======
False  False  False
True   False  False
False  True   False
True   True   True
=====  =====  =======

.. code:: rst

    =====  =====  =======
      A      B    A and B
    =====  =====  =======
    False  False  False
    True   False  False
    False  True   False
    True   True   True
    =====  =====  =======

Complex tables are formatted like so:

+------------------+-----------------------------------------------------------+
|**Title 1**       |**Title 2**                                                |
+------------------+-----------------------------------------------------------+
|Some entry        |Single row                                                 |
+------------------+-----------------------------------------------------------+
|Some other entry  |Split row                                                  |
|                  +-----------------------------------------------------------+
|                  |Split row                                                  |
+------------------+-----------------------------------------------------------+

.. code::

    +------------------+-------------------------------------------------------+
    |**Title 1**       |**Title 2**                                            |
    +------------------+-------------------------------------------------------+
    |Some entry        |Single row                                             |
    +------------------+-------------------------------------------------------+
    |Some other entry  |Split row                                              |
    |                  +-------------------------------------------------------+
    |                  |Split row                                              |
    +------------------+-------------------------------------------------------+

Code
====

Code blocks are initiated by

.. code-block:: Python

   @Slot
   def foo(self):
       """Does nothing"""
       pass

.. code-block:: C++

   KARABO_REGISTER_SLOT(foo);
   void foo() {
       // Does nothing
   }

.. code:: rst

    .. code-block:: Python

   @Slot
   def foo(self):
       """Does nothing"""
       pass

.. code-block:: C++

   KARABO_REGISTER_SLOT(foo);
   void foo() {
       // Does nothing
   }

Images and Figures
==================

To add an image use:

.. code:: rst

    .. image:: _images/darthvader.jpg

    .. figure:: _images/xfel.jpg
        :align: center
        :height: 100px
        :alt: alternate text
        :figclass: align-center


Figures are like images but with a caption

.. code:: rst

    .. image:: _images/darthvader.jpg

    .. figure:: _images/xfel.jpg
        :align: center
        :height: 100px
        :alt: alternate text
        :figclass: align-center

        Figures are like images but with a caption


.. _adding_graphs:

Graphs
======

Drawing of graphs is also supported:

Examples
--------

.. graphviz::

   digraph {
      "From" -> "To";
   }

.. code:: rst

    .. graphviz::

       digraph {
          "From" -> "To";
       }

.. digraph:: example

   "device1" [shape=circle, style=filled, fillcolor=green]
   "device2" [shape=circle, style=filled, fillcolor=orange]
   "broker"  [shape=box, height=2, style=filled, fillcolor=gray]

   "device1" -> "broker"
   "device2" -> "broker"

.. code:: rst

    .. digraph:: example

       "device1" [shape=circle, style=filled, fillcolor=green]
       "device2" [shape=circle, style=filled, fillcolor=orange]
       "broker"  [shape=box, height=2, style=filled, fillcolor=gray]

       "device1" -> "broker"
       "device2" -> "broker"

Math
====

.. math::

    n_{\mathrm{offset}} = \sum_{k=0}^{N-1} s_k n_k

.. code:: rst

    .. math::

        n_{\mathrm{offset}} = \sum_{k=0}^{N-1} s_k n_k


Footnotes
=========

Some text that requires a footnote [#f1]_ .

.. rubric:: Footnotes

.. [#f1] Text of the first footnote.

.. code:: rst

    Some text that requires a footnote [#f1]_ .

    .. rubric:: Footnotes

    .. [#f1] Text of the first footnote.

