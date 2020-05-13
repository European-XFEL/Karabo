.. _karaboKnownIssues:

************
Known Issues
************

In this chapter known issues of the karabo framework are listed. These
known issues are expected!

C++ API
=======

- To be filled.

Bound Python API
================

- To be filled.

Middlelayer API
===============

- The MDL API cannot allow the setting of ``Attributes`` on runtime in elements which are in ``Nodes``.
- The alarms for the ``globalAlarmCondition`` are not latching!
  Hence, acknowledging of these alarms is not possible.


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
- Common ``Nodes`` do not have a direct value, but are containers. From the
  graphical user interface, if a property within a ``Node`` is changed
  in the ``Configurator`` (GUI), the ``Node`` will not reflect a CHANGING
  (blue) background.
- Vector Edits: The karabo GUI currently does not validate the minimum and maximum value
  input elements of the respective types (int8, uint8). Setting a non-conform value might result in an
  overflow. Be careful when reconfiguring values!
