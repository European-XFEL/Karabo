
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
directory will be automatically generated when using the minimal Python device
template.

.. code-block:: shell

    $ # Generate a new Python device
    $ ./karabo new myNewDevice testDevices pythonDevice minimal MyNewDevice

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

    $ # Show all the build options...
    $ make help
      Please use 'make <target>' where <target> is one of
        html       to make standalone HTML files
        dirhtml    to make HTML files named index.html in directories
        singlehtml to make a single large HTML file
        pickle     to make pickle files
        json       to make JSON files
        htmlhelp   to make HTML files and a HTML help project
        qthelp     to make HTML files and a qthelp project
        devhelp    to make HTML files and a Devhelp project
        epub       to make an epub
        latex      to make LaTeX files, you can set PAPER=a4 or PAPER=letter
        latexpdf   to make LaTeX files and run them through pdflatex
        latexpdfja to make LaTeX files and run them through platex/dvipdfmx
        text       to make text files
        man        to make manual pages
        texinfo    to make Texinfo files
        info       to make Texinfo files and run them through makeinfo
        gettext    to make PO message catalogs
        changes    to make an overview of all changed/added/deprecated items
        xml        to make Docutils-native XML files
        pseudoxml  to make pseudoxml-XML files for display purposes
        linkcheck  to check all external links for integrity
        doctest    to run all doctests embedded in the documentation (if enabled)
