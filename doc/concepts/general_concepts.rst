..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

****************
General Concepts
****************


Devices
=======

The device is the core concept of Karabo. Basically, Karabo consists of a set of devices
running somewhere on the network. Every device is an instance of an object-oriented class
which implements the device interface and logic. Each device serves one logical
and encapsulated service, i.e. 1 device = 1 service.

A device can represent:

- A single IO channel (e.g. a digital output for switching something on or off)
- A single piece of equipment (e.g. a motor or a pump)
- A controller driving a set of equipment (e.g. a pump controller)
- A group of equipment that together forms a larger component (e.g. a slit using two
  underlying motors)
- A software algorithm (e.g. image processing)
- A connection to a service, file system or database (e.g. data archive reader,
  calibration database adapter)

The main purpose of devices is to hide the implementation details of the underlying
service from the user and provide a standardized interface to the outside world.

In order to unambiguously address a device running somewhere in the network, each device
is identified by a unique name, its device id. A Karabo distributed system will not allow a second device
to be started with an instance name that already exists somewhere in its managed topology.

Device instance ids are strings that must not be empty.
Allowed characters are upper or lower case letters of the English alphabet,
digits, or the special characters '_', '/' and '-'.
Preferrably, ids have three parts *domain*, device *type* and *member*
separated by '/':

.. code-block:: XML

   <domain>/<type>/<member>

Device Implementation
=====================


As previously mentioned devices are *classes*, with *methods* and *properties*. All devices
inherit from a base class in the respective API, ensuring that a common core functionality
in terms of inter-device communication, data types, self-description and logging is
provided.

Device Slots
++++++++++++

*Device slots* can conceptually be seen as member functions of a C++ or Python
class which are additionally exposed to all other devices in the control
system. Slots may be called with up to four arguments of the types described in
Section :ref:`data_types` (although many more are possible using a Hash as a container).
They may have zero to four return values of the a Karabo-known types.

Slots that are part of the device self-description and are thus exposed to the
graphical user interface, do not take arguments.
As *commands* they should return the *state* that the device is in after slot
execution.



The Call & Request/Reply Patterns
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

At its core Karabo uses a combination of signals and slots to provide for (inter-)device
communication. This *low-level* interface can be directly used if a large degree of message
passing patterns and the (a)synchronicity of events is needed. In the C++ API and Bound API it is
exposed as part of the Device interface.
If an exception occurs during execution of a slot on the remote device,
an exception will be thrown in case of synchronous operations. In the
asynchronous case, one can specify a failure handler in addition to the normal
handler.

In the simplest case a device method is called (possibly from another device) and any
return value is not expected. This is the *call* pattern.

.. code-block:: python

		class RemoteDevice(PythonDevice):
			...
			def __init__(self, configuration):
			        super(RemoteDevice, self).__init__(configuration)
				self.KARABO_SLOT(self.foo)

			def foo(self, a):
				self.log.INFO(a)

		# code on caller
		self.call("a/remote/device", "foo", 1)

.. note::

	A special case of the call pattern is the global call. The idea is to call a specific
	slot function irrespective of the device that carries it. This is expressed by using
	a "*" instead of a specific device name. Global calls should not be used in device
	code but are mentioned here for completeness.

The call follows a fire-and-forget mentality and any potential reply statement on the
remote function will be ignored and not sent back to the callee.
Neither are any failures reported like non-existence of the called device or
slot. Calling a remote slot will
never block the caller.

If return values are expected the *request and reply pattern* is used. A request to a
method may be called in two different ways:

- **synchronously, as a direct call:** the caller will block until the method
  execution returns or fails by throwing an exception. A timeout may be configured
  if a *reply* is expected.

	.. code-block:: python

		class RemoteDevice(PythonDevice):
			...
			def initialization(self):
				self.KARABO_SLOT(self.bar)

			def bar(self, b):
				c = b + 1
				#this is a slot which should send out a reply
				self.reply(c)

		# code on caller
		result = self.request("/a/remote/device", "bar", 1).waitForReply(1000)


- **asynchronously, with callback:** the call to the method directly returns to the
  caller. Upon completion of the call the callback is executed (in a separate thread)
  and any return values are supplied as arguments.

	.. code-block:: python

		class RemoteDevice(PythonDevice):
			...
			#as before

		#code on caller
		def onBar(self, response):
			self.log.INFO(response)

		self.request("a/remote/device", "bar, 2).receiveAsync(self.onBar)


In C++ the syntax is slightly different and the callbacks are bound in runtime,
using ``karabo::util::bind_weak``:

.. code-block:: c++

    string txt("The answer is: ");
    request("some/device/1", "slotFoo", 21)
	.receiveAsynce<int>(bind_weak(&onReply, this, txt, _1),
                            bind_weak(&onError, this));

    void onReply(const std::string& arg1, int arg2) {
        std::cout << arg1 << arg2 << std::endl; // Prints: "The answer is: 42"
    }

    void onError() {
        try {
		throw;
	} catch (const std::exception& e) {
		std::cout << "An error occurred when calling 'slotFoo': "
		<< e.what() << std::endl;
	}
    }

    // Replying instance ("some/device/1"):
    void slotFoo(const int arg1) {
        reply(arg1 + arg1);
    }


.. note::

   Using ``karabo::util::weak_bind`` ensures that while the callback is being
   executed it is protected from destruction of ``this``, while at the same
   time a bound but not executed callback will not prevent destruction of
   ``this``.

A **signal** can directly be used to initiate action: the method is
attached to a signal and is executed when this signal is emitted. This is especially
useful if the update of a parameter should trigger different actions on multiple devices
with multiple methods.

.. code-block:: python

    class RemoteDevice(PythonDevice)
        ...
        def initialization(self):
            self.registerSignal("foo", int)

        def bar(self):
            self.emit("foo", 1)

    class Receiver1(Python):
        ...
        def initialization(self):
            self.KARABO_SLOT(self.onFoo)
            self.connect("remote/device/1", "foo", "", "onFoo")

        def onFoo(self, a):
            self.log.INFO(a)

    class Receiver2(Python):
        ...
        def initialization(self):
            self.KARABO_SLOT(self.onBar)
            self.connect("remote/device/1", "foo", "", "onBar")

        def onBar(self, b):
            self.log.INFO(b+1)


Technical Implementation
~~~~~~~~~~~~~~~~~~~~~~~~
Every device is subscribed as a client to a central message broker. All devices subscribe
with their device names. The broker uses these names for message routing during the
request / reply communication. The requesting instance generates a unique ID for each
request, which is shipped with the message and is used for blocking and unblocking or
registering and finding a provided callback, respectively.

Device Properties
+++++++++++++++++

.. note::
   The below writing addresses the C++ and Bound APIs,
   property access is simplified in *middle-layer* devices.

Device properties are the equivalent to public members in C++ or properties in Python,
i.e. they are class member variables which you would like to expose to the outside world,
or in the context of a distributed control system, to other devices. In the Tango world
they directly correspond to attributes; in the DOOCS world they correspond to properties.

In Karabo they are defined statically in the so-called ``expectedParameters`` section.
Properties may be of any of the types specified in Section :ref:`data_types` and may have
received further specification using attributes. Alongside
methods, properties constitute an integral part of a device's self description, as defined
by its ``Schema``. By defining a property the following is implied

- the property is readable (*get*) and possibly writable (*set*) from within the distributed
  system using a combination of *device id* and *property key* and given the user
  has appropriate access rights.
- the combination of *device id* and *key* is unique across the distributed system installation.
- the GUI provides basic functionality for displaying the property
- the GUI provides basic functionality for altering the property
- the property is available to middle-layer devices and macros via proxies
- the property can be serialized in Karabo's serialization and DAQ formats.

.. ifconfig:: includeDevInfo is True

	.. note::

		For framework developers it is important that the listed implications are
		seen as absolute requirements. This means that adding any basic data type to the
		framework implies that a GUI display solution (or a graceful failover option)
		is provided alongside.


Properties can be any of the Karabo data types described in Section `Karabo Data Types`_.
They are defined in the so-called *expected parameters* definition of a device and are
known to the system at static time.

.. code-block:: Python

	@staticmethod
	def expectedParameters(expected):

	    (
	        STRING_ELEMENT(expected).key("stringProperty")
	            .displayedName("A string property")
	            .assignmentMandatory()
	            .commit()
	            ,
	        UINT32_ELEMENT(expected).key("integerProperty")
	            .displayedName("An integer property")
	            .assignmentOptional().defaultValue(1)
	            .commit()
	            ,
	    )

As shown in the code, properties are defined by creating an *element*, identified by the
Karabo type with the suffix *_ELEMENT*. They need to be given a unique key, and may be
further specified through attributes.

Node Elements
+++++++++++++

Karabo allows grouping of properties into hierarchical tree structures. This is done using
*node* elements. A node element can be seen as an intermediate component in the
path uniquely identifying a property. It is a natural consequence of allowing nested Hash
structures. Accordingly, requesting the value of a node element will return a Hash with
the node's inner elements as members.

A device may give different options on which kind of node to use, this
is called a *choice of nodes* element::


    @staticmethod
    def expectedParameters(expected):
        (
            CHOICE_ELEMENT(expected).key("connection")
            .appendNodesOfConfigurationBase(ConnectionBase)
            .commit()
        )


In some occasions, it may be useful to have an entire list of
different nodes, which is the *list of nodes* element. The device
programmer defines node types which can be used in this list::


    @staticmethod
    def expectedParameters(expected):
        (
            LIST_ELEMENT(expected).key("categories")
            .appendNodesOfConfigurationBase(CategoryBase)
            .commit()
        )

Device version
++++++++++++++

Each device declares in its configuration the Karabo Framework version
as well as its package version. The automation of this feature allows to
seamlessly store the software configuration in the logging system.

See the respective API sections on examples of how this is done for the C++ and
python APIs.


Device Hooks
++++++++++++

Karabo devices provide a set of common hooks in both the Python and C++ APIs (but not
the middle-layer API). Developers can use these hooks to trigger special functionality
on events common to all devices. They are as follows:

- *preReconfigure(incomingReconfiguration)*: allows an incoming re-configuration to the
  device to be altered *before* actually updating device properties. This hook can be used
  to perform more sophisticated validity checks or to alter the configuration before
  its application. The configuration is passed as a Karabo Hash which contains all
  altered properties.

  .. note::

  	The incoming configuration can contain one to many altered properties, depending on
  	whether *apply* or *apply all* was executed from the GUI.

- *postReconfigure*: this parameterless hook is called *after* a new configuration has
  been applied. One can use this hook to perform some action on hardware after
  configuration has been validated and set.

- *preDestruction*: this parameterless hook is executed before a device instance is
  destroyed. You should use it to clean up, close any open sockets or connections or
  possible bring the hardware back into a specified safe state.

- *onTimeUpdate(trainId, sec, frac, period)*: is executed when the device receives an
  update from the timing system.

  - The `registerInitialFunction(func)` method can be used to register a
    function to be called at the end of device initialization, i.e. after the device
    properties' initial values have been set and are available through the *get* and *set*
    methods. Usually, this function should bring the device into an initial known state.

Events vs. Polling on bound devices
====================================

In the context of *bound* devices Karabo imposes no restrictions if values from hardware
are introduced into the distributed control system in an event-driven fashion or through
polling. Hardware interaction may thus occur via the hardware sending event messages via a
defined channel, i.e. an open socket, to the device, possibly with a PLC system mediating
between both sides, or by actively polling the hardware on an interface or connection
at a predefined update interval.

In either case new values (from the hardware) are made available to the distributed system
in a standardized fashion by assigning (setting) to the corresponding property, defined
as an expected parameter. Possibly, some sort of computation has occurred prior to this,
e.g. if a histogram is computed from digitizer output and the individual samples are not
further used.

Assigning to a property is an atomic, blocking operation, i.e. the rest of the distributed
system is only made aware of the property change if the assignment succeeded. Similarly,
retrieval of a property value is an atomic, blocking operation: during retrieval it is
guaranteed that the current value is not altered by an assignment operation.

.. note::

	This does not mean that there may not be a more up-to-date value available from the
	hardware. It only means that the distributed system returns the most current value
	it is aware of.


A device polling hardware should usually implement its own worker thread as is shown
in the following code example.

.. code-block:: Python

    from karabo.bound.worker import Worker
    from karabo.bound.decorators import KARABO_CLASSINFO
    from karabo.bound.device import PythonDevice, launchPythonDevice
    from ._version import version as deviceVersion

    @KARABO_CLASSINFO("HardwarePollingDevice", deviceVersion)
    class HardwarePollingDevice(PythonDevice):

        def __init__(self, configuration):
            super(HardwarePollingDevice).__init__(self, configuration)
            self.pollWorker = None
            self.registerInitialFunction(self.initialization)

        def preDestruction(self):
            if self.pollWorker is not None:
                if self.pollWorker.is_running():
                    self.pollWorker.stop()
                self.pollWorker.join()
                self.pollWorker = None

        @staticmethod
        def expectedParameters(expected):
            (
                INT32_ELEMENT(expected).key("polledValue)
                    .readOnly().noInitialValue()
                    .commit()
                    ,
                    ...
            )

        def initialization(self):
            if self.pollWorker is None:
                # Create and start poll worker
                timeout = 1000 # milliseconds
                self.pollWorker = Worker(self.pollingFunction, timeout, -1).start()

        def pollingFunction(self)
            #do something useful
            .....
            self.set("polledValue", value)



Synchronous and Asynchronous Communication via the Client Interface
===================================================================

As was mentioned in the `Device Slots`_ section, Karabo devices support two types
of calls to slots on devices: synchronous calls and asynchronous calls on the lower-level
signal-slot interface. Often such a detailed level of control over (a)synchronicity
of communication is not needed. In such cases the *DeviceClient* interface can be used.
The device client is accessible using the ``remote()`` function:

.. code-block:: Python

	self.remote().execute("/a/remote/device", "foo", 1)

will block on the caller until the call either returns or fails by throwing an exception,
the latter could e.g. happen if you called to a wrong id, gave the wrong type or number of
arguments or there was a problem with the network connection. Optionally, you can specify
a timeout as last parameter, after which an exception is thrown if the call has not
completed by then.

In contrast,

.. code-block:: Python

	self.remote().executeNoWait("/a/remote/device", "foo", 1)

will directly return to the caller if no exception is thrown. Similarly, you can alter
properties on a remote device using

.. code-block:: Python

	self.remote().set("/a/remote/device", "A", 1)
	self.remote().setNoWait("/a/remote/device", "B", 2)

and retrieve them

.. code-block:: Python

	self.remote().get("/a/remote/device", "A")

If you depend on executing some code whenever a property on a device changes *property
monitors* come into use. They allow you to register a callback to be executed whenever the
property changes:

.. code-block:: Python

	def myCallBack(self, a, timestamp):
	    self.log.INFO("Value has changed: {} at {}".format(a,t))

	self.remote().registerPropertyMonitor("/a/remote/device", "A", self.myCallBack)

Callbacks can also be registered to receive notifications if a device has generally
changed, i.e. its properties or state were altered:

.. code-block:: Python

	def myCallBack(self, a, timestamp):
	    #do something useful
	    ...

	self.remote().registerDeviceMonitor("/a/remote/device", "A", self.myCallBack)

.. note::

	While communication via the client interface offers some degree of convenience for
	*bound* device development, it is recommended that such devices which do not need
	low-level event control are programmed in the middle-layer API instead, where a more
	concise interface for the client functionality as just described is available.

.. _general_concepts_simple_state_machine:

The Simple State Machine
========================

All device APIs in Karabo provide state-awareness via so-called *simple state machines*.
The underlying assumption is that for (bound) devices, where strict state transition rules
need to be enforced, these will have been implemented in hardware or in firmware on PLCs.
Bound devices thus need to be able to follow or reflect the hardware state, but not enforce
strict state transition rules. In other words: state-violating input to the hardware is
caught by the hardware, preserving hardware safety, not by the software device.

Leveraging this policy software state handling can be more relaxed: slots are state aware in
that it can be defined for which states they may be executed, but no transition rules need
to be enforced. Instead state transition is programmatically driven using

.. code-block:: Python

	def expectedParameters(expected)
	    (
	        SLOT_ELEMENT(expected).key("start")
	            .displayedName("Start")
	            .allowedStates([States.STOPPED, States.IDLE])
	            .commit()
	            ,
	        SLOT_ELEMENT(expected).key("stop")
	            .displayedName("Stop")
	            .allowedStates(States.MOVING)
	            .commit()
	            ,
	    )

	#...

	def start(self):
	    #...
	    self.updateState(states.MOVING)

The available states are consistent across the distributed system and defined in the
*states* enumerator. Details can be found in Section :ref:`states`.


.. _data_types:

Karabo Data Types
=================

Karabo properties can have a number of common data types, ranging from simple and complex
scalars, vectors of these, as well as composite types such as arrays of arbitrary rank
and tables, i.e. 2-d arrays with a fixed column schema.

Additionally, Karabo implements a key-value container which preserves insertion order
and can be iterated over: the Karabo Hash.

Karabo datatypes "live" in the Karabo Hash. They are converted to the native types of
the programming language upon retrieval (get) from the Hash and from the native types
upon assignment to the Hash. In C++ this is explicitly done using template mechanisms,
in Python an implicit conversion is performed. Casting is supported using the ``getAs`
method:

.. code-block:: Python

	h = Hash("foo", 1) # assigned an integer to foo
	f = h.getAs("foo", float) # f is a float
	s = h.getAs("foo", str) # s is a string

	h2 = Hash("bar", "Hello World!) #assigned a string to bar
	i = h2.getAs("bar", int)
	# will raise an exception as Hello World cannot be converted to int

In C++ templating mechanisms are used:

.. code-block:: C++

	Hash h("foo", 1)
	float f = h.getAs<float>("foo")
	std::string s = h.getAs<std::string>("foo")

	Hash h2("bar", "Hello World!")
	int i = h2.getAs<int>("bar") // will throw

.. _cppHash:

The Karabo Hash
+++++++++++++++

The Karabo Hash is a key-value container. This means the (values of) elements in a Hash
can be addressed by a string key.

.. code-block:: Python

	h = Hash()
	h.set("foo", 1)
	v = h.get("foo")

Insertion order into the Hash is preserved and iteration supported:

.. code-block:: Python

	h = Hash()
	h.set("foo", 1)
	h.set("bar", 2)

	for key in h.getKeys():
	    print(key, h.get(key))

	#will print
	# foo, 1
	# bar, 2

Hash key-value pairs can have attributes assigned to them, allowing to specify e.g.
validity bounds:

.. code-block:: Python

	def checkBounds(h,k):
	    if h.hasAttribute(k, "warnLow") and h.hasAttribute(k, "warnHigh"):
	    	if h.get(k) < h.getAttribute(k, "warnLow") or \
	    	    h.get(k) > h.getAttribute(k, "warnHigh"):

	    	    raise AttributeError("Value out of bounds")

	h = Hash()
	h.set("foo", 1)
	h.setAttribute("foo", "warnLow", 0)
	h.setAttribute("foo", "warnHigh", 2)

	checkBounds(h, "foo")
	#all good
	h.set("foo", 3)
	checkBounds(h, "foo")
	#raises AttributeError

In fact bound-checking is already included in Karabo and can be enabled upon property
definition. It is implemented using attributes.

From the Python perspective a Hash corresponds to something like an ordered
``dict()`` which allows attribute assignment to each key. C++ programmers by think of it as
an ordered ``std::map``.

Finally, Hashes may contain other Hashes, adding hierarchy to the container. Values are
thus identifiable by *paths*, separated with "." characters:

.. code-block:: Python

	h1 = Hash()
	h2 = Hash("a", 1)
	h1.set("b", h2)

	h3 = Hash("c", h1)

	print(h1.get("b.a"))
	# will print 1
	print(h3.get("c.b.a"))
	# will print 1

	h3.setAttribute("c.b.a", "myAttribute", "Test")
	print(h3.getAttribute("c.b.a", "myAttribute"))
	#will print "Test"

Note that in the above examples copies of *h2* and *h1* are created upon insertion. The
following call will thus fail, as *h2* has not been set an attribute:

.. code-block:: Python

	print(h2.getAttribute("a", "myAttribute"))


.. note::

	While the above examples are Python code, having to access items of a dictionary-like
	container by key, instead of iterating over key-value pairs, seems unnecessary complex.
	In the middle-layer API a more *pythonic* solution is available using
	``Hash.iteritems()``.

.. note::

    In both Python APIs requesting a non-existing key from the Hash will return
    ``None``.


Scalar Types
++++++++++++

Karabo support the most common scalar data types:

===========================  ==============================
Boolean type:                 BOOL
Character type (raw byte):    CHAR
Signed integer types:         INT8, INT16, INT32, INT64
Unsigned integer types:       UINT8, UINT16, UINT32, UINT64
Floating point types:         FLOAT, DOUBLE
===========================  ==============================

.. note::

	There is purposely no INT or LONG type in Karabo. Depending on the host and operating
	system these type can either be 32 bits or 64 bits long, leading to ambiguity. Instead
	use the INT32 type if you need a 32 bit integer and the INT64 type if you need a
	64 bit integer. Out of similar reasons try to avoid using *size_t* for counters and
	rather use the explicit *uint64_t*, which is assured to of 64 bits length.


Complex Types
+++++++++++++

Complex types are available in Karabo. They are available for float and double
scalar and vector types described in the previous section by prepending
``COMPLEX``.

=======================  ===========================================
Complex scalar types:    COMPLEX_FLOAT, COMPLEX_DOUBLE
=======================  ===========================================

In C++ the underlying type is ``std::complex<>``, in Python the ``complex``
type is used. The following two examples are equivalent:

.. code-block:: C++

    using namespace std::complex_literals;
    std::complex<double> z1 = 1i * 1i;
    std::cout<<z1.real<<" "<<z1.imag;

.. code-block:: Python

    z1 = 1j*1j
    print(z1.real, z1.imag)




Vector Types
++++++++++++

Karabo supports vectors of all scalar and complex types as well as vectors of Hashes.
Vector types are specified by prepending ``VECTOR_`` to the scalar property name or to the
Hash:

=======================  ==========================================================
Boolean type:             VECTOR_BOOL
Signed integer types:     VECTOR_INT8, VECTOR_INT16, VECTOR_INT32, VECTOR_INT64
Unsigned integer types:   VECTOR_UINT8, VECTOR_UINT16, VECTOR_UINT32, VECTOR_UINT64
Floating point types:     VECTOR_FLOAT, VECTOR_DOUBLE
Complex vector types:     VECTOR_COMPLEX_FLOAT, VECTOR_COMPLEX_DOUBLE
Hash type:                VECTOR_HASH
=======================  ==========================================================

NDArray Types
+++++++++++++

Multidimensional types are represented using the ``NDArray`` type and the
associated ``NDARRAY_ELEMENT``. The element itself is untyped. Rather it
will always internally store data as a ``ByteArray`` alongside an attribute
for type information.

For images the ``ImageData`` and ``IMAGEDATA_ELEMENT`` build on-top of the
``NDArray``, adding additional standardized meta-data.

Both types derive from the Karabo ``Hash`` and thus can fully be serialized.
You can set and retrieve objects of these types using the standard ``get`` and
``set`` interfaces.

.. _attributes:

Attributes
++++++++++

Attributes have already briefly been introduced. In Karabo they can be used to further
specify the characteristics of a property. The can be set for any key in a Karabo Hash.

While attributes are freely assignable and may consist of all scalar, vector and complex
types, Karabo comes with a set of standardized attributes, used for common tasks such as
bound checking or defining the unit and order of magnitude of a value. These are exposed
via a dedicated interface, in addition to being accessible via the *setAttribute* and
*getAttribute* methods.

Numerical Representation
~~~~~~~~~~~~~~~~~~~~~~~~

Karabo allows to set the numerical representation of a value in the GUI.

.. code-block:: Python

	UINT8_ELEMENT(expected).key("binaryRep)
	    .displayedName("As binary")
	    .bin()
	    .assignmentOptional().defaultValue(128)
	    .commit()

	#is displayed as 0b10000000

	UINT8_ELEMENT(expected).key("hexRep)
	    .displayedName("As hex")
	    .hex()
	    .assignmentOptional().defaultValue(128)
	    .commit()

	#is displayed as 0x80

	UINT8_ELEMENT(expected).key("octalRep)
	    .displayedName("As octal")
	    .oct()
	    .assignmentOptional().defaultValue(128)
	    .commit()

	#is displayed as 0o200

The following representations are available:

=========== =====
Binary mask bin()
Hexadecimal hex()
Octal       oct()
=========== =====


Bounds & Alarm Conditions
~~~~~~~~~~~~~~~~~~~~~~~~~

Bounds may be set as inclusive or exclusive bounds indicating setting,
warning and alarm bounds and ranges. Karabo allows for setting lower (minimum)
and upper (maximum) bounds, and any set operation or property change using the
GUI will check against these before updating the property value.
Bounds are specified when defining a devices expected parameters:

.. code-block:: Python

        UINT32_ELEMENT(expected).key("bounded")
            .displayedName("Has bounds")
            .minIncl(100).maxExcl(600)
            .alarmLow(200).needsAcknowledging(True)
            .alarmHigh(500).description("Foo").needsAcknowledging(True)
            .warnLow(300).needsAcknowledging(False)
            .warnHigh(400).needsAcknowledging(False)
        .assignmentOptional().defaultValue(128)
        .commit()

        self.set("bounded", 30)  # raises exception, too low
        self.set("bounded", 100)  # works, but shows alarm
        self.set("bounded", 200)  # works, but shows alarm
        self.set("bounded", 300)  # works, but shows warning
        self.set("bounded", 350)  # just works
        self.set("bounded", 400)  # works, but shows warning
        self.set("bounded", 500)  # works, but shows alarm
        self.set("bounded", 600)  # raises exception~

Additionally, alarm conditions may be set in the variance of a parameter,
evaluated in a defined rolling window:

.. code-block:: Python

        UINT32_ELEMENT(expected).key("bounded")
            .displayedName("Has bounds")
            .warnVarHigh(10).needsAcknowledging(True)
            .alarmVarLow(10).needsAcknowledging(True)
        .assignmentOptional().defaultValue(128)
        .commit()



.. note::

   Alarm condition definitions need to always be closed of by stating if the
   alarm needs acknowledging on the alarm service to disappear.

Units
~~~~~

It is considered best practice to always assign a unit if a property represents a physical
observable. Karabo provides for assigning SI (System International) and selected derived
and historical units as property attributes. The following units are available:

=================== ========== ================= ================================================================
**Unit**            **Symbol** **Karabo**        **Used for**
unitless            --         NUMBER            Values without a clearly defined unit
count               --         COUNT             Counters, iteration variable
meter               m          METER             Length measurements, wavelength
gram                g          GRAM              Weight measurements
second              s          SECOND            Time measurement
ampère              A          AMPERE            Electrical currents
kelvin              K          KELVIN            Temperature measurements
mole                mol        MOLE              Molecular amounts
candela             cd         CANDELA           Luminous intensity
litre               l          LITRE             Volume
hertz               Hz         HERTZ             Frequency measurements
radian              rad        RADIAN            Angular distances
degree              °          DEGREE            Angular distances
steradian           sr         STERADIAN         Solid angles
newton              N          NEWTON            Force
pascal              Pa         PASCAL            Pressure
joule               J          JOULE             Energy
electron volt       eV         ELECTRONVOLT      Energy, :math:`1\,\text{eV} = 1.602176\times 10^{-19}\,\text{J}`
watt                W          WATT              Power
coulomb             C          COULOMB           Charge
volt                V          VOLT              Voltage
farad               F          FARAD             Capacity
ohm                 Ω          OHM               Resistance
siemens             S          SIEMENS           Electric conductance, admittance, susceptance
weber               Wb         WEBER             Magnetic flux
tesla               T          TESLA             Magnetic flux density
henry               H          HENRY             Inductivity
degree celsius      °C         DEGREE_CELSIUS    Temperature measurements
lumen               lm         LUMEN             Luminous flux
lux                 lx         LUX               Luminous emittance
becquerel           Bq         BECQUEREL         Radioactivity
gray                Gy         GRAY              Ionizing dose
sievert             Sv         SIEVERT           Effective dose
katal               kat        KATAL             Catalytic activity (in enzymes)
minute              min        MINUTE            Time measurement
hour                h          HOUR              Time measurement
day                 d          DAY               Time measurement
year                yr         YEAR              Time measurement
bar                 bar        BAR               Pressure measurement (consider using pascal)
pixel               px         PIXEL             Image display
byte                B          BYTE              Computer memory and storage
bit                 b          BIT               Computer memory and storage, architecture
meter per second    m/s        METER_PER_SECOND  Velocity
volt per second     V/s        VOLT_PER_SECOND   Voltage ramping
ampère per second   A/s        AMPERE_PER_SECOND Current ramping
percent             %          PERCENT           Relative quantification
=================== ========== ================= ================================================================

.. note::

	While support for some historic, non-SI units is provided, please consider using SI
	units as much as possible.

.. warning::

	While Karabo allows for specifying units it does **not** take these into
	account in any calculations: it is up to the programmer to make sure that algebra
	on different properties in compatible in terms of units and to determine the unit
	of the result!

Metric Prefixes
~~~~~~~~~~~~~~~

Frequently, it is favorable to not represent a value in SI-units, but with a multiplication
factor in powers of 10 of that unit. This is called the metric prefix and commonly
expressed by adding a prefix to the unit, e.g. 1 km, instead of 1000 m. In every day usage
we do this to not have to deal with overly large or small numbers when comparisons or
calculations are made with value which have the same order of magnitude. In terms of
computer processing there is an additional benefit: the value range of integer values is
limited, as is the precision of floating point numbers. By introducing a metric prefix
attribute we can shift values back into a specified range, without sacrificing precision:


.. code-block:: Python

	UINT8_ELEMENT(expected).key("prefixedValue")
	    .displayedName("Prefixed value")
	    .metricPrefix(MetricPrefix.MEGA)
	    .assignmentOptional().defaultValue(128)
	    .commit()


A 1B unsigned int value has a maximum value of 255. By assigning the prefix we can express
that we actually mean ::math:`128\times10^{6}`. The following metric prefixes are available
in Karabo:

========== ================= ================= ================= ================= ================= ================= ================= ================= ================= =================
**prefix** y                 z                 a                 f                 p                 n                 :math:`\mu`       m                 c                 d
**factor** :math:`10^{-24}`  :math:`10^{-21}`  :math:`10^{-18}`  :math:`10^{-15}`  :math:`10^{-12}`  :math:`10^{-9}`   :math:`10^{-6}`   :math:`10^{-3}`   :math:`10^{-2}`   :math:`10^{-1}`
**Karabo** YOCTO             ZEPTO             ATTO              FEMTO             PICO              NANO              MICRO             MILLI             CENTI             DECI
========== ================= ================= ================= ================= ================= ================= ================= ================= ================= =================

========== ================= ================= ================= ================= ================= ================= ================= ================= ================= =================
**prefix** da                h                 k                 M                 G                 T                 P                 E                 Z                 Y
**factor** :math:`10^{1}`    :math:`10^{2}`    :math:`10^{3}`    :math:`10^{6}`    :math:`10^{9}`    :math:`10^{12}`   :math:`10^{15}`   :math:`10^{18}`   :math:`10^{21}`   :math:`10^{24}`
**Karabo** DECA              HECTO             KILO              MEGA              GIGA              TERA              PETA              EXA               ZETTA             YOTTA
========== ================= ================= ================= ================= ================= ================= ================= ================= ================= =================

No prefix does not need an explicit specification but can be specified as
*MetricPrefix.NONE*. It corresponds to a multiplication by 1.

.. note::

	While Karabo allows for specifying metric prefixes it does **not** take these into
	account in any calculations: whenever you retrieve a Karabo property it is converted
	to the programming language's native type, which has no notion of prefixes! You can
	however use the *getPrefixFactor()* method to return a multiplicative factor depending
	on the assigned prefix.

.. code-block:: Python

	a = self.get("prefixedValue")*self.getPrefixFactor("prefixedValue")
	# a = 128e6 as given in the previous example

.. todo::

 	Implement the getPrefixFactor method if not already existing. I think it is needed
 	as otherwise uses would need to end up doing there on prefix-checking-multiplying
 	code all the time. Should be simple to implement by expanding the prefix enum.



Advantages of Using Units, Metric Prefixes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Adding units, metric prefixes and unit scales to values may seem like a nuisance
at first. It has two major benefits though:

- Persons not intimately familiar with a device can get a better understanding of its
  properties in a much shorter time, ambiguity of a properties meaning is avoided and
  proper understanding of critical values enforced.

- Karabo can (in the future) offer you a much richer plotting experience. Karabo plots allow you to drag
  multiple properties into the same plot to display them against each other. By using
  units and metric prefixes Karabo can decide which values can share the same y-axis,
  and add new axes if data in a new unit is dragged onto the plot.



Timestamps
++++++++++

Karabo's properties have timestamps, which are either passed up from hardware
interfaced to the control system or set to the current time upon property
assignment. A central timing service assures synchronization across the
distributed system. Alternatively, developers may set an arbitrary timestamp
upon assignment as an optional parameter in set commands:

.. code-block:: Python

    now = self.getActualTimestamp()
    timeNow = Epochstamp() # this is only a time
    train = 12 # we also need a train id
    now2 = Timestamp(timeNow, train) # a timestamp consists of a time and train id
    self.set("a", 1, now)

You can convert Karabo's internal timestamps to other representations using
the following functions:

.. function:: toIso8601(precision = MICROSEC, extended = False)

    Generates a sting (respecting ISO-8601) for object time for INTERNAL usage
    ("%Y%m%dT%H%M%S%f" => "20121225T132536.789333[123456789123]")

    ``precision`` - Indicates the precision of the fractional seconds
    (e.g. MILLISEC, MICROSEC, NANOSEC, PICOSEC, FEMTOSEC, ATTOSEC)

    ``extended`` - "true" returns ISO8601 extended string; "false" returns
    ISO8601 compact string

.. function:: toIso8601Ext(precision = MICROSEC, extended = False)

    Generates a string (respecting ISO-8601) for object time for EXTERNAL usage
    ("%Y%m%dT%H%M%S%f%z" => "20121225T132536.789333[123456789123]Z")

    ``precision`` - Indicates the precision of the fractional seconds
    (e.g. MILLISEC, MICROSEC, NANOSEC, PICOSEC, FEMTOSEC, ATTOSEC)

    ``extended`` - "true" returns ISO8601 extended string; "false" returns
    ISO8601 compact string

.. function:: toFormattedString(format = "%Y-%b-%d %H:%M:%S", localTimeZone = "Z")

     Formats to specified format time stored in the object

     ``format`` the format of the time point (visit `strftime <`http://www.cplusplus.com/reference/ctime/strftime/>`_
     for more info).

     ``localTimeZone`` - String that represents an ISO8601 time zone.

.. function:: getSeconds()

    Returns the seconds of the unix epoch for this timestamp

Timestamps are given by seconds of the UNIX epoch alongside fractional seconds
used to provide additional accuracy for resolving the XFEL pulse-structure in
the femtosecond range.

.. function:: getFractionalSeconds()

    Returns the fractional seconds of this timestamp

.. function:: getTrainId()

    Returns the train id for this timestamp

.. _setandexecute:


The Karabo Schema
+++++++++++++++++

Karabo stores a static description of a device as part of the device's schema.
The schema contains information on the expected parameters of the device,
including property types and default values. Underneath, the schema uses the
same technology as the Karabo Hash to construct a hierarchical, ordered key-
value representation. It is serializable to XML. Currently, Karabo does not
support schema evolution.

The TABLE_ELEMENT
+++++++++++++++++

The ``TABLE_ELEMENT`` internally is a ``VECTOR_HASH`` property which has a ``rowSchema``
attribute defining the cells a row consists of. As this is the same for all rows, the
schema defines the columns of the table. Columns may be of any Karabo data type, although
the GUI will only render scalar types and fail gracefully for others. A ``TABLE_ELEMENT``
is defined as follows:

.. code-block:: Python

	tableSchema = Schema()
	(
	    UINT32_ELEMENT(tableSchema).key("col1)
	       .displayedName("Column One")
	       .assignmentOptional().noDefaultValue()
	       .commit()
	       ,
		STRING_ELEMENT(tableSchema).key("a)
	       .displayedName("A")
	       .assignmentOptional().defaultValue("Hello World!")
	       .commit()
	       ,
	    FLOAT_ELEMENT(tableSchema).key("b)
	       .displayedName("Float Val")
	       .assignmentMandatory()
	       .commit()
	)

	tableDefault = [Hash("col1", 1, "b", 2.0)]

	TABLE_ELEMENT(expected).key("table")
	    .displayedName("A Table Element")
	    .setRowSchema(tableSchema)
	    .assignmentOptional().defaultValue(tableDefault)
	    .commit()

This will render to

+-------------+--------------+---------------+
| **Column 1**| **A**        | **Float Val** |
+-------------+--------------+---------------+
|     1       | Hello World! | 2.0           |
+-------------+--------------+---------------+

in the GUI. Entries of the element are validated according to the validation rules
specified in the property definition: *col1* may stay undefined and will not if initialized
to a default value in this case, *a* is initialized to "Hello World!" if it is undefined,
and *b* needs to be defined, otherwise an exception is thrown.

Default values are passed to the element as a vector/list of Hashes, where each Hash
validates against the row schema.
