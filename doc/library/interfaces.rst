***********
Interfaces
***********

Motor Interface
===============

The motor interface fixes the basic commands and properties available for any device that is driving a physical motor.

CAVEAT: This interface definition is in its early stages and is likely to change slightly in future.


States
------

The following (device-)states are available:

* Safe
* Error
* Initializing
* Off
* Stopped
* Idle
* Moving
* Homing

**Safe**

This state should be used to reflect critical situations. Moving into
safe state using the command *safe*, is similiar to hitting some
emergency button. Depending on the specific instance the motor will
either be stopped or turned off and then brought to safe state. Broken
interlock conditions for example may lead to this behavior. The safe
state protects the device from macros or user inputs that may still
issues commands although the system should not be touched.

**Error**

Will be the state in case any error conditions or hardware faults are
encountered.

**Initializing**

The state during which the control software tries to connect to the
motor firmware.

**Off**

The connection to the motor is established, however the motor is
turned off, i.e. no currents are applied to the coils. The motor
should be manually (by hand) movable and unplugging it is safe.

**Stopped**

The motor does not move, but typically some current (see *reducedCurrent*) is applied, holding the motor in position. However, in this state the motor won't try to automatically correct for the target position.

**Idle**

This state can only be reached, if the motor runs in *followTarget* mode and currently is in the targetPosition (respecting some *deadband*). Any target position change will atomatically lead to a movement of the motor.

**Moving**

This is the state the motor is in while it is moving.

**Homing**

This states indicates that the motor is running an instance specific homing procedure. Once completed it will fall back to *Stopped* state and the *currentPosition* will typicaly 0'ed.   


Commands
--------

The following commands are available:

+------------+----------------------+-----------+
| Command    | Allowed States       | Access    |
+============+======================+===========+
| on         | Off                  | Operator  |
+------------+----------------------+-----------+
| off        | Stopped              | Operator  |
+------------+----------------------+-----------+
| stop       | Moving, Homing, Idle | Operator  |
+------------+----------------------+-----------+
| home       | Stopped, Idle        | Operator  |
+------------+----------------------+-----------+
| move       | Stopped, Idle        | Operator  |
+------------+----------------------+-----------+
| safe       | *Any*                | Operator  |
+------------+----------------------+-----------+
| normal     | Safe                 | Expert    |
+------------+----------------------+-----------+


NOTE: The Beckhoff based motors currently also have a stepUp and a stepDown
command and in combination with the *stepLength* property allow for
relative movements. In future the idea is to equip the GUI with the functionality to
increase property values in steps and auto-apply, which if executed on
the targetPosition (see below) solves the problem generically.


The state diagram below summarizes states, commands and their transitions:

.. image:: images/motor_fsm.png

General Properties
------------------

**currentPosition**

NOTE: In the current beckhoffMotor this property is called *encoderPosition*

Always shows the current absolute position in the physical unit: millimeters or milli-radian.

* Type: double
* Access Mode: read-only	      

**targetPosition**

Sets an absolute target position in the physical unit: millimeters or
milli-radian. The behavior of the motor after setting this property
depends on the value of the boolean property *followTarget*. If false,
the target position will only be loaded to the controller, but the
motor won't move until the command *move* is triggered. 

If the motor is in *Idle* state, the motor will immediately move to the applied
target position. NOTE: This is an exception to the usual design in Karabo, in which property changes typically don't result in state changes!

* Type: double
* Access Mode: reconfigurable
* Allowed States: Stopped, Off, Idle, Moving

**followTarget**

A boolean flag that decides how the motor should react with respect to
the currently loaded target position.
 
If followTarget is set to true, the next move command will bring the
motor into a closed loop such that it always corrects it's position
(respecting a defined *deadband*) to match the target position's
value. The states will be *Moving* when the motor moves and *Idle*
once the motor is on target. 

If followTarget is set to false, the motor will not stay in a closed
loop once the targetPosition is reached, but will fall back to
*Stopped* state.

* Type: bool
* Access Mode: reconfigurable
* Allowed States: Stopped

**isHardLimitUpper**

Boolean value indicating whether the motor is in the upper limit switch.

* Type: bool
* Access Mode: read-only

**isHardLimitLower**

Boolean value indicating whether the motor is in the lower limit switch.

* Type: bool
* Access Mode: read-only

**isHardLimitHome**

Boolean value indicating whether the motor is in the home switch.

* Type: bool
* Access Mode: read-only

**isSoftLimitUpper**

Boolean value indicating whether the motor reached the soft upper limit (as defined by softLimitUpper).

* Type: bool
* Access Mode: read-only


**isSoftLimitLower**

Boolean value indicating whether the motor reached the soft lower limit (as defined by softLimitLower).

* Type: bool
* Access Mode: read-only

**deadband**

Defines the deviation from the targetPosition in physical units for which the controller still reports to be on target.

* Type: float
* Access Mode: reconfigurable (expert)
* Allowed State: Off, Stopped, Idle

**backlash**

If this attribute is defined to something different than 0, the motor
will always stop the motion coming from the same mechanical
direction. This means that it could be possible to ask the motor to go
a little bit after the desired position and then to return to the
desired position. The attribute value is the number of steps the motor
will pass the desired position if it arrives from the “wrong”
direction. This is a signed value. If the sign is positive, this means
that the authorized direction to stop the motion is the increasing
motor position direction. If the sign is negative, this means that the
authorized direction to stop the motion is the decreasing motor
position direction.

* Type: float
* Access Mode: reconfigurable (expert)

**resetCurrentPosition**

This property acts like a software homing. The value given by this property will be treated as the *currentPosition* for the location the motor is currently in.
The upper and lower soft limits are adapted accordingly.

* Type: double
* Access Mode: reconfigurable (expert)
* Allowed State: Off, Stopped, Idle

**isHomed**

Boolean flag indicating, whether the motor was homed before. The flag gets position either after using the homing procedure or after *resetCurrentPosition* was applied.

* Type: bool
* Access: read-only

Beckhoff Specific Properties
----------------------------

**stepLength**

The step length is used in conjuction with the *stepUp* and *stepDown* commands. It describes the relativ length in physical units (millimeter or milli-radian) that the motor is moved.

* Type: float
* Access Mode: reconfigurable
* Allowed States: Off, Stopped, Idle

**offset**

The offset to be applied in the motor position computation. By default
set to 0. It is typically memorized on the motor hardware. It is changed upon changing the value of *resetCurrentPosition*.

* Type: double
* Access Mode: reconfigurable (expert)
* Allowed States: Off, Stopped, Idle

**stepCounterPosition**

The step counter position describes the motor position calculated from counter steps (instead of encoder values).
NOTE: The property should only be used for debugging purposes.

* Type: float
* Access Mode: read-only (expert)

**gear**

NOTE: This property should in future be renamed to stepsPerUnit.

The gear defines how many (micro-)steps are finally done to move the motor by one physical unit (millimeter, milli-radian or degree). 64 micro-steps are done per step and something like ~200 steps (depends on motor) will result in a full rotation.

* Type: float
* Access: reconfigurable (expert)
* Allowed States: Off, Stopped 

**encodeStep**

This property describes the factor by which each encoder step should be scaled to correctly map to the physical unit. If no encoder is present the value will represent the length of a microstep (i.e. 1 / gear).

* Type: float
* Access: reconfigurable (expert) 

**epsilon**

The epsilon determines the difference in change (in physical units) until a new physical value is posted from the hardware, i.e. updates the value of *currentPosition*.

NOTE: In the current implementation, care must be taken with very small values of epsilon, as this results in sending very many messages.

* Type: float
* Access Mode: reconfigurable (expert)

**vMax**

Maximum velocitiy. In position mode, the motor will drive with this velocity. 

**aMax**

Maximum acceleration.

**iMax**

Maximum current.








 


