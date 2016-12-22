*****************
DOOCS Integration
*****************

DOOCS Bridge
============

Karabo devices may bridge to DOOCS servers through the ``doocsapi`` Karabo dependency. 
In this scenario Karabo clients interface to the Karabo bridge device, which in turn 
translates property requests, assignments and command calls to the DOOCS server. 
The API supports DOOCS ``get``, ``set`` and ``monitor`` methods, which have the following
behaviour:

.. function:: get(propertyName)

   executed by a Karabo client will trigger the Karabo bridge device to request a property
   from the DOOCS server, then update the Karabo device's respective property.
   
.. function:: set(propertyName, Value)

   executed by a Karabo client will trigger the Karabo bridge device to assign a value
   to the property identified by ``propertyName`` on the DOOCS server, and then reflect
   the successful set in the Karabo device's property.
   
.. function:: monitor(propertyName)

   executed on the Karabo client will trigger the Karabo bridge device to request event
   driven updates of the property identified by ``propertyName`` from the DOOCS server.
   The Karabo bridge device's property will then be updated in an event-driven fashion.
   
.. note::

	The ``doocsapi`` is available both in the Python and C++ driver APIs.
	
.. todo::

	Sergey should write an example for both the C++ and Python APIs and verify that
	the DOOCS bridge still works.
	
Karabo Clients
==============

Karabo Clients should talk to DOOCS devices via a `DOOCS Bridge`_  device. With respect to
these devices all other concepts and policies for Karabo apply.

DOOCS Clients
=============

A Java interface (``jKarabo``) for the Karabo GuiServer exists which can be integrated on DOOCS side.
By using this library DOOCS clients should be able to directly access Karabo devices.
