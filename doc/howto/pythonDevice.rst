
.. _pythonDevice:

*********************************
 How to write a device in Python
*********************************


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
    import sys
    from karabo.device import *
    from karabo.no_fsm import (NoFsm, Worker)
    
    @KARABO_CLASSINFO("ConveyorPy", "1.3")
    class ConveyorPy(PythonDevice):
    
            
        @staticmethod
        def expectedParameters(expected):
            '''Description of device parameters statically known'''
            (
            # Button definitions
            SLOT_ELEMENT(expected).key("start")
             .displayedName("Start").description("Instructs device to go to started state")
             .allowedStates("Stopped")
             .commit()
             ,
            SLOT_ELEMENT(expected).key("stop")
             .displayedName("Stop").description("Instructs device to go to stopped state")
             .allowedStates("Started")
             .commit()
             ,
            SLOT_ELEMENT(expected).key("reset")
             .displayedName("Reset").description("Resets the device in case of an error")
             .allowedStates("Error")
             .commit()
             ,
            # Other elements
            DOUBLE_ELEMENT(expected).key("targetSpeed")
             .displayedName("Target Conveyor Speed")
             .description("Configures the speed of the conveyor belt")
             .unit(Unit.METER_PER_SECOND)
             .assignmentOptional().defaultValue(0.8)
             .reconfigurable()
             .commit()
             ,
            DOUBLE_ELEMENT(expected).key("currentSpeed")
             .displayedName("Current Conveyor Speed")
             .description("Shows the current speed of the conveyor")
             .readOnly()
             .commit()
             ,
            BOOL_ELEMENT(expected)
             .key("reverseDirection").displayedName("Reverse Direction")
             .description("Reverses the direction of the conveyor band")
             .assignmentOptional().defaultValue(False).reconfigurable()
             .allowedStates("Ok.Stopped")
             .commit()
            ,
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
            '''Initial function called after constructor but with equipped SignalSlotable under runEventLoop'''
            try:
                self.updateState("Initializing")
                self.log.INFO("Connecting to conveyer hardware, setting up motors...")
                self.set("currentSpeed", 0.0)
                self.stop()
            except Exception as e:
                print("'initialState' method failed : {}".format(e))
                self.exceptionFound("'initialState' method failed", str(e))
    
        def start(self):
            try:
                self.updateState("Starting...") # set this if long-lasting work follows
                
                # Retrieve current values from our own device-state
                tgtSpeed     = self.get("targetSpeed")
                currentSpeed = self.get("currentSpeed")
    
                # If we do not stand still here that is an error
                if currentSpeed > 0.0:
                    raise ValueError("Conveyer does not stand still at start-up")
    
                increase = tgtSpeed / 50.0
    
                # Simulate a slow ramping up of the conveyor
                for i in range(50):
                    currentSpeed += increase
                    self.set("currentSpeed", currentSpeed);
                    time.sleep(0.05)
                # Be sure to finally run with targetSpeed
                self.set("currentSpeed", tgtSpeed)
                
                self.updateState("Started")      # reached the state "Ok.Started"
                
                # start worker that will call 'hook' method repeatedly
                self.counter = 0
                self.worker = Worker(self.hook, self.timeout, self.repetition).start()
            
            except Exception as e:
                print("'start' method failed : {}".format(e))
                self.exceptionFound("'start' method failed", str(e))
            
        def hook(self):
            self.counter += 1
            self.log.INFO("*** periodicAction : counter = " + str(self.counter))
                
        def stopFsm(self):
            ''' This class has no FSM, but this method allows us to shutdown all the
                workers by hands.
            '''
            if self.worker is not None:
                if self.worker.is_running():
                    self.worker.stop()
                self.worker.join()
                self.worker = None
            
        def stop(self):
            # First shut the worker down ...
            if self.worker is not None:
                if self.worker.is_running():
                    self.worker.stop()
                self.worker.join()
                self.worker = None
    
            try:
                # Retrieve current value from our own device-state
                currentSpeed = self.get("currentSpeed")
                if currentSpeed != 0:
                    self.updateState("Stopping...") # set this if long-lasting work follows
                    # Separate ramping into 50 steps
                    decrease = currentSpeed / 50.0
                    # Simulate a slow ramping down of the conveyor
                    for i in range(50):
                        currentSpeed -= decrease
                        self.set("currentSpeed", currentSpeed)
                        time.sleep(0.05)
                    # Be sure to finally stand still
                    self.set("currentSpeed", 0)
                    
                self.updateState("Stopped")      # reached the state "Ok.Stopped"
            except Exception as e:            
                print("'stop' method failed : {}".format(e))
                self.exceptionFound("'stop' method failed", str(e))
            
        def reset(self):
            '''Put here business logic.'''
            pass
        
        # Put here more state machibe actions if needed...
       
    # This entry used by device server
    if __name__ == "__main__":
        launchPythonDevice()

Consider the main steps of the code above, that are Important to mention while writing devices in Python:

1. Import all modules from karabo.device:

  .. code-block:: python

      from karabo.device import *
  
2. Decide whether you want to use an FSM. In our example we don't use it,
   therefore we have:

   .. code-block:: python

     from karabo.no_fsm import (NoFsm, Worker)    

   Current raccomandation is to use NoFsm. If you need an FSM, read
   :ref:`this <stateMachines>` section.

3. Place decorator KARABO_CLASSINFO just before class definition. It has two 
   parameters: "classId" and "version" similar to corresponding C++ macro. 
   In class definition we specify that our class inherits from PythonDevice 
   as well as from StartStopFsm (see step 2):

   .. code-block:: python

     @KARABO_CLASSINFO("ConveyorPy", "1.3")
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

   In the constructor you always have to call first the superclass constructor.

   Then you need to register the function that will be called when the device
   is instantiated.

   Finally you have to register all the slots: in the example start, stop and
   reset.

5. Define static method expectedParameters, where you should describe what
   properties are available on this device.

6. Define implementation of initial function (in the example initialState)
   and of the slots. They will have to call self.updateState(newState) at the
   very end, in order to update device's state.

   These functions must be non-blocking: if they need to run some process which
   takes long time, they should start it in a separate thread, or even better by
   using the Worker class. See the complete example code for the Worker's 
   usage.


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

