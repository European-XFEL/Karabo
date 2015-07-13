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

Commands
--------

The following commands are available:

+------------+----------------+-----------+
| Command    | Allowed States | Access    |
+============+================+===========+
| on         | Off            | Operator  |
+------------+----------------+-----------+
| off        | On             | Operator  |
+------------+----------------+-----------+
| stop       | Moving, Homing | Operator  |
+------------+----------------+-----------+
| home       | Stopped, Idle  | Operator  |
+------------+----------------+-----------+
| move       | Stopped, Idle  | Operator  |
+------------+----------------+-----------+
| stepUp     | Stopped, Idle  | Operator  |
+------------+----------------+-----------+
| stepDown   | Stopped, Idle  | Operator  |
+------------+----------------+-----------+
| safe       | Any            | Operator  |
+------------+----------------+-----------+
| normal     | safe           | Expert    |
+------------+----------------+-----------+

The state diagram below summarizes states, commands and their transitions:

.. image:: images/motor_fsm.png

Properties
----------

**Current Position**

Always shows the current absolute position in the physical unit: millimeters or milli-radian.

* Key: currentPosition
* Type: double
* Access Mode: read-only	      

**Target Position**

Sets an absolute target position in the physical unit: millimeters or
milli-radian. The behavior of the motor after setting this property
depends on the value of the boolean property *followTarget*. If false,
the target position will only be loaded to the controller, but the
motor won't move until the command *move* is triggered. 

If *followTarget* is true, the motor will immediately move to the applied
target position. **NOTE**: This is an exception to the usual design in Karabo, in which property changes typically don't result in state changes!

* Key: targetPosition
* Type: double
* Access Mode: reconfigurable
* Allowed States: Stopped, Off, Idle, Moving

**Follow Target**

A boolean flag that decides how the motor should react with respect to the currently loaded target position. If true, the motor will always correct it's position (respecting a defined *dead band*) to match the target position's value. The states will be *Moving* during correction and *Idle* once the motor is on target.

If followTarget is set to false, the motor will only move after an explicitly triggering the *move* command. The state will fall back to *Stopped* after a movement was triggered and the motor is on target.

* Key: followTarget
* Type: bool
* Access Mode: reconfigurable
* Allowed States: Off

**Step Length**

The step length is used in conjuction with the *stepUp* and *stepDown* commands. It describes the relativ length in physical units (millimeter or milli-radian) that the motor is moved.

* Key: stepLength
* Type: float
* Access Mode: reconfigurable
* Allowed States: Off, Stopped, Idle

**Offset**

The offset to be applied in the motor position computation. By default
set to 0. It is typically memorized on the motor hardware.

* Key: offset
* Type: double
* Access Mode: reconfigurable (expert)
* Allowed States: Off, Stopped, Idle

**Dial Position**

The following formula links together the currentPosition, dialPosition, sign and offset properties:

    currentPosition = sign * dialPosition + offset

This allows to have the motor position centered around any position
defined by the offset property (classically the X ray beam
position). It is a read only property. To set the motor position, the
user has to use the targetPosition attribute. The unit used
for this attribute is the physical unit: millimeters or
milli-radian. It is also always an absolute position.

* Key: dialPosition
* Type: double
* Access Mode: read-only

**isHardLimitUpper**

Boolean value indicating whether the motor is in the upper limit switch.

* Key: isHardLimitUpper
* Type: bool
* Access Mode: read-only

**isHardLimitLower**

Boolean value indicating whether the motor is in the lower limit switch.

* Key: isHardLimitLower
* Type: bool
* Access Mode: read-only

**isHardLimitHome**

Boolean value indicating whether the motor is in the home switch.

* Key: isHardLimitHome
* Type: bool
* Access Mode: read-only

**isSoftLimitUpper**

Boolean value indicating whether the motor reached the soft upper limit (as defined by softLimitUpper).

* Key: isSoftLimitUpper
* Type: bool
* Access Mode: read-only


**isSoftLimitLower**

Boolean value indicating whether the motor reached the soft lower limit (as defined by softLimitLower).

* Key: isSoftLimitLower
* Type: bool
* Access Mode: read-only

**Deadband**

Defines the deviation from the targetPosition in physical units for which the controller still reports to be on target.

* Key: deadband
* Type: float
* Access Mode: reconfigurable (expert)
* Allowed State: Off, Stopped, Idle

**Epsilon**

The epsilon determines the difference in change (in physical units) until a new physical value is posted from the hardware, i.e. updates the value of *currentPosition*.

* Key: epsilon
* Type: float
* Access Mode: reconfigurable (expert)

**Push Interval**

The push interval determines a regular interval (in ms) in which the current position is updated. A value of 0 disables any update.

* Key: pushInterval
* Type: int32
* Access Mode: reconfigurable (expert)

**Backlash**

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

* Key: backlash
* Type: float
* Access Mode: reconfigurable (expert)

**Acceleration**


**Deceleration**


**Velocity**









 


