The Karabo API
==============

Karabo has an extensive API, both in C++ and Python.

The API for communication with remote devices
---------------------------------------------

.. automodule:: karabo.api2.device_client
   :members:


The API of the GUI
------------------

This is mostly for writing widgets.

.. automodule:: widget

.. automodule:: schema


The internal API
----------------

The main datastructure when transporting data over the network or
storing them to file is the Karabo hash.

.. automodule:: karabo.api2.hash
   :members:

.. automodule:: karabo.api2.enums
   :members:
