.. _middle_layer_api:

********************
Middle Layer Devices
********************

In Karabo ``middle-layer`` devices are devices which interact with other devices,
frequently *bound* devices, as a means of aggregating their functionality into
a view closer to the hardware being controlled, controlling many similar entities at once
or providing aggregate state information (``composite devices``). They may also implement
higher-level functionality, such as scans (``scan devices``), data acquisition scenarios
or processing pipeline reconfiguration.

The ``middle-layer`` API has been designed with such tasks in mind. It exposes a concise
and efficient interface for tasks such as interacting with properties of other devices,
monitoring such properties, executing commands and waiting on their completion, either
synchronously or asynchronously. It does not expose the fine-grained event control as is
possible with the *bound* APIs nor does it allow for a full FSM implementation. A complete
list of limitations is given in the corresponding section.



Benefits
========

The benefits of the middle-layer API are its pythonic, concise syntax aimed at rapid
development cycles. As a result much fewer lines of code for implementing interfaces
are needed than in the *bound* APIs, and there is no need to worry about request/reply
patterns. Instead properties of remote devices are accessed just as if they were a normal
Python object. Consider

..  code-block:: Python

    from karabo.api import PythonDevice, SLOT_ELEMENT, FLOAT_ELEMENT

    class MyPythonDevice(PythonDevice):

        def expectedParameters(expected):

            (
              SLOT_ELEMENT(expected).key("slot1")
                  .displayedName("Slot 1")
                  .commit()
                  ,
              FLOAT_ELEMENT(expected).key("afloat")
                  .displayedName("A Float")
                  .assignmentOptional().noDefaultValue()
                  ,
              ...
            )

         def __init__(self):
             self.KARABO_SLOT("slot1")

         def slot1(self):
             f = self.get("afloat")
             ....


compared to

..  code-block:: Python

    from karabo.middlelayer import Device, Slot, Float

    class MyMiddleLayerDevice:

        afloat = Float(displayedName="A Float")

        @Slot(displayedName="Slot 1")
        def slot1(self):
            f = self.afloat
            ....




Limitations
===========

The middle-layer API is aimed at devices controlling other devices, **not** at controlling
hardware directly. This is reflected in the following limitations with respect to  *driver*
API devices:

- no full finite state machine is implemented. Specifically, the simple state machine
  available does not enforce state transitions via a transition table. Rather it is up
  to the programmer to assure proper transitions at all times.

- the low level signal/slots interface is not exposed through the API. Accordingly,
  fine-grained control of event synchronicity is not possible to implement.

- the pipelined processing interfaces are not exposed. For performance reasons the
  pipelined processing API is realized via
  C++ bindings in both core APIs. The middle-layer API is binding free and thus does not
  implement this interface.



API Documentation
=================

Karabo uses `Python <http://www.python.org>`_ as a middle-layer and macro language.
Readers are thus referred to the `Python Documentation
<http://docs.python.org/3/>`_ for language-specific questions. Especially the `tutorial
<http://docs.python.org/3/tutorial/index.html>`_ is a good starting point for beginners.

Similarly to devices developed in the *bound* APIs, the middle-layer API supports the
notion of commands, accessible as slots, and properties on devices. Slots may be called
and properties may be accessed across all three APIs, as for the distributed system they
expose the same interface.

The middle-layer API however simplifies definition of expected parameter properties and
slots by implicitly binding slots to functions and GUI buttons and the set and get methods
as Python properties.

Hello World!
+++++++++++

Let's start with the classic hello world example::

    from karabo.middlelayer import PythonDevice, Slot

    class HelloWorld(Macro):
        __version__ = "1.4 1.5"

        @Slot()
        def hello(self):
            print("Hello world!")

As is shown in the source code, a middle-layer device is created by inheriting from
the middle-layer's ``PythonDevice`` base class. Similar to the ``@CLASSINFO`` decorator found
in the *bound* APIs it is expected that the programmer specifies under which Karabo versions
this device can be run. This is done by setting the ``__version__`` property.

In the example we create a single slot by decorating a member function accordingly. This
will render as a button labeled "hello" in the GUI or be accessible using
``remote().execute("/some/remote/device", "hello")`` from the CLI.

.. note::

	It is good practice to explicitly specify imports as shown in the example, rather than
	using ``from foo import \* ``.

Adding properties is similarly straight-forward::


    from karabo.middlelayer import PythonDevice, Slot, String

    class HelloYou(PythonDevice):
        __version__ = "1.4 1.5"
        name = String()

        @Slot()
        def hello(self):
            print("Hello", self.name, "!")

The device ``HelloYou`` now has a string expected parameter, which is rendered in the GUI
as a text box, and accessible from the CLI via ``remote().get("InstanceID", "name")``.
Attributes assignable in the expected parameter declarations of the ``core`` APIs may also
be specified in the middle-layer API property definitions::

    from karabo.middlelayer import PythonDevice, Slot, String

    class HelloYou(PythonDevice):
        __version__ = "1.4 1.5"
        name = String(displayedName="User Name",
                      description="Please enter your name here")

        @Slot(displayedName="Greet User")
        def hello(self):
            print("Hello", self.name, "!")

        @Slot(displayedName="Greet Ernie")
        def greetErnie(self):
            self.name = "Ernie"
            self.hello()


Units
+++++

The properties of a ``PythonDevice`` contain more than just their
value: they are full-fledged objects with additional attributes.

You can define a unit for a property, which is then used in the
calculations of this property. In the middle-layer API, units, amongst other
things, are implemented using the ``pint`` module.

A unit is declared using the ``unitSymbol`` and optionally, the
``metricPrefixSymbol`` attributes::

    distance = Float(
        unitSymbol=Unit.METER,
        metricPrefixSymbol=MetricPrefix.MICRO)
    times = VectorFloat(
        unitSymbol=Unit.SECOND,
        metricPrefixSymbol=MetricPrefix.MILLI)
    speed = Float(
        unitSymbol=Unit.METER_PER_SECOND)
    steps = Float()

Once declared, all calculations have correct units::

    self.speed = self.distance / self.times[3]

In this code units are  converted automatically. An error is
raised if the units don't match up::

    self.speed = self.distance + self.times[2]  # Ooops! raises error

If you need to add a unit to a value which doesn't have one, or remove
it, there is the ``unit`` object which has all relevant units as its
attribute::

    self.speed = self.steps * (unit.meters / unit.seconds)
    self.steps = self.distance / (3.5 * unit.meters)

.. warning::

    While the middle-layer API of Karabo in principle allows for automatic
    unit conversion, developers are strongly discouraged to use this feature for
    critical applications: the Karabo team simply cannot guarantee that
    ``pint`` unit handling is preserved in all scenarios, e.g. that a unit
    is not silently dropped.

Timestamps
++++++++++

All properties in Karabo may have timestamps attached. In the middle-layer API
they can be accessed from the ``timestamp`` attribute::

    self.speed.timestamp

They are automatically attached and set to the current time upon
assignment of a value that does not have a timestamp::

    self.steps = 5  # current time as timestamp attached

A different timestamp may be attached using the ``timestamp``
function::

    self.steps = timestamp(5, "2009-09-01 12:34 UTC")

If a value already has a timestamp, it is conserved, even through
calculations. If several timestamps are used in a calculation, the
newest timestamp is used. In the following code, ``self.speed`` gets
the newer timestamp of ``self.distance`` or ``self.times``::

    self.speed = 5 * self.distance / self.times[3]

.. warning::

    Developers should be aware that automated timestamp handling defaults to the
    newest timestamp, i.e. the time at which the last assignment operation
    on a variable in a calculation occured. Additionally, these timestamps are
    not synchronized with XFEL's timing system, but with the host's local clock.
    If handling of timestamps is a critical aspect of the algorithm being
    implemented it is strongly recommended to be explicit in timestamp handling,
    i.e. use ``speed_timestamp = self.speed.timestamp`` and re-assign this
    as necessary using ``timestamp(value, timestamp).

Other Property Attributes
+++++++++++++++++++++++++

Attributes of properties may be accessed as members of the property.
This may sound a bit strange as first, if one views the property as a
piece of data. By understanding that the properties are in fact objects, the
interface becomes more natural.

The attributes which can be specified as part of property definition,
i.e. *default* attributes, are directly accessible. Note that the middle-layer
API knows a fixed list of attributes. It is not possible to have user-defined
attributes.

+------------------+------------------------------------------------------------------------------------+
|**Attribute**       **Example**                                                                        |
+------------------+------------------------------------------------------------------------------------+
|display type      | self.a.displayType  #returns oct, bin, dec, hex                                    |
+------------------+------------------------------------------------------------------------------------+
|minimum (setting) | self.a.minInc  # the inclusive-minimum value                                       |
|                  +------------------------------------------------------------------------------------+
|                  | self.a.minExc  # the exclusive-maximum value                                       |
+------------------+------------------------------------------------------------------------------------+
|maximum (setting) | self.a.maxInc  # the inclusive-minimum value                                       |
|                  +------------------------------------------------------------------------------------+
|                  | self.a.maxExc  # the exclusive-maximum value                                       |
+------------------+------------------------------------------------------------------------------------+
|warning           | self.a.warnLow  # values below or equal to this cause a warnin                     |
|                  +------------------------------------------------------------------------------------+
|                  | self.a.warnHigh  # values above or equal to this cause a warning                   |
+------------------+------------------------------------------------------------------------------------+
|alarm             | self.a.alarmLow  # values below or equal to this cause an alarm                    |
|                  +------------------------------------------------------------------------------------+
|                  | self.a.alarmHigh  # values above or equal to this cause an alarm                   |
+------------------+------------------------------------------------------------------------------------+
|variance (warning)| self.a.warnVarHigh # the maximum variance value                                    |
|                  +------------------------------------------------------------------------------------+
|                  | self.a.warnVarLow # the minimum variance value                                     |
+------------------+------------------------------------------------------------------------------------+
|variance (alarm)  | self.a.alarmVarHigh # the maximum variance value                                   |
|                  +------------------------------------------------------------------------------------+
|                  | self.a.alarmVarLow # the minimum variance value                                    |
+------------------+------------------------------------------------------------------------------------+
|unit              | self.a.unitSymbol  # a unit enum, e.g. Unit.METER                                  |
|                  +------------------------------------------------------------------------------------+
|                  | self.a.metricPrefixSymbol  # a prefix enum, e.g. MetricPrefix.MILLI                |
+------------------+------------------------------------------------------------------------------------+
|unit scale        | self.a.unitScale  # the key of property holding the scale                          |
+------------------+------------------------------------------------------------------------------------+
|access mode       | self.a.accessMode  # an access mode enum, e.g. AccessMode.READONLY                 |
+------------------+------------------------------------------------------------------------------------+
|assignment        | self.a.assignment  # an assignment enum, e.g. Assignment.OPTIONAL                  |
+------------------+------------------------------------------------------------------------------------+
|default value     | self.a.defaultValue  # the default value or None                                   |
+------------------+------------------------------------------------------------------------------------+
|access level      | self.a.requiredAccessLevel  # access level enum, e.g. AccessLevel.EXPERT           |
+------------------+------------------------------------------------------------------------------------+
|allowed states    | self.a.allowedStates  # the list of allowed states                                 |
+------------------+------------------------------------------------------------------------------------+
|direct execution  | self.a.setAndExecute # setting this property leads to a command execution          |
|upon assignment   |                                                                                    |
+------------------+------------------------------------------------------------------------------------+

Logging Information
+++++++++++++++++++

The examples so far have printed directly to the command line. Frequently, information
should be passed to the distributed system in terms of a log message::

    self.logger.info("Some Information")
    self.logger.warning("Things are starting to go wrong")
    self.logger.error("Something went wrong")
    self.logger.debug("I am in debugging mode")


As is evident from the example there are four types of message categories: ``info, warning,
error and debug.`` It is important to understand the different between warning and error
messages: a warning should be issued if some a state has been reached that is not
considered normal, but logic has been implemented to recover from it. It may also be
issued if there are known indications that the device is running into an error state.

Conversely, an error message should be issued if an unforeseen scenario has happened, i.e.
there is no logic to recover from this, or if a foreseen error has happened which needs
human attention to recover from, e.g. by manual procedures or overrides.

.. warning::

	Make yourself aware of the conceptual difference between warnings and errors and
	emit error notification only in case of actual errors.

	Experience from other facilities shows that *error-spamming* leads to users accepting
	errors and associated notifications as a normal operation state - they are not!.
	Error notifications should be so rare that they trigger a human examination of the
	problem!

	An error-categorized message may trigger email or text notification of experts.


.. _synchronized:

Synchronized Functions
++++++++++++++++++++++

There are many functions in Karabo which do not instantaneously execute.
Frequently, it is important that other code can continue running
while such a function is still executing. For the ease of
use, all those functions, which are documented here as
*synchronized*, follow the same calling convention, namely, they have
a set of additional keyword parameters to allow for non-blocking calls to them:

timeout
    gives a timeout in seconds. If the function is not done after
    it timed out, a ``TimeoutError`` will be raised, unless the
    timeout is -1, meaning infinite timeout. The executed function
    will be canceled once it times out.

callback
    instead of blocking until the function is done, it returns
    immediately. Once the function is done, the supplied callback
    will be executed. The function returns a ``Future`` object,
    described below; the callback will get the same
    future object passed as its only parameter.

    If callback is ``None``, the function still returns immediately
    a future, but no callback is called.

The future object contains everything to manage asynchronous
operations:

.. py:class:: Future

    .. py:method:: cancel()

        Cancel the running function. The running function will stop
        executing as soon as possible.

    .. py:method:: cancelled()

        Return whether the function was canceled

    .. py:method:: done()

        Return whether the function is done, so returned normally,
        raised an exception or was canceled.

    .. py:method:: result()

        Return the result of the function, or raise an error if the
        function did so.

    .. py:method:: exception()

        Return the exception the function raised, or ``None``.

    .. py:method:: add_done_callback(cb)

        Add a callback to be run once the function is done. It gets passed
        the future as the single parameter.

    .. py:method:: wait()
        wait for the function to finish

You can call your own synchronized_ functions and launch them in the
background:

.. py:function:: background(func, *args)

   Call the function *func* with *args*.

   The function passed is wrapped as a synchronized_ function.
   So if you give the *callback* parameter, the *func* gets called in
   the background, and the caller is notified via a the callback upon
   completion, with any return values passed as a future.

   The called function can be canceled. This happens the next time it
   calls a synchronized_ function. A ``CancelledError`` is raised in
   the called function, which allows you to react to the cancellation,
   including ignoring it.

    .. note::
        It is not possible to cancel any other operation than calls to
        synchronized_ functions, as interventions in third party code
        are not possible.

.. py:function:: sleep(delay)

   Stop execution for at least *delay* seconds.

   This is a synchronized_ function, so it may also be used to
   schedule the calling of a callback function at a later time.

   .. warning::

      You should always prefer this sleep function over
      ``time.sleep``. As described above, this sleep can be canceled,
      while ``time.sleep`` cannot.


Interacting with Other Devices
++++++++++++++++++++++++++++++

The main purpose of middle-layer devices (and macros) is to interact with other devices,
be it in the form of composition, state aggregation, or implementing some level of higher
functionality. In the middle-layer API this is done by connecting another device to a
a ``proxy object``::

    @Slot()
    def some_function(self):
        with getDevice("some_device") as device:
	        print(device.speed)

.. note::

	Observe the with statement used in the example. The proxy device supports the
	``__enter__`` and ``__exit__`` methods, which means it will cleanly instantiate itself
	upon usage and make sure that it has a clean exit when it falls out of use, e.g. the
	connections to the remote device are closed.

The remote device, wrapped in the proxy, can then be used as if it were a local object
with properties and methods. Here properties map to the remote devices ``set`` and ``get`` methods
and methods to its commands, exposed as slots::

    device.someProperty = 7
    device.start()  # as if you had pushed the start button
    print(device.someProperty)
    c = device.someOtherProperty

Assignment to a proxy's attribute is a blocking operation, the
execution will only continue after the device has acknowledged the
assignment.

Calling slots on a proxy is a synchronized_ operation: you can use
the *timeout* and *callback* parameters as usual.

There is also a synchronized_ version of an assignment, called
``set``::

    try:
       device.someProperty.set(1, timeout=10)
    except TimeoutError as e:
       print("Failed at setting someProperty")

Often many parameters of a device need to be changed at the same time.
Then it makes a lot of sense to set those parameters in bulk. This can
be done with the context manager ``bulk_set``::

    with bulk_set():
        device.property1 = 1
        device.property2 = 2
        ...
        device.property100 = 100
        device.start()

With this context manager, all setting operations are cached and sent
to the device in bulk. Any call to a synchronized_ function within
this context manager also flushes the cached sets to their devices, to
retain a linear program flow.

Generically using other devices
+++++++++++++++++++++++++++++++

Often one wants to generalize a middle-layer device such that it can
utilize other devices generically, defined by the user. Three special
properties allow for that: ``RemoteDevice`` lets a user define a
device to be used, while with a ``RemoteProperty`` the user can point
the device to a single property within a device. A ``RemoteSlot``
likewise defines a slot on another device. When the device is started,
Karabo will assure that the remote devices are also running, and that
the properties and slots exist. Otherwise an error will be noted.
The remote devices will stay connected during the lifetime of the
device.

Those remote properties may be used as if they were local properties::

    class GenericDevice(Device):
        client = RemoteDevice()
        target = RemoteProperty()
        start = RemoteSlot()

        @Slot
        def someSlot(self):
            # use devices:
            waitUntil(lambda: self.client.state == State.RUNNING)

            # read and write properties:
            if self.target > 0:
                self.target = 22

            # call remote slots:
            self.start()


Compositing other devices
+++++++++++++++++++++++++

An additional option is to create a node element holding some of the remote device's
properties as part of your devices expected parameters definition. The following
example illustrates this with a device creating a circular movement by coordinating
the movement of an X- and Y-stage:

.. literalinclude:: examples/circlemotor.py


.. note::

    Calling ``DeviceNode`` without parameters will only expose the state, alarm
    condition and status properties of the remote device. A list of parameters
    may be used to define parameters *in addition* to these. Further you may
    define a list of commands, which if an item is given as a tuple will bind
    the particular slot to the given slot, or if only a string is given binds
    to the same name. If the property given is a node element, all properties
    below that node element are mirrored.

The remote devices will render as nodes in your expected parameters under the
names you assign them. Each of these nodes contains as its first entry the
remote device's instance_id, which you should assign upon initialization.

Optionally, the ``target`` attribute
can be used to inject the remote device properties from device node into a specific
node element in the local device, or at the top of the hierarchy by using
``target=root``.

.. note::

    The assignment of an instance id to a DeviceNode is not mandatory, the
    node representing the device will simply stay empty if you do not do
    this. It is however an ``init-only`` parameter. If you need to access
    devices depending on run-time information you should use the ``getDevice``
    and ``connectDevice`` methods.



Monitoring Remote Devices
+++++++++++++++++++++++++

For monitoring the properties of a remote device, one can use the
synchronized_ function ``waitUntilNew``. A simple loop is all one
needs to achieve this::

    def monitorProperty(self):
        with getDevice("someDevice") as device:
            while True:
                waitUntilNew(device.someProperty)
                # do something with the property

Frequently, it is expected that a command will take a while to
execute. Depending on how this is realized in the remote device the
device will change states, e.g. go into a moving state, but the
command returns immediately, or the command does not return until the
action has completed.  In such cases the the program flow of the
middle-layer device should either:

- wait for the remote device to reach a defined state,
- continue but trigger a callback if the state is reached,
- or continue despite the remote call blocked and react on a callback on
  its actual return

To handle the first two cases we may use the ``waitUntil`` methods::

    def blockAndWaitOnState(self):
        with getDevice("someDevice") as someDevice:
            someDevice.start()  # this call is expected to return immediately
            # now we block until the STOPPED state is reached
            waitUntil(lambda: someDevice.state == states.STOPPED)
            # program flow continues after wait
            ...

    def continueAndCallback():
        with getDevice("someDevice") as someDevice:
            someDevice.start()  # this call is expected to immediately return
            # the next statement is non-blocking
            waitUntil(lambda: someDevice.state == states.STOPPED,
                      callback=self.callback)
            # program flow commences immediately
            ...

    def callback(future):
        # do something
        ...


The final case, continuing in the middle-layer device program flow
although a command is blocking and then executing a callback upon
completion, is handled by supplying the callback to the command::

    def blockingCallWithCallback(self):
        with getDevice("someDevice") as someDevice:
            # the following call returns immediately
            someDevice.start(callback=self.boundCallback)
            #program flow continues
            ...

    def callback(future):
        # do something
        ...

In all cases the callback function has *future* as its only parameter.
You can get the possible return value of the called function with
``future.result()``.

.. py:function:: waitUntil(condition)

    Wait until the condition is True

    the condition is typically a lambda function, as in::

        waitUntil(lambda: device.speed > 3)

    The condition will be evaluated each time something changes. Therefore the
    condition should be something that can be evaluated fast, as the simple
    comparison in the example. This is a synchronized_ function.

.. py:function:: waitUntilNew(property)

   wait until a new value for a property is available

   this synchronized_ function waits until a specific property of a device
   changes::

       waitUntilNew(someDevice.someProperty)

   Note that this function does not guarantee that you get all
   updates of a property. If updates arrive too fast, Karabo may skip
   them and only return on the last update.

.. py:class:: Queue(property)

   queue all updates of *property*. This is a way to be informed about
   all changes of a particualar properties::

       queue = Queue(someDevice.someProperty)

   .. py:method:: get()

   return and remove a parameter update from the queue. This is a
   synchronized_ method.

Accessing Remote Property Attributes
+++++++++++++++++++++++++++++++++++++

Attribute interaction on remote devices is intentionally very similar to
interacting with the calling device. However, attributes on remote devices are always
read-only! Thus the following works::

    someDevice.a.unitSymbol

but ``setAttribute`` is not implemented. If there is a need to actually alter an attribute
from a middle-layer device this functionality should be explicitly exposed by the remote
device in terms of a slot for the middle-layer device to call.

..  code-block:: Python

    class RemoteDevice(Device):

        digitizerValue = Integer(displayedName = "Digitizer Value",
                                 metricPrefix = metric_prefixes.KILO)

        ...

        @Slot()
        def amplificationChanged(amp):
            if amp == 1000:
                self.digitizerValue.setAttribute("metric_prefix", metric_prefixes.KILO)
            elif amp == 100:
                self.digitizerValue.setAttribute("metric_prefix", metric_prefixes.CENTI)
            else:
                raise AttributeError("Unknown amplification")

    class MiddleLayerDevice(Device):

        ...

        @Slot()
        def changeAmplification()
            with getDevice("remote") as remoteDevice:
                remoteDevice.amplificationChanged(self.amplification)



Locking Devices In Use
++++++++++++++++++++++

Middle-layer devices controlling hardware via *driver* devices often need to be assured
of exclusive access to the hardware. For instance, during a scan one would want to
prevent accidental overriding of commands issued by the scan device, as would be possible
by a user accessing the ``driver`` device. To resolve this issue devices support
locking. A locked device will only allow read-only access
to its properties by a device not holding the lock. Similarly command execution is
restricted to the lock holder::

    @Slot(displayedName="Perform X-scan")
    def scan_x(self):
        with getDevice("some_device") as device, \
             getDevice("some_other_device") as other_device:

            with lock(device), lock(other_device):
                # do something useful here

.. py:function: lock(device, timeout=0)

   lock the *device* for exclusive use by this device. If the lock
   cannot be acquired within *timeout* seconds, a ``TimeoutError``
   will be raised. A *timeout* of ``-1`` signifies an unlimited wait.

   the function returns a context manager to be used in a ``with``
   statement.

   In Karabo, locks may be "stolen" by users with a higher access level.
   If this happens, the next call to a Karabo function will result in
   a ``LockStolenError`` being raised.

The parameter ``lockOwner`` of a device contains the current owner
of the lock, or an empty string if nobody holds a lock.

.. warning::

	Device locks are not thread-safe locks. If you execute two threads in your device
	in parallel, interactions will be serialized by the Karabo core, but you cannot use
	the device locking mechanism to enforce a particular order of execution. If you need
	to lock threads, use the appropriate locks from the threading library.

.. todo::

	The lock concept needs to be implemented and discussed. Since it needs to be accessible
	also from the APIs one option is to implement a lock slot in all APIs. This takes
	the locking devices instance id as parameter, sets a ``lockedBy`` property to the
	instance id of the locking device and after execution will block all interactions
	not from this instance id until the device is unlocked. In a first step we might actually
	only implement that the GUI evaluated the device as being locked and gray it out.
	Conversely, there needs to be an unlock slot (also taking the instance id as parameter)
	and allowing an unlock by the device which locked in the first place. Finally, we need
	a parameterless unlockOverride slot, which is expert only and can always trigger the unlock.
	Device locking should possibly reflect its state in the configuration data base, so
	that locked device may be queried.

	In  devices the functionality will be less frequently needed but would then already
	be exposed via calls to these slots.

	In either case: locking is a MUST requirement and needs to be implemented.


Convenience Shorthands
++++++++++++++++++++++

Although property access via device proxies is usually to be preferred, there are scenarios
where only a single or very few interactions with a remote device are necessary. In such
a case the following shorthands may be used::

   setWait("deviceId", "someOtherParameter", a)
   execute("deviceId", "someSlot", timeout=10)

The aforementioned commands are blocking and all accept an optional timeout parameter.
They raise a ``TimeoutError`` if the specified duration has passed.

Additionally, non-blocking methods are provided, indicated by the suffix ``NoWait`` to
each command::

   def callback(deviceId, parameterName, value):
       #do something with value
       ...

   setNoWait("deviceId", "someOtherParameter", a)
   executeNoWait("deviceId", "someSlot", callback = callback)

As shown in the code example a non-blocking property retrieval is realized by supplying
a callback when the value is available. The callback for ``executeNoWait`` is optional and
will be triggered when the execute completes.

.. ifconfig:: includeDevInfo is True

	The ``executeNoWait`` method without callback is internally implemented by sending
	a fire-and-forget signal to the remote device.

	If a callback is given, instead a blocking signal is launched in co-routine,
	triggering the callback upon completion. The ``executeNoWait`` call will immediately
	return though.


Finally, the following holding methods are available:

.. code-block:: Python

	def callback(deviceId, parameterName, value):
	    #do something with value

	waitUntilNew("deviceId", "someParameter") #blocks
	executeOnUpdate("deviceId", "someParameter", callback) #does not block

.. note::

	There is a subtle difference between ``getNoWait`` and ``executeOnUpdate``:
	``getNoWait`` will immediately try to retrieve the requested property,
	while *executeOnUpdate* will not initiate a property request, but execute the callback
	when the remote device sends a notification of a property update.

.. warning::

	It may seem tempting to always use these convenience methods. Keep in mind though
	that for each call of one of these methods a connection to a remote device needs to
	be established, the request needs to be executed, and the connection needs to be
	closed.

	Using a proxy object on the other hand keeps the connection alive for the lifetime
	of the proxy, with the additional option of manually connecting and disconnecting.
	If you frequently need to interact with a remote device this is thus the more
	efficient solution.


Error Handling
==============

Errors do happen. When they happen, in Python typically an exception is
raised. The best way to do error handling is to use the usual Python
try-except-statements.

So far we have introduced and taken care of time-out errors. Another recurring situation
is that a user cancels a operation currently in progress. In such cases a ``CancelledError``
is raised:

..  code-block:: Python

    @Slot
    def do_something(self):
        try:
            # start something here, e.g. move some motor
        except CancelledError:
            # clean up stuff
        finally:
            # something which should always be done, e.g. move the motor
            # back to its original position

Sometimes, however, an exception happens unexpectedly, or should be handled in a quite
generic fashion. In either case it might be advisable to bring the system back into a
defined, safe state. This can be done by overwriting the following device methods::

    def onCancelled(self, slot):
        """to be called if a user canceled the operation"""

The ``slot`` is the slot that had been executed.


Injecting Parameters
====================

Sometimes it is necessary to inject new parameters while the device is
already running. This should be used with care: injected parameters
obviously cannot be pre-configured, and data logging may be hard to
comprehend if the data fields constantly change.

Devices which wish to change their parameters need to inherit from
:class:`~karabo.middlelayer.Injectable`. This mixin class assures that a
brand-new class is created for every instance. This is necessary, as
we do not want to modify the class for every instance, but only for
the device we are working on.

Once we inherited from :class:`~karabo.middlelayer.Injectable`, we can
freely modify ``self.__class__``. Once we have done those
modifications,
:meth:`~karabo.middlelayer.Injectable.publishInjectedParameters`. As
an example::

    class MyDevice(Injectable, Device):
	def inject_something(self):
	    # inject a new property into our personal class:
	    self.__class__.injected_string = String()
	    self.publishInjectedParameters()

	    # use the property as any other property:
	    self.injected_string = "whatever"


Programming Policies
====================

First of all: try to write concise code, which is explicit enough to be read *and
understood* by other people. If in doubt type::

    import this

in a Python prompt and follow the instructions provided. For those too lazy to do so,
read the following note.

.. note::

	| The Zen of Python, by Tim Peters

	| Beautiful is better than ugly.
	| Explicit is better than implicit.
	| Simple is better than complex.
	| Complex is better than complicated.
	| Flat is better than nested.
	| Sparse is better than dense.
	| Readability counts.
	| Special cases aren't special enough to break the rules.
	| Although practicality beats purity.
	| Errors should never pass silently.
	| Unless explicitly silenced.
	| In the face of ambiguity, refuse the temptation to guess.
	| There should be one-- and preferably only one --obvious way to do it.
	| Although that way may not be obvious at first unless you're Dutch.
	| Now is better than never.
	| Although never is often better than *right* now.
	| If the implementation is hard to explain, it's a bad idea.
	| If the implementation is easy to explain, it may be a good idea.
	| Namespaces are one honking great idea -- let's do more of those!

To be a bit more explicit in terms of Karabo: use device proxies if you need to repeatedly
interact with a remote device (or just use them in general). Avoid using the short-hand
functions unless your really only need to access a remote device once. Make yourself
familiar with the limitations of the API and please **do not** implement hardware accessing
devices therein. Avoid writing *C* in *Python*: if your algorithm works on a vector or
array a nested for loop is most likely the wrong approach and vectorized numpy calls the
right approach. Check out the python dependencies available for Karabo, or request a
library to be added, before reimplementing existing functionality.

Finally, as middle-layer devices often facilitate recurring tasks,
there is a chance that someone has faced the same problem before and thus a macro or
middle-layer device for a similar problem is already available. Search the repository
first, before reinventing the wheel.

Composite Devices
+++++++++++++++++

As indicated by their name, composite devices are used to compose the functionality of
multiple devices into a single entity. They come in different, non-exclusive variants:

- State composition: the composite device evaluates the individual states of multiple
  devices and aggregate this information into a kind of meta-state. Often, the
  meta-state indicate if all devices are *okay*, of if *any* device is in an *error*
  or *warning* state. It is common practice to communicate the device state that is
  defining to the state aggregation rule, i.e. if a group of devices is in a *moving*
  state, the meta-state will be *moving* as long as a single device still moves.

- Property aggregation: the more common scenario here is batch property setting, i.e.
  setting the same property on multiple device to the same target value. Conversely,
  a composite device may present the mean value or other statistical aggregates of
  properties it reads from the other devices.

- Manager-type devices: manage a group of possibly heterogeneous devices by assuring e.g.
  that configuration or calibration data is loaded and distributed, state transitions
  are performed in a sequenced order for the entire group, or that managed devices are
  made aware of each other through a single communication point.

In all cases it is good practice to make the composite device configurable to the device
instances it is composed of, i.e. you should not hard code instance ids into it. Instead,
either provide a string or table field to add instance ids too, or a slot on which the
devices composed upon can register themselves, provided they are given the *configurable*
instance id of the composition device.

.. warning::

	When writing composite devices make yourself aware if the hardware enforces some
	type of protection from misconfiguration or not. In the latter case, consider not
	re-implementing safety features already present on the device you compose upon, but
	delegate the decision of whether a command is safe to execute to them.

.. note::

	Before implementing a new composite device check whether a similar task has been
	taken care of before, and may be adaptable to your needs.


Scan Devices
++++++++++++

Scan devices are can be classified as *manager* type *composite* devices (see above). A
scan device most frequently

1. brings the hardware and devices it relies on into a defined state
2. steps a hardware property through a range of values, defined by a starting position, an
   increment and an end position or the number of steps
3. at each step triggers some sort of processing or data acquisition of other value
4. brings the hardware and devices it relies on back into the initial state or another defined
   state.

Given the above requirements, a scan device thus

- needs to be aware of subordinate devices it controls, which may be of heterogeneous
  nature, which can be implemented in either or both ways described for composite devices.
- Must be able to lock these devices so that no outside interference is possible during
  the scan. This is done via the locking mechanism.
- Must be able to trigger data acquisition via the run management, which may be done
  using the Karabo-provided *simpleRunAcquistion* device.

An exemplary device scanning is implemented below. In the example ``command()``
would refere to trigger the DAQ.

.. literalinclude:: examples/scan.py

A PID Controller Device
+++++++++++++++++++++++

A PID controller tries to minimize the difference between the *process
variable* of a device and a user defined *setpoint* by acting on a *control
variable*, often on a different device.

The time evolution of this difference, the *error*, is fed back to the
control variable using the error itself, its integral and its derivative.

.. literalinclude:: examples/pid.py



.. ifconfig:: includeDevInfo is True

    The design of the Karabo Middle-Layer API
    =========================================

    In Karabo, every device has a *schema*, which contains all the details
    about the expected parameters, its types, comments, units, everything.
    It is only broadcast rarely over the network, typically only during
    the initial handshake with the device. Once the schema is known, only
    *configurations*, or even only parts of configurations, are sent over
    the network in a tree structure called *Hash* (which is not a hash
    table).

    These configurations know nothing anymore about the meaning of the
    values they contain, yet they are very strictly typed: even different
    bit sizes of integers are conserved.

    This dichotomy is similar to classes and objects (more precisely: an
    object's ``__dict__``) in Python. Similar, but different, which means
    that every time data is sent to or received from the network, we have
    to do a conversion step. When data is received, we check it for
    validity and add all the details that we know from the schema, once
    data is sent, we assure all data is converted to the correct unit and
    data format, and strip it of all the details.

    Setting, sending and receiving parameters
    -----------------------------------------

    All these conversions are centered around the
    :class:`~karabo.middlelayer.Type`. Its main conversion routines are
    :meth:`~karabo.middlelayer.Type.toKaraboValue` which converts data
    from the network or from the user to a
    :class:`~karabo.middlelayer.KaraboValue`, and
    :meth:`~karabo.middlelayer.Type.toHash`, which converts a Karabo value
    to the hash for the network.
    :meth:`~karabo.middlelayer.Type.toKaraboValue` has an attribute
    *strict* which defines whether the conversion should check exactly the
    right unit or whether it simply adds a unit if none exists. The latter
    behavior is needed for data coming from the network, as it has no unit
    information, while the former behavior is used in case the user
    changes the value, who better does proper unit handling.

    In total, five different conversions need to
    be done: receiving and sending for the current and remote devices, as
    well as during initialization:

    - When devices change their own properties, Python calls the
      descriptor's :meth:`~karabo.middlelayer.Descriptor.__set__` method.
      This converts the incoming value using the strict
      :meth:`~karabo.middlelayer.Type.toKaraboValue`, thereby checking
      that the value is valid, and attaches the current time as timestamp
      if no timestamp has been given. Then it calls
      :meth:`~karabo.middlelayer.SignalSlotable.setValue` on the
      device, which sets the value in the device's ``__dict__``, and
      also stores it in a :class:`~karabo.middlelayer.Hash` using
      :meth:`~karabo.middlelayer.Descriptor.toHash` to broadcast it
      via :meth:`~karabo.middlelayer.SignalSlotable.signalChanged`.

    - During initialization by
      :meth:`~karabo.middlelayer.Configurable.__init__`, the user-supplied
      or default value is passed to
      :meth:`~karabo.middlelayer.Type.initialize`. Its default
      implementation passes this over to the coroutine
      :meth:`~karabo.middlelayer.Type.setter`, which calls
      :func:`setattr`, which hands it over to the usual Python machinery
      just described.

      During initialization, more properties may be set than during a
      normal reconfiguration at runtime. This is why we have to treat it
      as a special case and cannot use the code path for the latter.

      This behavior also allows us to define special properties that do
      something during initialization. As an example, a ``RemoteDevice``
      parameter may already connect to the remote device upon
      initialization. Properly declaring a parameter to refer to a remote
      device instead of a mere string also adds the option for a Karabo
      global initializer to start devices in the right order.

    - Devices receive requests to change their configuration through
      :meth:`~karabo.middlelayer.Device.slotReconfigure`. This calls
      :meth:`~karabo.middlelayer.Descriptor.checkedSet` for every parameter to
      be reconfigured, which checks whether it is allowed to modify this
      parameter and raises an error if that's not the case. It converts
      the incoming value using the non-strict
      :meth:`~karabo.middlelayer.Type.toKaraboValue`, which also checks limits,
      and attaches a timestamp if supplied.
      Then it calls the coroutine :meth:`~karabo.middlelayer.Descriptor.setter`,
      and returns the result. :meth:`~karabo.middlelayer.Device.slotReconfigure`
      can then run all the coroutines to change values in parallel.

      This seemingly complicated procedure has several advantages: if the
      user tries to set a read-only (or non-existent) parameter, we
      immediately refuse the entire reconfiguration request, as it is
      obviously wrong. On the other hand, we are still able to have
      setters which take some time, as they are a coroutine.

      For nodes, :meth:`~karabo.middlelayer.Node.checkedSet` recurses into
      the node and calls :meth:`~karabo.middlelayer.Descriptor.checkedSet` for
      its members.

    - If a device wants to access another, remote device, it uses a
      subclass of :class:`~karabo.middlelayer.Proxy`. This subclass contains the
      same descriptors as devices. When a user changes a value, the
      proxy's :meth:`~karabo.middlelayer.Proxy.setValue` is called. It converts
      the value using the non-strict
      :meth:`~karabo.middlelayer.Type.toKaraboValue` and attaches the current
      time as timestamp if no other has been given. It will not set the
      value in the object, but instead send the changes to the network
      converting it to a :class:`~karabo.middlelayer.Hash` using
      :meth:`~karabo.middlelayer.Descriptor.toHash`.

    - Changes received from a remote device enter through the device's
      :meth:`~karabo.middlelayer.Device.slotChanged`. This will call the
      proxy's :meth:`~karabo.middlelayer.Proxy._onChanged` method. This will
      convert the incoming value using the non-strict
      :meth:`~karabo.middlelayer.Type.toKaraboValue` and attach the timestamp
      from the network, before entering the value into the proxy's
      ``__dict__``.

    The Karabo basetypes
    --------------------

    The Karabo basetypes were designed to ease the use of all the features
    of Karabo expected parameters, namely the fact that they have units
    and timestamps. Given that most devices in a scientific control system
    are typically written in a very rapid prototyping manner, and given
    that one of Karabo's goals is to enable many users to quickly write
    proper Karabo devices, it is obvious that most device programmers
    won't care about proper treatment of timestamps, let alone units.

    This is why we do that automatically. For the unit part, we use
    :mod:`pint`, while the timestamps part had to be written by us. The
    timestamp itself is just a :class:`~karabo.middlelayer.Timestamp`.
    In Karabo, a value is considered valid in an interval, this means the
    timestamp gives the start time after which this value is valid, until
    the next value arrives.

    Handling timestamps
    ~~~~~~~~~~~~~~~~~~~

    When a user operates on a :class:`~karabo.middlelayer.KaraboValue`, the
    timestamp of the result is the newest timestamp of all timestamps that
    take part in the operation, unless the user explicitly sets a
    different one. This is in line with the validity intervals described
    above: if a value is composed from other values, it is valid typically
    starting from the moment that the last value has become valid (this
    assumes that all values are still valid at composition time, but this
    is the responsibility of the user, and is typically already the case).

    Technically, we automatically wrap all methods of a
    :class:`~karabo.middlelayer.KaraboValue` using
    :func:`~karabo.middlelayer.basetypes.wrap_function`, which goes through all
    attributes to the wrapped function and converts the returned value
    into a :class:`~karabo.middlelayer.KaraboValue` using
    :func:`~karabo.middlelayer.basetypes.wrap`, attaching the newest timestamps
    of the attributes.

    In the case of numpy arrays, we instead override
    :meth:`~karabo.middlelayer.QuantityValue.__array_wrap__`, which is designed
    particularly to do the wrapping job.

    Handling descriptors
    ~~~~~~~~~~~~~~~~~~~~

    It might be unnecessary at first sight to store the descriptor of a
    value in the value itself, especially as it gets lost immediately when
    operating on that value.

    But the reason becomes obvious when we want to use device properties
    for anything other than their value. Most simply,
    ``help(device.speed)`` should not show the help for float values,
    but actually give help on the device's parameter.

    We use this extensively in other parts. As an example,
    ``waitUntilNew(device.speed)`` wouldn't work if ``device.speed``
    wouldn't know where it comes from. For sure, ``3 * device.speed`` has
    no relation to the original anymore, so ``waitUntilNew(3 *
    device.speed)`` wouldn't make much sense, thus it loses the descriptor.
