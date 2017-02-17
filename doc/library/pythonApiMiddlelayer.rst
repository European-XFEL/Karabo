Python Middlelayer API
======================

.. automodule:: karabo.middlelayer

Accessing remote devices with proxies
-------------------------------------

The core functionality of the middle layer is to access other devices.
This can also be done from the command line. Accessing other devices
is done via *proxies*:

.. autoclass:: karabo.middlelayer.Proxy()

.. autofunction:: karabo.middlelayer.getDevice(deviceId)

.. autofunction:: karabo.middlelayer.connectDevice(deviceId)

.. autofunction:: karabo.middlelayer.updateDevice

.. autofunction:: karabo.middlelayer.isAlive

Accessing remote devices directly
---------------------------------

There is also a second API without proxies to access remote devices.
This is useful if many devices have to be accessed for only a short
interaction, that does not warrant the overhead of creating a proxy.
It should be noted that most of that overhead is to inquire the
current state of devices. This is why there is no non-proxy version
to inspect the state of a remote device, as for this it is more
efficient to create a proxy.

.. autofunction:: karabo.middlelayer.setWait

.. autofunction:: karabo.middlelayer.setNoWait

.. autofunction:: karabo.middlelayer.execute

.. autofunction:: karabo.middlelayer.executeNoWait


Running devices on servers
--------------------------

Devices can be instantiated and shut down on other device servers.

.. autofunction:: karabo.middlelayer.instantiate

.. autofunction:: karabo.middlelayer.instantiateNoWait

.. autofunction:: karabo.middlelayer.shutdown

.. autofunction:: karabo.middlelayer.shutdownNoWait


Inspecting device servers
-------------------------

The command line, and devices which inherit
:class:`~karabo.middlelayer.DeviceClientBase`
keep track of all other running devices:

.. autofunction:: karabo.middlelayer.getDevices

.. autofunction:: karabo.middlelayer.getServers

.. autofunction:: karabo.middlelayer.getClasses


Writing a device
----------------

.. autoclass:: karabo.middlelayer.Configurable
   :members:

.. autoclass:: karabo.middlelayer.Device
   :members:

.. autoclass:: karabo.middlelayer.DeviceClientBase

.. autoclass:: karabo.middlelayer.Injectable
   :members:

.. autoexception:: karabo.middlelayer.KaraboError


Synchronization
---------------

.. autofunction:: karabo.middlelayer.background

.. autofunction:: karabo.middlelayer.gather

.. autofunction:: karabo.middlelayer.sleep

.. autoclass:: karabo.middlelayer.KaraboFuture
   :members:

.. autoclass:: karabo.middlelayer.lock


Karabo descriptors
------------------

.. autoclass:: karabo.middlelayer.Descriptor()


The atomic Karabo descriptors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. inheritance-diagram:: ComplexDouble ComplexFloat Double Float Int16 Int32 Int64 Int8 UInt16 UInt32 UInt64 UInt8 Bool VectorDouble VectorFloat VectorInt16 VectorInt32 VectorInt64 VectorInt8 VectorUInt16 VectorUInt32 VectorUInt64 VectorUInt8 VectorComplexDouble VectorComplexFloat VectorBool String VectorString VectorChar Char
   :parts: 1


Descriptor categories
~~~~~~~~~~~~~~~~~~~~~

The atomic data types are grouped by several categories. They are
abstract classes only.

.. autoclass:: karabo.middlelayer.Type()

.. autoclass:: karabo.middlelayer.Vector()

.. autoclass:: karabo.middlelayer.NumpyVector()

.. autoclass:: karabo.middlelayer.Simple()

.. autoclass:: karabo.middlelayer.Number()

.. autoclass:: karabo.middlelayer.Integer()

.. autoclass:: karabo.middlelayer.Enumable()

The atomic Karabo descriptors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following descriptors represent the atomic data types that Karabo
can handle:

.. autoclass:: karabo.middlelayer.String()

.. autoclass:: karabo.middlelayer.VectorChar()

.. autoclass:: karabo.middlelayer.ComplexDouble()

.. autoclass:: karabo.middlelayer.ComplexFloat()

.. autoclass:: karabo.middlelayer.Double()

.. autoclass:: karabo.middlelayer.Float()

.. autoclass:: karabo.middlelayer.Int16()

.. autoclass:: karabo.middlelayer.Int32()

.. autoclass:: karabo.middlelayer.Int64()

.. autoclass:: karabo.middlelayer.Int8()

.. autoclass:: karabo.middlelayer.UInt16()

.. autoclass:: karabo.middlelayer.UInt32()

.. autoclass:: karabo.middlelayer.UInt64()

.. autoclass:: karabo.middlelayer.UInt8()

.. autoclass:: karabo.middlelayer.Bool()

.. autoclass:: karabo.middlelayer.VectorDouble()

.. autoclass:: karabo.middlelayer.VectorFloat()

.. autoclass:: karabo.middlelayer.VectorInt16()

.. autoclass:: karabo.middlelayer.VectorInt32()

.. autoclass:: karabo.middlelayer.VectorInt64()

.. autoclass:: karabo.middlelayer.VectorInt8()

.. autoclass:: karabo.middlelayer.VectorUInt16()

.. autoclass:: karabo.middlelayer.VectorUInt32()

.. autoclass:: karabo.middlelayer.VectorUInt64()

.. autoclass:: karabo.middlelayer.VectorUInt8()

.. autoclass:: karabo.middlelayer.VectorComplexDouble()

.. autoclass:: karabo.middlelayer.VectorComplexFloat()

.. autoclass:: karabo.middlelayer.VectorBool()

.. autoclass:: karabo.middlelayer.VectorString()

.. autoclass:: karabo.middlelayer.Char()

Compound descriptors
~~~~~~~~~~~~~~~~~~~~

.. autoclass:: karabo.middlelayer.Node()

.. autoclass:: karabo.middlelayer.VectorHash()


Special descriptors
~~~~~~~~~~~~~~~~~~~

.. autoclass:: karabo.middlelayer.DeviceNode

Karabo data types
-----------------

.. autoclass:: karabo.middlelayer.KaraboValue()

.. autofunction:: karabo.middlelayer.isSet

.. autoclass:: karabo.middlelayer.QuantityValue()

.. autoclass:: karabo.middlelayer.StringValue()

.. autoclass:: karabo.middlelayer.VectorStringValue()

.. autoclass:: karabo.middlelayer.TableValue()

.. autoclass:: karabo.middlelayer.EnumValue()

.. autoclass:: karabo.middlelayer.BoolValue()

.. autoclass:: karabo.middlelayer.NoneValue()

.. autoclass:: karabo.middlelayer.Timestamp()
   :members:
