How to write a macro
====================

Karabo uses `Python <http://www.python.org>`_ as a macro language. So it might
be a good idea to have a look at the `Python Documentation
<http://docs.python.org/3/>`_ first, especially at the `Tutorial
<http://docs.python.org/3/tutorial/index.html>`_.

Macros are organized into classes, which then contain methods, which are the
actually executable units. They are shown in the GUI by buttons one can click
on. Macros may also have properties which can be edited like properties of
devices.

Hello World!
------------

Let's just start with the classic hello world example: Right-Click onto the
Macros section in the project and add a macro, calling it for example
helloWorld (the name must be a python literal, so no spaces are allowed).
An editor opens in the central panel, where you can enter the macro:

::

    from karabo import *  # import everything important for macros

    class HelloWorld(Macro):
        @Slot()
        def hello(self):
            print("Hello world!")

Once this macro is in a project, it will appear with its class name in the
project tree. Click on it, and you will find that on the right side there is
button labeled *hello*. Once you click it, ``Hello World!`` will be printed on
the console, that's just what one expects.

Let's now add a property. Just replace the class by

::

    class HelloYou(Macro):
        name = String()

        @Slot()
        def hello(self):
            print("Hello", self.name, "!")

which, after having entered your name into the name field and clicked onto
the hello button, will greet you.

Sometimes the property names need some more explanation, which can be done as
follows:

::

    class HelloYou(Macro):
        name = String(displayedName="User Name",
                      description="Please enter your name here")

        @Slot(displayedName="Greet User")
        def hello(self):
            print("Hello", self.name, "!")


Waiting for things to happen
----------------------------

Very often in a macro, one needs to wait for something to happen. This may
actually be much more often then one expects, since not only real-world things
like motors moving or computers calculating need to be waited for, but also
network latency. Karabo macros use the *yield from* method introduced with the
Python :mod:`asyncio` library. This means that everything that takes time has
to be prefixed with a *yield from*-expression. Those time-taking activities are
called *coroutines*. This means that the macro is stopped at that point, to be
restarted once the event being waited for happened. This is all done behind the
scenes, so you don't need to care about it.

The most simple thing to wait for is to wait for some time to elapse. This is
done with the :func:`~asyncio.sleep` coroutine, as in the following example:

::

    class Waiter(Macro):
        @Slot()
        def wait(self):
            print("Going to sleep...")
            yield from sleep(5)
            print("Just slept for 5 seconds")

This macro will print the first line, not do anything for five seconds, only to
print the second line afterwards.

Using devices
-------------

Macros would be quite boring, if one could not control devices with them. As a
first step, we need to connect to a device. This is done with the coroutine
:func:`~async.ClientBase.getDevice`, which happens to also be method of the
Macro class, so it is used as follows:

::

    @Slot()
    def some_macro(self):
        device = yield from self.getDevice("some_device")

we just left out the class definition, probably you got that. Once you have the
device, you can simply get and set its values and start its methods, as if it
was a local python object, as in

::

    device.someProperty = 7
    device.start() # as if you had pushed the start button
    print(device.someProperty)

you might have noticed that there is a not a single *yield from* expression in
this code. This means, we wait for nothing! The device properties are set and
read, and the device is started seemingly instantaneously. But this is
certainly not the case in reality. Indeed, all those commands are just send
over to the device, and the device will execute those orders hopefully soon.
But also maybe not, this is the task of the device, not of the macro to ensure.
This even leads to the seemingly weird behavior that a value that we just set
won't be there immediately - the print statement in the last example will
print the value that *someProperty* was set to before the first line, simply
because the device didn't have the time to set the value yet.

All this is solved by the coroutine :meth:`~karabo.async.waitUntil`.
Let's give an example:

::

    device.start()
    yield from waitUntil(lambda: device.state == "Started")

At the end of this code we can assure that the device is started.

Creating new devices
--------------------

In order to create a device, you first need to get its device class
with :meth:`~async.ClientBase.getClass`, which needs the server and class as parameters. This
returns an object representing the initial configuration of the device,
which you may inspect or change as desired. That done, one uses
:meth:`~async.ClientBase.startDevice`, which takes the device name and the said object to start
the device. It returns a device proxy the same way as *getDevice*.

::

    cls = yield from self.getClass("someServer", "someClass")
    cls.someParameter = 7
    obj = self.startDevice("someNewDevice", cls)

Tracking a property
-------------------

Until now, we were always just interested in the current value of a property.
Sometimes, however, one is interested in all changes of a property.
This is the case mostly for logging it, as shown in the following example:

::

    device = self.getDevice("some_device")
    p = yield from NewValue(device).some_property

now, *p* will be set to the next value of *some_property*.
