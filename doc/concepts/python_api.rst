.. _python_api:

**************
The Python API
**************

Usage Scenarios
===============

The Python *bound* API is to be used if direct hardware interaction is to
be implemented in the Python programming language. Additionally, it allows
access to Karabo's point-to-point communication interface, by means of binding
the C++ code. Thus any processing algorithms implemented in Python,
which need to pass larger amounts of data in a pipelined fashion should
be implemented in this API:

Benefits
++++++++

Depending on application the Python *bound* API may provide for faster developement
cycles than the C++  API. In any case, the Python *bound* API is
feature-complete with respect to the C++, and frequently uses the same code
basis by binding the C++ code.

Limitations
+++++++++++

As Python is a dynamically typed language, application which require close
matching to hardware types may be better programmed in statically typed and
compile-time checked C++.

Additionally, some libraries may only be available with C or C++ interfaces. In
these cases, rather than binding the library to Python it is recommended to use
the C++ API.

Finally, Python, being an interpreted language, has performance short-comings
with respect to compiled C++ code. For most control aspects these should be
negligable, and even for high-performance processing tasks can frequently be
mitigated by using e.g. ``mod:numpy`` or ``mod:scipy`` routines. European
XFEL's calibration suite is an example of high-performance code implemented in
Python.


Programming Policies
====================

While device developers are encouraged to write *pythonic* Python, the API
purposely breaks with some conventions, to more closely resemble the C++ API
calls. This was done so that programmers switching between both languages will
not need to learn a separate set of method calls to access the same underlying
functionality.

Especially when directly accessing hardware it is considered good practice to rather be
verbose in coding, than aim for the shortest possible implementation in Python.
Accordingly, if e.g. a list comprehension significantly obscurs the functionality
implemented, consider writing a loop instead.

For documentation, it is best practice to follow documentation guidelines set
in PEP8, and document using reStructured text syntax. Specifically, you will
need to document the devices's state diagram, as otherwise it should
not be publicly released.

The "Conveyor" device
=====================

Consider the code of our device - ConveyorPy.py:

.. code-block:: python

    #!/usr/bin/env python

    __author__="john.smith@xfel.eu"
    __date__ ="November, 2014, 05:26 PM"
    __copyright__="Copyright (c) 2010-2014 European XFEL GmbH Hamburg."
                  "All rights reserved."

    import sys
    import time

    from karabo.bound import (
        KARABO_CLASSINFO, PythonDevice, Worker,
        BOOL_ELEMENT, DOUBLE_ELEMENT, OVERWRITE_ELEMENT, SLOT_ELEMENT, Unit
    )


    @KARABO_CLASSINFO("ConveyorPy", "2.0")
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
                    .allowedStates(states.STOPPED)
                    .commit(),
            SLOT_ELEMENT(expected).key("stop")
                    .displayedName("Stop")
                    .description("Instructs device to go to stopped state")
                    .allowedStates(states.STARTED)
                    .commit(),
            SLOT_ELEMENT(expected).key("reset")
                    .displayedName("Reset")
                    .description("Resets the device in case of an error")
                    .allowedStates(states.ERROR)
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
                self.updateState(states.INIT)
                self.log.INFO("Connecting to conveyor hardware, setting up motors...")
                self.set("currentSpeed", 0.0)
                self.stop()
            except Exception as e:
                self.log.ERROR("'initialState' method failed : {}".format(e))
                self.exceptionFound("'initialState' method failed", str(e))

        def start(self):
            try:
                self.updateState(states.STARTING) # set this if long-lasting work follows

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

                self.updateState(states.STARTED) # reached the state states.STARTED

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

                self.updateState(states.STOPPED) # reached the state states.STOPPED
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

1. Import needed pieces from the karabo.api package:

  .. code-block:: python

      from karabo.api import (
          KARABO_CLASSINFO, PythonDevice, Worker,
          BOOL_ELEMENT, DOUBLE_ELEMENT, OVERWRITE_ELEMENT, SLOT_ELEMENT,
          Unit
      )

2. Decide whether you want to use an FSM. In our example we don't use it,
   therefore we have:

   .. code-block:: python

     from karabo.api import Worker

   The current recommendation is to use NoFsm. If you need an FSM, read
   :ref:`this <stateMachines>` section.

3. Place decorator ``KARABO_CLASSINFO`` just before class definition. It has
   two parameters: "classId" and "version" similar to the corresponding C++
   macro. In class definition we specify that our class inherits from
   ``PythonDevice`` as well as from ``NoFsm`` (see step 2):

   .. code-block:: python

     @KARABO_CLASSINFO("ConveyorPy", "2.3")
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

   In the constructor you always have to call first the super class' constructor.

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
==================

The ``Worker`` class is suitable for executing periodic tasks. It is defined
in the ``karabo.api`` module, from which it must be imported,

.. code-block:: python

    from karabo.api import Worker

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



