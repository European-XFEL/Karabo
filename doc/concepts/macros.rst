*****************
Macros & Monitors
*****************

Macros are short pieces of code, which exist as part of project configurations.
Macros may be executed either from the GUI or from the command line, in either
case they are executed on a macro server.


Usage Scenarios
===============

Macros
++++++

Macros should be used for the following recurring tasks:

* small programs to be run from ``iKarabo`` which specialize generic iKarabo functionality, e.g. ``listAllMotors()``
* recurring automizations, frequently limited to one experiment or a specific beamtime
* test implementations of procedures, prototyping.

Benefits
++++++++

Macros have the following benefits

* Are aimed to be written by non-(expert)-programmers, i.e. users, instrument scientists, engineers
* Simple syntax, while the same as middle-layer device only a small subset of functionality
  is commonly needed.
* Remote devices accessed by the macro should usually be locked


From the GUI you need to only click the ``play`` button, there is no need to instantiate a device.

.. ifconfig:: includeDevInfo is True

    .. note::

        In terms of implementation this means that Macros written in the GUI have a deploy/save button. This
        will save the macro (or update an existing one) in the project. When clicking on a macro in the project
        it either is already instantiated or automatically is instantiated. The user thus has the interface and
        can start it by clicking execute (possibly after having adjusted parameters).

        For the CLI I suggest that only named arguments or kwargs are allowed. Macros should also have a help
        function, which lists their expected parameters. For this we can use standard iPython shorthands.

        To allow for the above command line interface functionality I suggest to append macros to a Macro
        object which is empty upon initialization.


Limitations
+++++++++++

Macros by concept should be restricted to simple, possibly recurring tasks, which involve
interaction with other devices (*driver* or *middle-layer*). Macros should be simple enough
to be coded in a single execution block: the macro's ``execute`` method. If a task at hand is
more complex it should be written in terms of a middle-layer device, which is bound to
a device-server, and the configuration of which can be versioned as described in
:ref:`project_conf``.

The ``execute`` method will automatically be rendered as a button in the GUI. Additionally,
a cancel button is created. You can specify in the macro's options if it initiates into the
executing state or if an explicit action by clicking ``Execute`` is required.

In addition to overwriting the ``execute`` method you may specify unbound auxiliary functions.
The following piece of code thus constitutes a valid macro:

.. code-block:: Python

    from karabo.middlelayer import Macro

    def cToF(T):
        return T * 9 / 5 + 32

    def fToC(T):
        return (T - 32) * 5 / 9

    class MyMacro(Macro):

        deviceId = String(displayedName = "Fahrenheit Thermometer")

        def execute(self):
            someDevice = connectDevice(self.deviceId)
            try:
                someDevice.temperature = cToF(self.temperature)
                someDevice.heat()
            except CancellationError as e:
                self.log.warning("Heating has been cancelled")


Conversely, the following macro is bad practice:

..  code-block:: Python

    from karabo.middlelayer import Macro

    class MyMacro(Macro):

        def run():
            try:
                self.foo()
            except CancellationError as e:
                self.log.warning("MyMacro has been cancelled")

        def foo():
            print("Foo")



Additionally, only one instance of a given macro may run at a time. You cannot have multiple instances of the same
macro class running in the same domain!


Programming Policies
====================

In addition to the *single execution block* policy described above, the same policies
as mentioned for middle-layer devices apply: try to write concise code, which is explicit
enough to be read and understood by other people.

Given that macros have a limited evolution path in terms of complexity, consider whether
the problem you are trying to tackle might better be implemented in a middle-layer device.

Finally, as macros often facilitate small recurring tasks, there is a chance that someone
has faced the same problem before and thus a macro or middle-layer device for a similar
problem is already available. Search the repository first, before reinventing the wheel.

Conversely, write your macros in such a generic fashion that they may be useful to others
as well

..  code-block:: Python

    def execute(self):
         motorDevice1 = connectDevice("motorDevice1")
         motorDevice2 = connectDevice("motorDevice2")

            #do stuff
            ....

is easier to modify by another user than

..  code-block:: Python

    def execute(self):
        motorDevice1 = connectDevice("motorDevice1")
        ...
        #do stuff
        ...
        motorDevice1 = connectDevice("motorDevice2")
        ...
        #do more stuff
        ...


Device Locking
++++++++++++++

Remote devices accessed by the macro can be configured to be automatically locked for the duration of
the macro. You can control this by setting ``lock=True`` on the ``connectDevice`` call.

.. code-block:: Python

    def execute(self):
        myLockedProxy = connectDevice("myDevice")
        myProxy = connectDevice("myDevice")

        with lock(myLockedProxy):

            b = myLockedProxy.a()
            myLockedProxy.move()
            myLockedProxy.a = 1



Monitors
========

Quite often a middle-layer device needs to execute some action if a property on a remote
device changes. For handling such scenarios Karabo provides in *monitors*. A monitor is a
function which gets executed when a remote property has changed::

    from karabo.api_2 import Device, Monitor

    class MyMonitor(Device):
        someDevice = getDevice("some_interesting_device")

        @Monitor
        @Float(displayedName= "Temperature", unit=units.DEGREE_CELSIUS)
        def temperature(self):
            return (self.someDevice.temperature - 32) * 5 / 9

Devices specified in the above way are accessible throughout the macro's lifetime. They may
thus also be used in the macro's ``execute`` method.

.. warning::

	Don't overuse this feature. Once you monitor a device, all its
	changes are sent to your macro. Depending on what device that is, this
	could mean a lot of network traffic.

