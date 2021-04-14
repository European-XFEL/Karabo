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
- Bound API does not support CHAR_ELEMENT, (U)INT8_ELEMENT, (U)INT16_ELEMENT

Middlelayer API
===============

- The MDL API cannot set ``Attributes`` on runtime in elements which are in ``Nodes``.

Tools
=====

The packaging management handled with the ``karabo`` script is rudimentary and has the following shortcomings:

- The package names are case insensitive on the default package repository (XFEL-internal gitlab).
  This is not a limitation of the tool per se but a limitation of the repository server.

Karabo GUI
==========

- Renaming a macro file will not shutdown the instantiated instances. This is
  expected behavior and will not be prevented.
- Karabo Scene Links are not synchronized with their target name. Renaming a
  target scene will not change the name stored in the scene link.
- Vector of strings can be edited in an line-edit widget. For in this widget
  the comma is interpreted as a string separator.
  If one needs to input strings containing commas, one should use the edit button.
  Note that the line-edit widget will split the values with commas again when
  it is edited. This shortcoming is outweighed by the usefulness of being able
  to edit a vector of strings without a dialog.
- An empty string is interpreted as a vector of 0 elements. If one needs a vector
  with an empty string, they can use the edit button on the gui.
- Common ``Nodes`` do not have a direct value, but are containers. From the
  graphical user interface, if a property within a ``Node`` is changed
  in the ``Configurator`` (GUI), the ``Node`` will not reflect a CHANGING
  (blue) background.
- It can happen that devices crash and do not send their ``instanceGone`` signal
  to the gui servers. Hence, in rare situations, devices are visible in the
  GUI client although they are not online.
- When all instances of a device class has been shutdown/removed, the class is
  not removed from the topology. This behavior is opposed to device classes
  without instances not being shown upon loading.
