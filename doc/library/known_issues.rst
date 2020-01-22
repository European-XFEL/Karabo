.. _karaboKnownIssues:

************
Known Issues
************

In this chapter known issues of the karabo framework are listed. These
known issues are expected of not immediately or never being fixed.

C++ API
=======

- To be filled.

Bound Python API
================

- To be filled.

Middlelayer API
===============

- The MDL API cannot allow the setting of ``Attributes`` on runtime in elements
which are in ``Nodes``.

Karabo GUI
==========

- Renaming a macro file will not shutdown the instantiated instances. This is
expected behavior and will not be prevented.
- Karabo Scene Links are not synchronized with their target name. Renaming a
target scene will not change the name stored in the scene link.