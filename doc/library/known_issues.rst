.. _karaboKnownIssues:

************
Known Issues
************

In this chapter known issues of the karabo framework are listed. These
known issues are expected!

C++ API
=======

- The ``FileDataLogReader`` will start indexing a property on property history
  request. The first request will receive a remote exception. Any subsequent
  request until the indexing is complete will result in an empty set.

- Data logged into InfluxDB may take a while to be available for reading. The
  availability interval depends on the writing load on the InfluxDB
  infra-structure. This is not a Karabo issue, and maybe observed from other
  InfluxDB clients as well (e.g. Grafana). During the phase of migration of
  data from the legacy system to the one based on InfluxDB at European XFEL,
  writing loads way beyond normal usage ones were handled by the InfluxDB
  infra-structure. In such a scenario, intervals in the order of minutes could
  be observed for the data to be ready for reading.


Bound Python API
================

- Failure on instantiation generates an exception in all 3 APIs.
  In the Bounnd Pyhon API, this exception is not reported to the caller.

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
