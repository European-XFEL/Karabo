.. _open_closed_loop:

******************************
Open and Closed Loop Operation
******************************

Hardware devices can generally be classified into closed and open-loop control systems.
Open loop systems translate to a set value, but do not take any feedback on their state
or other environmental conditions into account while doing so.

Open Loop
=========

If a device is operated in an open loop it does not receive sensory input directing the
execution of an action. Examples for open loop operation are e.g. starting a motor,
opening a valve or turning off a pump. In pseudo-code an open loop control can be written
as:

.. code-block:: Basic

   IF (time_for_action = TRUE) THEN 
       FOR V IN target_values DO
           set_value(V)
       END FOR
       take_action(X)
   END 
   
In Karabo open loop devices are always operated by a combination of setting none to
many target values to control the action and then executing a command specifying an action.
Target value-less examples are opening a shutter, closing a valve, starting a pump.
Examples where target-values are involved are that for a stepper motor first the target
value is set, and then the motor drives to the target, or a heater warming an element
to a specified temperature.

.. note::

	The latter examples often implement a closed, regulator loop on the PLC side,
	but in terms of device software are treated as open loop devices as the software is
	not involved in the regulatory behavior.

Discrete Closed Loop
====================

Closed loop systems in contrast take sensory input into account, by calculating a
deviation from the target value and the current input, and then adjusting the input value
through a regulatory function to converge towards the target value. Closed loop behavior
may be discrete or continuous. In the first case a sensory value is used to decide the
course of action, afterwards the system is not further controlled:

.. code-block :: Basic

   IF (time_for_action = TRUE) THEN 
       measure(Y) 
       IF (Y = specified_condition) THEN 
           FOR V IN target_values DO
               set_value(V)
           END FOR
           take_action(X) 
       END
   END 
   
Examples for discrete closed loop devices can be found in interlock systems, e.g. when
a chiller may only be started if the detector vessel has been priorly evacuated. As is
evident from the pseudo-code the separation between command and target value assignment is
similar to the open loop example, except for the initial conditional check, which would
be implemented in Karabo either through state-checking of commands, validation of device
properties upon command execution, or in hardware through direct sensory input.

Continous Closed Loop
=====================

Finally, devices may be operated in continuous closed loop behavior, i.e. a continuous
sensory input is used to alter the input signal with the goal of minimizing the deviation
of the output from the target value. Examples of such devices are motors holding a
specific velocity or position, a heater or chiller with feedback from temperature sensors
or a power supply ramping a voltage or current. In pseudo-code such behavior can be
expressed as

.. code-block:: Basic

    FOR V IN target_values DO
        set_value(V)
    END FOR
    WHILE (Y <> specified_condition) 
        take_action(X) 
        measure(Y) 
        wait(Z)
    END WHILE

As stated earlier, many hardware devices implementing closed loops can actually be exposed
as open loop systems in the device software. This means that in Karabo operating in 
a closed-loop mode should generally still accept changes of the target value as a 
combination of setting the target value and the executing a command to update the target
value on the hardware. For devices which would implicitly act on a changed property this
behavior should be maintained by delay of the property setting to the command execution:

.. code-block:: Python

   from karabo.api_1 import PythonDevice

   class StepperMotor(PythonDevice):
   
       @staticmethod
       def expectedParameters(expected):
           ...
         
       def __init__(self, config):
           self.registerInitialFunction(self.initialization)
                  
       def initialization(self):
           self.KARABO_SLOT(self.moveToTarget)
       
       def updateHardware(self, config):
           ...
           # transfer value to hardware
       
       def callMove(self):
           ...
           #call move to target on Hardware
   
       def onConfigurationChanged(self, config):
           if self.get("closedLoopMode") == True:
              self._delayedConfig = config
           else:
              self.updateHardware(config)
          
       def moveToTarget(self):
           self.updateState(states.MOVING)
           if self.get("closedLoopMode") == True:
              updateHardware(self._delayedConfig)
              self.updateState(states.FOLLOWING)
           else:
              self.callMove()
              self.updateState(states.ON_TARGET)



As can be seen from the code, the update of the hardware configuration is delayed to the
moveToTarget command if the device is in closed-loop mode.

.. warning::

   The same concept should be used for devices implementing a closed loop in software.
   The software loop should continuously evaluate against a cached configuration, updated
   upon command execution and not property assignment.
   
.. code-block:: Python

   from karabo.api_1 import PythonDevice, Worker
   from threading import Lock
   import copy

   class SoftwarePidDevice(PythonDevice):
   
       @staticmethod
       def expectedParameters(expected):
           ...
         
       def __init__(self, config):
           self.registerInitialFunction(self.initialization)
           self._cachedConfig = config
           self._delayedConfig = config
           self._worker = Worker(self.pidLoop)
                 
       def initialization(self):
           self.KARABO_SLOT(self.updateTarget)
           self._worker.start()
           self.updateState(states.FOLLOWING)
           
       def preDestruction(self):
           self._worker.stop()
           self._worker.join()
           
       def slotReconfigure(self, config):
           for k in config:
               self._delayedConfig.set(k, config.get(k))
               
       def updateTarget(self):
           self.updateState(states.TRANSITION)
           self._updateLock.acquire()
           self._cachedConfig = copy.copy(self._delayedConfig)
           self._updateLock.release()
           self.updateState(states.FOLLOWING)
           
       def calcPID(self, current, target, k_p, t_1, ...)
           ...
           return input
           
       def updateHardware(self, input):
           ...
           # transfer value to hardware
           
       def pidLoop(self):
           self._updateLock.acquire()
           k_p = self._cachedConfig.get("k_p") #pid parameter
           t_1 = self._cachedConfig.get("t_1") #pid parameter
           t_2 = self._cachedConfig.get("t_2") #pid parameter
           ...
           self._updateLock.release()
           target = self._cachedConfig("target")
           current = self.get("current") #this is a sensory read value
           input = self.calcPID(current, target, k_p, t_1, ...)
           self.updateHardware(input)
           
           
As you can see from the code the *delayed* configuration is updated upon reconfiguration.
Only upon command execution (slot updateTarget) is the new configuration made available
to the worker thread executing the PID loop.

Identifying Closed-Loop Target Parameters
=========================================

In Karabo identifying whether a property is on target can be done either by
state inspection or evaluation of the ``onTarget`` attribute. In the first case,
when a motor operated in a closed-loop fashion is instructed to drive to a new target
position by issuing in sequence the new target value and then a move command, it will 
transition from the ``states.ON``state to the ``states.MOVING``state and back to
``states.ON``. In this example normal operation is assumed, at any point a fault may of
course resulting into a transition into e.g. ``states.WARNING`` or ``states.ERROR``.

.. note::

	From the control framework perspective there is no conceptional difference for 
	open-loop behavior of the motor. The state transitions will be the same:
	from the ``states.ON``state to the ``states.MOVING``state and back to
    ``states.ON``.
	A middle-layer device evaluating these transitions does thus usually not have to
	care about the operation mode of the motor!
	
Check whether a movement has completed on-target, either for a closed- or an open-loop
operated motor can thus be implemented as:

.. code-block:: Python

   ...
   motorProxy.target = x
   motorProxy.move()
   waitUntil(lambda: motorProxy.state == states.ON)
   ...
   #continue in program flow
   

The state transition diagram for a device changing to a target position thus looks as
follows. Note that only the ``NORMAL`` meta-state as discussed in Section
:ref:`states`.

.. digraph:: state_transitions

    rankdir=LR;

    "ACTIVE"[shape = box style=filled, fillcolor=darkgreen]
    "PASSIVE"[shape = box style=filled, fillcolor=white]
    "CHANGING"[shape = box style=filled, fillcolor=cornflowerblue]

    "ACTIVE"->"PASSIVE"[label="deactivateEvent"]
    "PASSIVE"->"ACTIVE"[label="activateEvent"]
    "ACTIVE"->"CHANGING"[label="changeEvent"]
    "CHANGING"->"ACTIVE"[label="stopChangeEvent"]
    "CHANGING"->"CHANGING"[label="changeEvent"]


In the concrete example of a motor operating either in closed- or open-loop mode the
state machine looks as following.
   
.. digraph:: state_transitions_motor

    "ON"[shape = box style=filled, fillcolor=darkgreen]
    "OFF"[shape = box style=filled, fillcolor=white]
    "MOVING"[shape = box style=filled, fillcolor=cornflowerblue]
    "HOMING"[shape = box style=filled, fillcolor=cornflowerblue]

    "ON"->"OFF"[label="switchOffEvent"]
    "OFF"->"ON"[label="switchOnEvent"]
    "ON"->"MOVING"[label="moveEvent"]
    "MOVING"->"ON"[label="stopEvent"]
    "MOVING"->"MOVING"[label="moveEvent"]

    "ON"->"HOMING"[label="homeEvent"]
    "HOMING"->"ON"[label="stopEvent"]
    "HOMING"->"HOMING"[label="homeEvent"]




The Beckhoff protocol supports marking readback values as being *onTarget*. This is
necessary as in a closed loop system the target value may never be exactly reached or
only in transient situations. If the *onTarget* flag is set it is indicated by setting
an *onTarget* attribute to true for the respective property. The attribute is set
to undefined for properties not affected by this:

.. code-block:: Python

   def checkOnTarget(self):
       motorDevice = getDevice("myMotor")

       waitUntil(lambda: motorDevice.targetPosition.onTarget == closedLoop.onTarget)
       self.updateState(states.FOLLOWING)



The onTarget attribute may have the following values: ``closedLoop.onTarget``,
``closedLoop.offTarget``, ``closedLoop.undefined``.

For software closed-loop devices the attribute is created and initialized to ``undefined`` in
the expectedParameters section of the device:

.. code-block:: Python

   def SoftwareLoopDevice(Device)

       targetPosition = Float(displayedName = "Target Position", identifyOnTarget = true)



.. todo::
   
   Change all devices which currently implement a direct state change following a
   property assignment: SimpleMotor, Powersupplies.
