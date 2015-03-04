Writing a macro
===============

Karabo uses `Python <http://www.python.org>`_ as a macro language. So it might
be a good idea to have a look at the `Python Documentation
<http://docs.python.org/3/>`_ first, especially at the `Tutorial
<http://docs.python.org/3/tutorial/index.html>`_.
The GUI also has a built-in command line in which one can try out things.

Macros are organized into classes, which then contain methods, which are the
actually executable units. They are shown in the GUI by buttons one can click
on. Macros can be configured using so-called properties which are shown as
editable widgets in the GUI.

Hello World!
------------

Let's just start with the classic hello world example: Right-Click onto the
Macros section in the project and add a macro, calling it for example
helloWorld (the name must be a python literal, so no spaces are allowed).
An editor opens in the central panel, where you can enter the macro::

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
follows::

    class HelloYou(Macro):
        name = String(displayedName="User Name",
                      description="Please enter your name here")

        @Slot(displayedName="Greet User")
        def hello(self):
            print("Hello", self.name, "!")


Talking to devices
------------------

Macros would be quite boring, if one could not control devices with them. As a
first step, we need to connect to a device. This is done as follows::

    @Slot()
    def some_function(self):
        with self.getDevice("some_device") as device:
	    print(device.speed)

we just left out the class definition, probably you got that.
 
Once you have the device (which you can understand like a proxy or remote
control for the real device sitting somewhere), you can get and set its values
and execute its commands::

    device.someProperty = 7
    device.start() # as if you had pushed the start button
    print(device.someProperty)

There is another way of setting and getting properties and executing
commands::

   self.set(device, "someProperty", 7)
   self.execute(device, "start")
   print(self.get(device, "someProperty")

Getting a device into a variable takes some time. It makes a lot of sense
if you are changing properties or calling slots on that device all the time.
If you just want to set one property on one device, and that's it, or just
call one slot, it is simpler to just write

::

   self.set("some_device", "someProperty", 7)
   self.execute("some_device", "start")
   print(self.get("some_device", "someProperty")
    

Note that in this way you never create a handle object to the device but always
add its address to the macro function.


Monitors
--------

Until now, we discussed how to control other devices using a macro. The
opposite is also an important usecase: process the data coming from other
devices. We are not talking about heavy processing like complicated fits
here, this is what compute devices are for, but for simple calculations,
like unit conversions. This is what monitors are for.

At the beginning of the macro, we need to define which devices we want
to monitor. This is done with a ``RemoteDevice`` declaration::

    from karabo import RemoteDevice, Monitor

    class MyMonitor(Macro):
        someDevice = RemoteDevice("some_interesting_device")

Those devices will be accessible throughout the macro's lifetime. The may
also be used elsewhere in the macro.

Please don't overuse this feature. Once you monitor a device, all its
changes are sent to your macro. Depending on what device that is, this
could mean a lot of network traffic.

Now its the time to do something with the data we received. As an example,
let's look at a simple converter::

    @Monitor()
    @Float()
    def temperature(self):
        return (self.someDevice.temperature - 32) * 5 / 9

This would convert Fahrenheit temperatures from a ridiculous american device
into the usual Celsius scale.


Timeouts and errors
-------------------

While the code above looks quite simple, the things that are happening under
the hood are quite complex and deserve some attention. The first thing to be
aware of, is that we are instructing our devices remotely via the network. Any
network call needs some time and may in general not be reliable (e.g. someone
may have pulled the network cable out of the computer running the device of our
interest).

Fortunately, Karabo takes care about this and you can be sure that if a macro
operation completed, it sucessfully travelled the network and performed its
operation. In any other case, an exception will be raised explaining what went
wrong. As a consequence all macro functions above will block the program
execution until it is clear that the operation was successful. Here comes the
tricky part, Karabo has to assume a default timeout value to judge whether an
operation has failed, else the code would just block forever and no exception
would be raised. In almost any case the timeout defaults should be fine, if you
may find a situation in which you want to still change them it can be done like
this::

    # This is already advanced

    @Slot()
    def some_function(self):
        with self.getDevice("some_device", timeout=3) as device:
	    self.set(device, "someProperty", 7, timeout=4)
	    device.start(timeout=5)  # Timeout after 5s
	print(self.get(device, "someProperty", timeout=6)

Or if you are using strings for addressing::

   @Slot()
   def some_function(self):
       self.set("some_device", "someProperty", 7, timeout=3)
       self.execute("some_device", "start", timeout=4)
       print(self.get("some_device", "someProperty", timeout=5))

*TODO: Show some examples for possible exceptions*

Non-blocking operations
-----------------------

While most of the time the blocking, sequencing like behaviour of dealing with
devices is exactly what you want and anyways the safest way to perform the
control tasks, you sometimes need exactly the opposite. Imagine you have 3
devices of the same class with a ``configure()`` command that downloads some
configuration to the connected hardware and needs 4 minutes each to do the job.
If execute the ``configure()`` command as described above your macro function
runs 12 minutes! If you want to trigger downloading of the configuration for
the 3 devices in parallel you can write like::

   @Slot()
   def some_function(self):
       with \ 
               self.getDevice("some_device1") as dev1, \
               self.getDevice("some_device2") as dev2, \
               self.getDevice("some_device3") as dev3:
           dev1.executeNoWait('configure')
	   dev2.executeNoWait('configure')
           dev3.executeNoWait('configure')

Or shorter by writing::

   @Slot()
   def some_function(self):
       devices = self.getDevices()
           for device in devices:
               self.execute(device, "configure", wait=False)

*TODO: Decide about naming: ``Exec.sync`` vs. ``Exec.async`` and
``executeNoWait`` vs. ``executeAsync``*

Now the macro should finish after about 3 minutes. What has happened? Well,
Karabo issued the commands with all "operation successful" checking disabled.
It executed the commands in a "fire and forget" fashion, i.e. did *not* block
at any time. Understanding this raises new questions: How can I finally be sure
that what I did really happended? This leads to the next section of learning
how to wait on something.

Waiting for things to happen
----------------------------

Waiting for things to happen is quite simple, you can do it like this::

   @Slot()
   def some_function(self):
       with self.getDevice("some_motor") as motor:
           motor.targetPosition = 10
           motor.move()
	   self.waitUntil(lambda: motor.state == "Stopped")

This is a good example where also a timeout makes sense. Maybe the
motor never reaches its target? So you could add a timeout like that::

    try:
        self.waitUntil(lambda: motor.state == "Stopped")
    except TimeoutError as e:
        self.log.error("Motor did not reach intended state but is in {}".
                       format(motor.state))

If you want to wait until a property has changed (i.e. has been updated) you
can do it like here::

   motor.waitUntilNew(state, 10)
   print("State has updated to: {}".format(motor.state))

It is a good idea to specify a timeout for how long you are going to wait. In
the example above it is 10 seconds. If you do not provide a timeout you may
wait forever...
