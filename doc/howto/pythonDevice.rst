
.. _pythonDevice:

*******************************
How to write a device in Python
*******************************

.. contents::
    :depth: 2

C++ like API based on Python bindings
=====================================


The "Conveyor" device
---------------------

Consider the code of our device - ConveyorPy.py:

.. code-block:: python

    #!/usr/bin/env python

    __author__="john.smith@xfel.eu"
    __date__ ="November, 2014, 05:26 PM"
    __copyright__="Copyright (c) 2010-2014 European XFEL GmbH Hamburg. All rights reserved."

    import sys
    import time

    from karabo.api_1 import (
        KARABO_CLASSINFO, PythonDevice, Worker,
        BOOL_ELEMENT, DOUBLE_ELEMENT, OVERWRITE_ELEMENT, SLOT_ELEMENT, Unit
    )


    @KARABO_CLASSINFO("ConveyorPy", "1.3")
    class ConveyorPy(PythonDevice):
        @staticmethod
        def expectedParameters(expected):
            """ Description of device parameters statically known
            """
            (
            # Button definitions
            SLOT_ELEMENT(expected).key("start")
                    .displayedName("Start")
                    .description("Instructs device to go to started state")
                    .allowedStates("Stopped")
                    .commit(),
            SLOT_ELEMENT(expected).key("stop")
                    .displayedName("Stop")
                    .description("Instructs device to go to stopped state")
                    .allowedStates("Started")
                    .commit(),
            SLOT_ELEMENT(expected).key("reset")
                    .displayedName("Reset")
                    .description("Resets the device in case of an error")
                    .allowedStates("Error")
                    .commit(),
            # Other elements
            DOUBLE_ELEMENT(expected).key("targetSpeed")
                    .displayedName("Target Conveyor Speed")
                    .description("Configures the speed of the conveyor belt")
                    .unit(Unit.METER_PER_SECOND)
                    .assignmentOptional().defaultValue(0.8)
                    .reconfigurable()
                    .commit(),
            DOUBLE_ELEMENT(expected).key("currentSpeed")
                    .displayedName("Current Conveyor Speed")
                    .description("Shows the current speed of the conveyor")
                    .readOnly()
                    .commit(),
            BOOL_ELEMENT(expected).key("reverseDirection")
                    .displayedName("Reverse Direction")
                    .description("Reverses the direction of the conveyor band")
                    .assignmentOptional().defaultValue(False).reconfigurable()
                    .allowedStates("Ok.Stopped")
                    .commit(),
        )

        def __init__(self, configuration):
            # Always call PythonDevice constructor first!
            super(ConveyorPy, self).__init__(configuration)
            # Register function that will be called first
            self.registerInitialFunction(self.initialState)
            # Register slots
            self.registerSlot(self.start)
            self.registerSlot(self.stop)
            self.registerSlot(self.reset)
            self.worker = None
            self.timeout = 1000  # milliseconds
            self.repetition = -1 # forever

        def initialState(self):
            """ Initial function called after constructor but with equipped
                SignalSlotable under runEventLoop
            """
            try:
                self.updateState("Initializing")
                self.log.INFO("Connecting to conveyor hardware, setting up motors...")
                self.set("currentSpeed", 0.0)
                self.stop()
            except Exception as e:
                self.log.ERROR("'initialState' method failed : {}".format(e))
                self.exceptionFound("'initialState' method failed", str(e))

        def start(self):
            try:
                self.updateState("Starting") # set this if long-lasting work follows

                # Retrieve current values from our own device-state
                tgtSpeed = self.get("targetSpeed")
                currentSpeed = self.get("currentSpeed")

                # If we do not stand still here that is an error
                if currentSpeed > 0.0:
                    raise ValueError("Conveyer does not stand still at start-up")

                # Separate ramping into 50 steps
                increase = tgtSpeed / 50.0

                # Simulate a slow ramping up of the conveyor
                for i in range(50):
                    currentSpeed += increase
                    self.set("currentSpeed", currentSpeed);
                    time.sleep(0.05)
                # Be sure to finally run with targetSpeed
                self.set("currentSpeed", tgtSpeed)

                self.updateState("Started") # reached the state "Started"

                # start worker that will call 'hook' method repeatedly
                self.counter = 0
                self.worker = Worker(self.hook, self.timeout, self.repetition).start()

            except Exception as e:
                self.log.ERROR("'start' method failed : {}".format(e))
                self.exceptionFound("'start' method failed", str(e))

        def hook(self):
            self.counter += 1
            self.log.INFO("*** periodicAction : counter = " + str(self.counter))

        def stopFsm(self):
            """ This class has no FSM, but this method allows us to shutdown
                all the workers by hand.
            """
            self._stopWorker()

        def stop(self):
            # First shut the worker down ...
            self._stopWorker()

            try:
                # Retrieve current value from our own device-state
                currentSpeed = self.get("currentSpeed")
                if currentSpeed != 0:
                    self.updateState("Stopping") # set this if long-lasting work follows
                    # Separate ramping into 50 steps
                    decrease = currentSpeed / 50.0
                    # Simulate a slow ramping down of the conveyor
                    for i in range(50):
                        currentSpeed -= decrease
                        self.set("currentSpeed", currentSpeed)
                        time.sleep(0.05)
                    # Be sure to finally stand still
                    self.set("currentSpeed", 0)

                self.updateState("Stopped") # reached the state "Stopped"
            except Exception as e:
                self.log.ERROR("'stop' method failed : {}".format(e))
                self.exceptionFound("'stop' method failed", str(e))

        def reset(self):
            """ Put business logic here.
            """
            pass

        def _stopWorker(self):
            if self.worker is not None:
                if self.worker.is_running():
                    self.worker.stop()
                self.worker.join()
                self.worker = None

        # Put more state machine actions here if needed...


Consider the main steps of the code above, that are important to
mention while writing devices in Python:

1. Import needed pieces from the karabo.api_1 package:

  .. code-block:: python

      from karabo.api_1 import (
          KARABO_CLASSINFO, PythonDevice, Worker,
          BOOL_ELEMENT, DOUBLE_ELEMENT, OVERWRITE_ELEMENT, SLOT_ELEMENT,
          Unit
      )

2. Decide whether you want to use an FSM. In our example we don't use it,
   therefore we have:

   .. code-block:: python

     from karabo.api_1 import Worker

   The current recommendation is to use NoFsm. If you need an FSM, read
   :ref:`this <stateMachines>` section.

3. Place decorator ``KARABO_CLASSINFO`` just before class definition. It has
   two parameters: "classId" and "version" similar to the corresponding C++
   macro. In class definition we specify that our class inherits from
   ``PythonDevice`` as well as from ``NoFsm`` (see step 2):

   .. code-block:: python

     @KARABO_CLASSINFO("ConveyorPy", "1.3")
     class ConveyorPy(PythonDevice, NoFsm):

4. Constructor:

   .. code-block:: python

     def __init__(self, configuration):
         # always call superclass constructor first!
         super(ConveyorPy,self).__init__(configuration)
         # Register function that will be called first
         self.registerInitialFunction(self.initialState)
         # Register slots
         self.registerSlot(self.start)
         self.registerSlot(self.stop)
         self.registerSlot(self.reset)
         self.worker = None
         self.timeout = 1000  # milliseconds
         self.repetition = -1 # forever

   In the constructor you always have to call first the superclass constructor.

   Then you need to register the function that will be called when the device
   is instantiated.

   Finally you have to register all the slots: in the example start,
   stop and reset.

5. Define static method ``expectedParameters``, where you should describe what
   properties are available on this device.

6. Define implementation of initial function (in the example ``initialState``)
   and of the slots. They will have to call ``self.updateState(newState)`` at
   the very end, in order to update device's state.

   These functions must be non-blocking: if they need to run some process which
   takes long time, they should start it in a separate thread, or even better by
   using the ``Worker`` class. See the complete example code for the Worker's
   usage.


The "Worker" class
------------------

The ``Worker`` class is suitable for executing periodic tasks. It is defined
in the ``karabo.api_1`` module, from which it must be imported,

.. code-block:: python

    from karabo.api_1 import Worker

It can be instantiated and started like this:

.. code-block:: python

    self.counter = 0
    self.timeout = 1000  # milliseconds
    self.repetition = -1  # forever
    self.worker = Worker(self.hook, self.timeout, self.repetition).start()

The 'repetition' parameter will specify how many times the task has to
be executed (-1 means 'forever'), the 'timeout' parameter will set the
interval between two calls, ``self.hook`` is the callback function defined
by the user, for example:

.. code-block:: python

    def hook(self):
        self.counter += 1
        self.log.INFO("*** periodicAction : counter = " + str(self.counter))

The worker can then be stopped like this:

.. code-block:: python

    if self.worker is not None:
        if self.worker.is_running():
            self.worker.stop()
        self.worker.join()
        self.worker = None


Pythonic API based on native Python
===================================

A device is not much more than a macro that runs on a server for a longer
time. So it is written mostly in the same way. The biggest difference
is that it inherits from :class:`karabo.api_2.PythonDevice` instead of
:class:`karabo.api_2.Macro`. But the main difference is actually that
a macro is something you may write quick & dirty, while a device should be
written with more care. To give an example:

.. code-block:: python

    from karabo.api_2 import PythonDevice

    class TestDevice(PythonDevice):
        __version__ = "1.3 1.4"

As you see, we avoid using star-imports but actually import everything by
name. As the next thing there is a *__version__* string. This is not the
version of your device, but the Karabo versions your device is supposedly
compatible to.


Starting a project using the ``karabo`` script
==============================================

Start by creating a new device project using the ``karabo`` script and the
minimal pythonDevice template:

.. code-block:: shell

    $ # run karabo help new for a description of the parameters
    $ karabo new PACKAGE_NAME PACKAGE_CATEGORY pythonDevice minimal CLASS_NAME [-noSvn]

A pythonDevice project created from the template can be built in a couple of
different ways. The first way is by using the ``karabo`` script again:

.. code-block:: shell

    $ # Note that PACKAGE_NAME and PACKAGE_CATEGORY are the same as above
    $ karabo rebuild PACKAGE_NAME PACKAGE_CATEGORY

Building the device in this way automatically installs it to the
run/servers/pythonDeviceServer/plugins directory. If you would like to choose
where the device is installed, read below about the self-extracting shell script.

To build a redistributable self-extracting installer for a pythonDevice,
navigate to the device's source code directory and run the following command:

.. code-block:: shell

    $ ./build-package.sh

A self-extracting shell script will be saved by the build command. It's in a
deeply nested directory in the "package" directory in the device's directory.
Run this script to install the device at a location of your choice.

The third way to build a pythonDevice enables development of the device's code
without the need to reinstall after making changes to the code. To use this
method, you should first navigate to the device's source directory. Then run the
following command:

.. code-block:: shell

    $ ./build-package.sh develop

That will make a link to the device's source code directory so that it is
visible to the device server's plugin discovery code. Note that currently
running device servers will not immediately see a device installed in this way.
The test device server should be restarted after running the above command.
After restarting the server, further changes to the device's source code will be
immediately available without an installation step. You can simply instantiate
a new instance of the device to get the changes.
**You should be careful to stop any devices that were instantiated with older
versions of the code.**
Note that you will only see the results of changes in newly created device
instances and not in, for example, the configuration associated with the device
class.

When you are done developing the device, you should remove this link with the
following command:

.. code-block:: shell

    $ # The only difference is the "-u" argument at the end
    $ ./build-package.sh develop -u


Updating an older ``PythonDevice`` project
==========================================

If your device project was created from the pythonDevice minimal template but
it *doesn't* have a setup.py file (karaboFramework 1.3 and earlier), it can
be converted to the newer structure automatically. For this, you use the
``convert-karabo-device-project`` program which comes with a Karabo framework
installation:

.. code-block:: shell

    $ # Assuming the Karabo bin directories aren't in your path...
    $ $KARABO/extern/bin/convert-karabo-device-project <path-to-project>

The result of running this program is fairly straightforward:

* All Python source files in the project's 'src' directory are imported and
  checked for the presence of a subclass of ``PythonDevice``.
* All Python source files in the project's 'src' directory are moved to a new
  package directory which is created in the 'src' directory.
* A 'setup.py' file is added to the project's root directory. This file defines
  an entry point for each ``PythonDevice`` subclass that was found when scanning
  the project's sources.
* A current version of the 'build-package.sh' script is added to the project's
  root directory. The old 'build-package.sh' (if it exists) is moved to a file
  named 'build-package-old.sh'.

Once converted, the above instructions relating to invocation of the
'build-package.sh' script apply. Your device will build as a self-extracting
shell script when using the ``karabo`` script or if you like, you can build
in "development" mode too.


``setup.py`` and Device entry points
====================================

Starting with Karabo framework version 1.5.0, each Python device project should
use a ``setup.py`` script to package itself for installation on both developer
and user systems.

Exhaustive documentation for the ``setuptools`` library and ``setup.py``
scripts can be found `here <https://pythonhosted.org/setuptools/setuptools.html>`_

To start, here is a sample ``setup.py`` script from a project which contains a
single device:

.. code-block:: python

    #!/usr/bin/env python

    from setuptools import setup, find_packages

    long_description = """\
    Surrounded by rocky, lifeless worlds and in need of a quick place to land
    your ship? Never fear! The Genesis Device is for you!

    * WARNING: Not to be used on inhabited planets. Point away from face when
    using. May cause grey goo.
    """

    setup(name='genesisDevice',
          version='1.0.5',
          author='Joe Smith',
          author_email='joe.smith@xfel.eu',
          description='Genesis Device: Rapid Planet Terraformer',
          long_description=long_description,
          url='http://en.memory-alpha.wikia.com/wiki/Genesis_Device',
          package_dir={'': 'src'},
          packages=find_packages('src'),
          entry_points={
              'karabo.python_device.api_1': [
                  'Genesis = genesisDevice.Genesis:GenesisTorpedo',
              ],
          },
          package_data={'': ['*.dat']},
          requires=['roddenberry >= 1.0'],
          )

The ``setup.py`` really only needs to call the ``setup`` function provided by
``setuptools``. For more complicated packages, C-API modules can be compiled or
versioning schemes can be implemented in the ``setup.py`` script. For most
Karabo devices, this simple example should be sufficient.

The most important keyword arguments are ``name``, ``packages``, and
``entry_points``.

``name`` is the name of the package. This should be obvious.

``packages`` is a list of all the Python packages that are part of this project.
For a simple device, this list might only have a single item. In this example,
that would be ``['genesisDevice']``. For more complicated projects, this list
should be a complete package hierarchy. For instance:
``['genesisDevice', 'genesisDevice.subPackage', 'genesisDevice.otherSub']``
would describe a Python package with two subpackages. The ``find_packages``
function provided by ``setuptools`` handles the creation of this package list
easily. In the case of a project based on the pythonDevice minimal template, the
packages are just directories contained within the 'src' directory which are
themselves Python packages (ie: They contain an ``__init__.py`` file).

``entry_points`` is a dictionary of classes which can be loaded by a device
server. The key used here is ``'karabo.python_device.api_1'``, which specifies
devices using the C++ like API. For the Pythonic API, the key is
``'karabo.python_device.api_2'``. The value is a list of strings which describe
the individual device entry points. The basic format is:

.. code-block:: python

    'UNIQUE_NAME = PACKAGE.[SUBPACKAGE.SUBPACKAGE.]SUBMODULE:CLASS_NAME'

``UNIQUE_NAME`` is some unique identifier for the device. After the equal-sign,
a path to the device's class is given. You can think of it as something like an
``import`` statement. The equivalent for the example would be:

.. code-block:: python

    from genesisDevice.Genesis import GenesisTorpedo

When the device server is running, it periodically checks its namespace
(api_1 or api_2) for all available device entry points. It attempts to import
each device. Every device which can be imported and which is a subclass of
``PythonDevice`` will be made available for instantiation by the server.

Some other potentially useful keyword arguments for the ``setup`` function are
``package_data`` and ``requires``. ``package_data`` is a dictionary of file
globs which allows for inclusion of non-Python sources in a built package.
``requires`` is a list of strings which denote third-party Python packages
which are required for the device to run. These arguments and others are
explained more completely in the ``setuptools``
`documentation <https://pythonhosted.org/setuptools/setuptools.html>`_
