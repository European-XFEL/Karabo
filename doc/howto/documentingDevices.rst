
.. _deviceDocumentation:

************************
How to document a device
************************

Documentation Syntax
====================

Karabo uses `Sphinx <http://www.sphinx-doc.org/en/1.3.5/>`_
for generating documentation. Please refer to the
`reStructuredText Primer <http://www.sphinx-doc.org/en/1.3.5/rest.html>`_
for more details about the markup available with Sphinx.


Automatically generated documentation structure
===============================================

When starting a new device project using the ``karabo`` script, a documentation
directory will be automatically generated when using the minimal python device
template.

If you have an existing device project which you would like to document or you
want to create a device project without using the ``karabo`` script, you can
use the ``sphinx-quickstart`` program to set up your documentation. See the
`documentation for sphinx-quickstart <http://www.sphinx-doc.org/en/1.3.5/invocation.html#invocation-of-sphinx-quickstart>`_
for more details.


Building Documentation
======================

The documentation directory contains a Makefile which invokes the ``sphinx-build``
program to generate a nicely formatted version for reading.

From inside your device's documentation directory run:

.. code-block:: shell

    $ # You most likely want the documentation as a hyperlinked web page
    $ make html

Sphinx can generate many other formats as well (epub, latex, pdf, etc.). To get
a list of available output formats run the following command:

.. code-block:: shell

    $ make help
