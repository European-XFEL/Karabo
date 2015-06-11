
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
    __date__ ="June 10, 2015, 03:39 PM"
    
    import time
    from karabo.device import *
    from karabo.start_stop_fsm import StartStopFsm
    
    @KARABO_CLASSINFO("ConveyorPy", "1.3")
    class ConveyorPy(PythonDevice, StartStopFsm):
    
        def __init__(self, configuration):
            # always call superclass constructor first!
            super(ConveyorPy,self).__init__(configuration)
            
        @staticmethod
        def expectedParameters(expected):
            (
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

        ##############################################
        #   Implementation of State Machine methods  #
        ##############################################
    
        def initializationStateOnEntry(self):
            self.log.INFO("Connecting to conveyer hardware, setting up motors...")
            self.set("currentSpeed", 0.0)
            
        def startAction(self):
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
            
        def stopAction(self):
            # Retrieve current value from our own device-state
            currentSpeed = self.get("currentSpeed")
            # Separate ramping into 50 steps
            decrease = currentSpeed / 50.0
            # Simulate a slow ramping down of the conveyor
            for i in range(50):
                currentSpeed -= decrease
                self.set("currentSpeed", currentSpeed)
                time.sleep(0.05)
            # Be sure to finally stand still
            self.set("currentSpeed", 0)
        
    # This entry used by device server
    if __name__ == "__main__":
        launchPythonDevice()

Consider the main steps of the code above, that are Important to mention while writing devices in Python:

1. Import all modules from karabo.device:

  .. code-block:: python

      from karabo.device import *

2. Decide whether you want to use existing FSM. In our example StartStopFsm will be used:

  .. code-block:: python

    from karabo.start_stop_fsm import StartStopFsm

  In order to use OkErrorFsm, the following import would be required:

  .. code-block:: python

    from karabo.ok_error_fsm import OkErrorFsm

3. Place decorator KARABO_CLASSINFO just before class definition. It has two parameters: "classId" and "version" similar to corresponding C++ macro. In class definition we specify that our class inherits from PythonDevice as well as from StartStopFsm (see step 2):

  .. code-block:: python

    @KARABO_CLASSINFO("ConveyorPy", "1.3")
    class ConveyorPy(PythonDevice, StartStopFsm):

4. Constructor:

  .. code-block:: python

    def __init__(self, configuration):
        # always call superclass constructor first!
        super(ConveyorPy,self).__init__(configuration)

5. Define static method expectedParameters, where you should describe what properties are available on this device.

6. Define implementation of State Machine methods. In our example: initializationStateOnEntry, startAction, stopAction.


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

