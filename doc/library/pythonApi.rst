The Karabo API
==============

Karabo has an extensive API, both in C++ and Python.

The API for communication with remote devices
---------------------------------------------

.. automodule:: karabo.middlelayer_api.device_client
   :members:


The API of the GUI
------------------

This is mostly for writing widgets.

.. automodule:: karabo_gui.widget

.. automodule:: karabo_gui.schema


The internal API
----------------

The main datastructure when transporting data over the network or
storing them to file is the Karabo hash.

.. automodule:: karabo.middlelayer_api.hash
   :members:

.. automodule:: karabo.middlelayer_api.enums
   :members:
