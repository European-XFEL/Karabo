
.. _pythonDevice:

*********************************
 How to write a device in Python
*********************************
.. sectionauthor:: Andrea Parenti <andrea.parenti@xfel.eu>

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
    
    import time

    from karabo.decorators import KARABO_CLASSINFO
    from karabo.device import PythonDevice, launchPythonDevice
    from karathon import (
        BOOL_ELEMENT, DOUBLE_ELEMENT, OVERWRITE_ELEMENT, SLOT_ELEMENT,
        Unit,
    )


    @KARABO_CLASSINFO("ConveyorPy", "1.3")
    class ConveyorPy(PythonDevice):
        
        @staticmethod
        def expectedParameters(expected):
            '''Description of device parameters statically known'''
            (
            OVERWRITE_ELEMENT(expected).key("state")
                    .setNewOptions("Initializing,Error,Started,Stopping,Stopped,Starting")
                    .setNewDefaultValue("Initializing")
                    .commit(),

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
                    .description("Resets in case of an error")
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
                    .assignmentOptional().defaultValue(False)
                    .allowedStates("Stopped")
                    .reconfigurable()
                    .commit(),

            BOOL_ELEMENT(expected).key("injectError")
                    .displayedName("Inject Error")
                    .description("Does not correctly stop the conveyor, such that a Error is triggered during next start")
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
            '''The preReconfigure hook allows to forward the configuration to some connected h/w'''

            try:
                if config.has("targetSpeed"):
                    # Simulate setting to h/w
                    self.log.INFO("Setting to hardware: targetSpeed -> " + str(config.get("targetSpeed")))

                if config.has("reverseDirection"):
                    # Simulate setting to h/w
                    self.log.INFO("Setting to hardware: reverseDirection -> " + str(config.get("reverseDirection")))

            except Exception as e:
                # You may want to indicate that the h/w failed
                self.log.ERROR("'preReconfigure' method failed : {}".format(e))
                self.updateState("Error")

        def initialize(self):
            '''Initial function called after constructor but with equipped SignalSlotable under runEventLoop'''
            try:
                # As the Initializing state is not mentioned in the allowed states
                # nothing else is possible during this state
                self.updateState("Initializing")

                self.log.INFO("Connecting to conveyer hardware...")

                # Simulate some time it could need to connect and setup
                time.sleep(2.)

                # Automatically go to the Stopped state
                self.stop()
            except Exception as e:
                self.log.ERROR("'initialState' method failed : {}".format(e))
                self.updateState("Error")

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
            
            except Exception as e:
                self.log.ERROR("'start' method failed : {}".format(e))
                self.updateState("Error")
            
        def stop(self):
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
                    if self.get("injectError"):
                        self.set("currentSpeed", 0.1)
                    else:
                        self.set("currentSpeed", 0.0)
                    
                self.updateState("Stopped") # reached the state "Stopped"
            except Exception as e:            
                self.log.ERROR("'stop' method failed : {}".format(e))
                self.updateState("Error")
            
        def reset(self):
            '''Put here business logic.'''
            self.set("injectError", False)
            self.set("currentSpeed", 0.0)
            self.initialize()
       
    # This entry used by device server
    if __name__ == "__main__":
        launchPythonDevice()


Consider the main steps of the code above, that are important to
mention while writing devices in Python:

1. Import needed pieces from the karabo and karathon packages:

  .. code-block:: python

      from karabo.decorators import KARABO_CLASSINFO
      from karabo.device import PythonDevice, launchPythonDevice
      from karathon import (
          BOOL_ELEMENT, DOUBLE_ELEMENT, OVERWRITE_ELEMENT, SLOT_ELEMENT,
          Unit
      )
  
2. Decide whether you want to use an FSM. In our example we don't use
   it, which is the current raccomandation is to use NoFsm. If you
   need an FSM, read :ref:`this <stateMachines>` section.

3. Place decorator KARABO_CLASSINFO just before class definition. It has two 
   parameters: "classId" and "version" similar to corresponding C++ macro. 
   In class definition we specify that our class inherits from PythonDevice 
   (see step 2):

   .. code-block:: python

     @KARABO_CLASSINFO("ConveyorPy", "1.3")
     class ConveyorPy(PythonDevice):

4. Define static method expectedParameters, where you should describe what
   properties are available on this device.

5. Constructor:

   .. code-block:: python

        def __init__(self, configuration):
            # Always call PythonDevice constructor first!
            super(ConveyorPy, self).__init__(configuration)

            # Register function that will be called first
            self.registerInitialFunction(self.initialize)

            # Register slots
            self.registerSlot(self.start)
            self.registerSlot(self.stop) 
            self.registerSlot(self.reset)

   In the constructor you always have to call first the superclass constructor.

   Then you need to register the function that will be called when the device
   is instantiated.

   Finally you have to register all the slots: in the example 'start',
   'stop' and 'reset'.

6. Define implementation of the 'preReconfigure' and 'postReconfigure'
   functions, which are called after a reconfiguration request was
   received, respectively before and after it has been merged into the
   device’s state.

7. Define implementation of initial function (in the example
   'initialize') and of the slots. They will have to call
   self.updateState(newState) at the very end, in order to update
   device's state.

   These functions must be non-blocking: if they need to run some
   process which takes long time, they should start it in a separate
   thread, or even better by using the Worker class.


The "Worker" class
------------------

The Woker class is suitable for executing periodic tasks. It is defined
in the karabo.no_fsm module, from which it must be imported,

.. code-block:: python

    from karabo.no_fsm import Worker

It can be instantiated and started like this:

.. code-block:: python

    self.counter = 0
    self.timeout = 1000  # milliseconds
    self.repetition = -1  # forever
    self.worker = Worker(self.hook, self.timeout, self.repetition).start()

The 'repetition' parameter will specify how many times the task has to
be executed (-1 means 'forever'), the 'timeout' parameter will set the
interval bewteen two calls, self.hook is the callback function defined
by the user, for example:

.. code-block:: python

    def hook(self):
        self.counter += 1
        self.log.INFO("*** periodicAction : counter = " + str(self.counter))

The worker can then be stopped like this

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
is that it inherits from :class:`karabo.device.Device` instead of
:class:`karabo.device.Macro`. But the main difference is actually that
a macro is something you may write quick&dirty, while a device should be
written with more care. To give an example:

.. code-block:: python

    from karabo import Device

    class TestDevice(Device):
        __version__ = "1.3 1.4"

As you see, we avoid using star-imports but actually import everything by
name. As the next thing there is a *__version__* string. This is not the
version of your device, but the Karabo versions your device is supposedly
compatible to.

