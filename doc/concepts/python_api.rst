.. _python_api:

**************
The Python API
**************

Usage Scenarios
===============

The Python *bound* API is to be used if direct hardware interaction is to
be implemented in the Python programming language. Additionally, it allows
access to Karabo's point-to-point communication interface by means of binding
the C++ code. Thus any processing algorithms implemented in Python
which need to pass larger amounts of data in a pipelined fashion should
be implemented in this API.

Benefits
++++++++

Depending on the application the Python *bound* API may provide for faster
development cycles than the C++ API. In any case, the Python *bound* API is
feature-complete with respect to the C++, and frequently uses the same code
basis by binding the C++ code.

Limitations
+++++++++++

As Python is a dynamically typed language, applications which require close
matching to hardware types may be better programmed in statically typed and
compile-time checked C++.

Additionally, some libraries may only be available with C or C++ interfaces. In
these cases, rather than binding the library to Python it is recommended to use
the C++ API.

Finally, Python, being an interpreted language, has performance short-comings
with respect to compiled C++ code. For most control aspects these should be
negligible, and even for high-performance processing tasks can frequently be
mitigated by using e.g. ``mod:numpy`` or ``mod:scipy`` routines. European
XFEL's calibration suite is an example of high-performance code implemented in
Python.


Programming Policies
====================

While device developers are encouraged to write *pythonic* Python, the API
purposely breaks with some conventions to more closely resemble the C++ API
calls. This was done so that programmers switching between both languages will
not need to learn a separate set of method calls to access the same underlying
functionality.

Especially when directly accessing hardware it is considered good practice to be somewhat
verbose in coding, rather than aim for the shortest possible implementation in Python.
Accordingly, if e.g. a list comprehension significantly obscures the functionality
implemented, consider writing a loop instead.

For documentation, it is best practice to follow documentation guidelines set
in PEP8, and document using reStructured text syntax. Specifically, you will
need to document each device's state diagram, as otherwise it should
not be publicly released.

The "Conveyor" device
=====================

Consider the code of our device - ConveyorPy.py:

.. code-block:: python

    #!/usr/bin/env python
    
    __author__="name.surname@xfel.eu"
    __date__ ="November, 2014, 05:26 PM"
    __copyright__="Copyright (c) 2010-2014 European XFEL GmbH Hamburg. All rights reserved."
    
    import time
    
    from karabo.bound import (
        BOOL_ELEMENT, DOUBLE_ELEMENT, KARABO_CLASSINFO, OVERWRITE_ELEMENT, SLOT_ELEMENT, 
        PythonDevice, State, Unit
    )
    
    
    
    
    @KARABO_CLASSINFO("ConveyorPy", "1.3")
    class ConveyorPy(PythonDevice):
    
            
        @staticmethod
        def expectedParameters(expected):
            """Description of device parameters statically known"""
            OVERWRITE_ELEMENT(expected).key("state")
                    .setNewOptions(State.INIT, State.ERROR, State.STARTED, State.STOPPING, State.STOPPED, State.STARTING)
                    .setNewDefaultValue(State.INIT)
                    .commit(),
    
            # Button definitions
            SLOT_ELEMENT(expected).key("start")
                    .displayedName("Start")
                    .description("Instructs device to go to started state")
                    .allowedStates(State.STOPPED)
                    .commit(),
    
            SLOT_ELEMENT(expected).key("stop")
                    .displayedName("Stop")
                    .description("Instructs device to go to stopped state")
                    .allowedStates(State.STARTED)
                    .commit(),
    
            SLOT_ELEMENT(expected).key("reset")
                    .displayedName("Reset")
                    .description("Resets in case of an error")
                    .allowedStates(State.ERROR)
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
                    .assignmentOptional().defaultValue(False)
                    .allowedStates(State.STOPPED)
                    .reconfigurable()
                    .commit(),
    
            BOOL_ELEMENT(expected).key("injectError")
                    .displayedName("Inject Error")
                    .description("Does not correctly stop the conveyor, such "
                                 "that a Error is triggered during next start")
                    .assignmentOptional().defaultValue(False)
                    .reconfigurable()
                    .expertAccess()
                    .commit(),
    
            )
    
        def __init__(self, configuration):
            # Always call PythonDevice constructor first!
            super(ConveyorPy, self).__init__(configuration)
    
            # Register function that will be called first
            self.registerInitialFunction(self.initialize)
    
            # Register slots
            self.registerSlot(self.start)
            self.registerSlot(self.stop) 
            self.registerSlot(self.reset)
    
        def preReconfigure(self, config):
            """ The preReconfigure hook allows to forward the configuration to some connected h/w"""
    
            try:
                if config.has("targetSpeed"):
                    # Simulate setting to h/w
                    self.log.INFO("Setting to hardware: targetSpeed -> " + str(config.get("targetSpeed")))
    
                if config.has("reverseDirection"):
                    # Simulate setting to h/w
                    self.log.INFO("Setting to hardware: reverseDirection -> " + str(config.get("reverseDirection")))
    
            except RuntimeError as e:
                # You may want to indicate that the h/w failed
                self.log.ERROR("'preReconfigure' method failed : {}".format(e))
                self.updateState(State.ERROR)
    
        def initialize(self):
            """ Initial function called after constructor but with equipped SignalSlotable under runEventLoop"""
            try:
                # As the Initializing state is not mentioned in the allowed states
                # nothing else is possible during this state
                self.updateState(State.INIT)
    
                self.log.INFO("Connecting to conveyer hardware...")
    
                # Simulate some time it could need to connect and setup
                time.sleep(2.)
    
                # Automatically go to the Stopped state
                self.stop()
            except RuntimeError as e:
                self.log.ERROR("'initialState' method failed : {}".format(e))
                self.updateState(State.ERROR)
    
        def start(self):
            try:
                self.updateState(State.STARTING) # set this if long-lasting work follows
                
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
                
                self.updateState(State.STARTED) # reached the state "Started"
            
            except RuntimeError as e:
                self.log.ERROR("'start' method failed : {}".format(e))
                self.updateState(State.ERROR)
            
        def stop(self):
            try:
                # Retrieve current value from our own device-state
                currentSpeed = self.get("currentSpeed")
                if currentSpeed != 0:
                    self.updateState(State.STOPPING) # set this if long-lasting work follows
                    # Separate ramping into 50 steps
                    decrease = currentSpeed / 50.0
    
                    # Simulate a slow ramping down of the conveyor
                    for i in range(50):
                        currentSpeed -= decrease
                        self.set("currentSpeed", currentSpeed)
                        time.sleep(0.05)
                    # Be sure to finally stand still
                    if self.get("injectError"):
                        self.set("currentSpeed", 0.1)
                    else:
                        self.set("currentSpeed", 0.0)
                    
                self.updateState(State.STOPPED) # reached the state "Stopped"
            except RuntimeError as e:            
                self.log.ERROR("'stop' method failed : {}".format(e))
                self.updateState(State.ERROR)
            
        def reset(self):
            self.set("injectError", False)
            self.set("currentSpeed", 0.0)
            self.initialize()
       


Consider the main steps of the code above, which are important to
mention while writing devices in Python:

1. Import needed pieces from the karabo.bound package:

  .. code-block:: python

    from karabo.bound import (
        KARABO_CLASSINFO, PythonDevice, launchPythonDevice,
        BOOL_ELEMENT, DOUBLE_ELEMENT, OVERWRITE_ELEMENT, SLOT_ELEMENT, Unit, State
    )

2. Decide whether you want to use an FSM. In our example we don't use it,
   therefore we have:

   .. code-block:: python

     from karabo.bound import Worker

   The current recommendation is to use NoFsm. If you need an FSM, read
   :ref:`this <stateMachines>` section.

3. Place the decorator ``KARABO_CLASSINFO`` just before class definition. It has
   two parameters: "classId" and "version" similar to the corresponding C++
   macro. In class definition we specify that our class inherits from
   ``PythonDevice`` as well as from ``NoFsm`` (see step 2):

   .. code-block:: python

     @KARABO_CLASSINFO("ConveyorPy", "2.3")
     class ConveyorPy(PythonDevice):

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

   In the constructor you always have to call the super class's constructor first.

   Then you need to register the function that will be called when the device
   is instantiated.

   Finally you have to register all the slots: in the example start,
   stop and reset.

5. Define the static method ``expectedParameters``, where you should describe what
   properties are available on this device.

6. Define the implementation of initial function (in the example ``initialState``)
   and of the slots. They will have to call ``self.updateState(newState)`` at
   the very end, in order to update device's state.

   These functions must be non-blocking: if they need to run some process which
   takes a long time, they should start it in a separate thread, or even better by
   using the ``Worker`` class. See the complete example code for the Worker's
   usage.


The "Worker" class
==================

The ``Worker`` class is suitable for executing periodic tasks. It is defined
in the ``karabo.bound`` module, from which it must be imported,

.. code-block:: python

    from karabo.bound import Worker

It can be instantiated and started like this:

.. code-block:: python

    self.counter = 0
    self.timeout = 1000  # milliseconds
    self.repetition = -1  # forever
    self.worker = Worker(self.hook, self.timeout, self.repetition).start()

The 'repetition' parameter will specify how many times the task has to
be executed (-1 means 'forever'), the 'timeout' parameter will set the
interval between two calls and ``self.hook`` is the callback function defined
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



