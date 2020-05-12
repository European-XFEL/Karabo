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
- Vector of strings can be edited in an line-edit widget. For in this widget
the comma is interpreted as a string separator.
If one needs to input strings containing commas, one should use the edit button.
This shortcoming is outweighed by the usefulness of being able to edit a
vector of strings without a dialog.
An empty string is interpreted as a vector of 0 elements. If one needs a vector
with an empty string, they can use the edit button on the gui.