..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _device_interfaces:

*****************
Device Interfaces
*****************

Karabo devices can implement one or more of the following interfaces:

- ``Motor`` :ref:`motor_interface`
- ``MultiAxisMotor`` :ref:`multi_axis_motor_interface`
- ``Trigger``
- ``Camera``
- ``Processor``
- ``DeviceInstantiator``

the implemented interfaces must be declared in the ``interfaces`` parameter which is a ``VectorString``.
The devices implementing an interface must fulfill the requirements detailed below.

.. _motor_interface:

Motor Interface
===============

Device states
-------------

The read-only ``state`` parameter (present by default in all Karabo devices) must take one of the following
values in the circumstances described:

* ``ON`` - The motor is powered and idle.
* ``OFF`` - The motor is not powered.
* ``DISABLED`` - A motor is fully functioanl, but cannot be controlled by the Karabo device
  (e.g. is part of a coupled motion group, or controlled by a local i/f)
* ``MOVING`` - When a motor is performing a move operation.

On top of that, it can assume the states commonly used by Karabo Device

* ``UNKNOWN`` - The Karabo device has no connection to the hardware
* ``INIT`` - The Karabo device has not be initialized yet.
* ``ERROR`` - An error has been reported by the HW or anyway detected.
* ``INTERLOCKED`` - The motor cannot be operated due to interlock condition
* ``INTERLOCK_BROKEN`` - The interlock condition has been manually disabled.
* ``CHANGING`` - the motor device is in a generic transien state.

The device may also implement the following motor-specific states:

* ``STOPPED`` - the motor was stopped in an unexpected way (emegency stop).
* ``STOPPING`` - the motor is slowing down after a stop command.
* ``HOMING`` - the motor is performing a 'home' procedure.

Other state values should not be used (e.g. ``ROTATING``, ``MOVING_LEFT`` etc. )


Mandatory features:
-------------------

One entry of ``interfaces`` must be ``'Motor'``.

The device must have the following parameters and slots:

* Motor Position

  * ``actualPosition``: a read-only floating point number exposing the
    current actual position.
  * ``targetPosition``: a read-write floating point number expressing the
    target for the next move. Writing to ``targetPosition`` does not trigger a motion.

* Move action

  * ``move``: When this slot is called, the motor tries to move to the next target position.
    The motion is initiated (and the device transit to ``MOVING`` state) before the slot returns,
    or omitted [#omitted_move]_.
  * ``stop``: A slot to terminate a movement in process.

* Hardware limits

  * ``isCWLimit``: a boolean which indicates whether the motor reached the high HW limit switch.
  * ``isCCWLimit``: a boolean which indicates whether the motor reached the low HW limit switch.

* On Target

  * ``isOnTarget``: a read-only boolean which indicates whether the motor reacthed its target position.


Optional features:
------------------

Other optional features are defined here. These are not mandatory, but are implemnented in many
Karabo motors. They are useful to take advantage of other libraries' (such as the ``virtualMotorBase``)
features:

* Software limits

  * ``swLimitHigh``: a configurable parameter to set the high software limit.
  * ``swLimitLow``: a configurable parameter to set the low software limit.
  * ``isSWLimitHigh``: a boolean which indicates whether the motor reached the high SW limit switch.
  * ``isSWLimitLow``: a boolean which indicates whether the motor reached the low HW limit switch.

* Step motion

  * ``stepUp``: a slot that initiates a relative motion in the positive direction
    (the device behave similarly to the ``move`` slot).
  * ``stepDown``: a slot that initiates a relative motion in the negative direction
    (the device behave similarly to the ``move`` slot).
  * ``stepLength``: a number to configure the displacement of a relative motion.


* Relative motion

  * ``moveRelative``: a slot that initiates a relative motion of `stepSize` displacement
    (the device behave similarly to the ``move`` slot).
  * ``stepSize``: the (signed) value of the displacement for a ``moveRelative`` command.

* Power

  * ``on``: A slot that can be executed only in the ``OFF`` state, which applies power
    to the motor. The device state changes to ``ON``.
  * ``off``: A slot that can be executed only in the ``ON`` state, which applies power
    to the motor. The device state changes to ``OFF`` state.

* Velocity

  * ``actualVelocity``: a read-only floating point number exposing the current velocity, expressed in
    the same units of the position values per second.

  * ``targetVelocity``: a read-write floating point number expressing the desired velocity for the movements.

* Epsilon

  * ``epsilonActualPosition``: the minimum change to trigger an update of the `actualPositionValue`. [#event_driven]_
  * ``epsilonActualVelcocity``: the minimum change to trigger an update of the `actualVelocityValue`.

* Deadband

  * ``deadband``: a number specifying the motor deadband, if this is implemented. The value
    must be expressed in the same units of target and actual position.

* Backlash

  * ``backlash``: a number defining the size of the position backlash to compensate.
  * ``enableBacklashCompensation``: a boolean enabling/disabling the backlash compensation.

* Homing

  * ``home``: a slot to initiate a homing (sometimes called referencing) procedure. The device
    state changes to ``HOMING``

* Coupled Motion

  * ``coupling``: a 'node' containing all the coupling related parameters.
  * ``isConfigurableAsSlave``: a read-only boolean that tells wheter the motor can be part of
    a coupled motion.
  * ``isMaster``: a read-only boolean.
  * ``isSlave``: a read-only boolean.
  * ``couple``: a slot that enables the coupling .
  * ``decouple``: a slot that disables the coupling.
  * ``masterDevice``: the Karabo ID of the coupled group master (parent) device.
  * ``numerator``: the numerator of the coupling ratio expressed as a fraction.
  * ``denominator``: the denominator of the coupling ratio expressed as a fraction.

..
  We should consider rephrasing the master/slave pattern to e.g. parent/child or similar.
  However, this should be done in parallel on the PLC side...


.. _multi_axis_motor_interface:

Multi-Axis Motor Interface
==========================

One entry of ``interfaces`` must be ``'MultiAxisMotor'``.

The parameter ``axes`` ``VectorString`` must contain the names of the axis
(e.g.: ``['X', 'Y', 'Z', 'RX', 'RY', 'RZ']`` ).

The device must have one node per each axis, whith the name stated in the ``axes``
parameter value.

Each axis node must have the same parameters and slots listed in the
:ref:`motor_interface` interface. It must also have a `state` parameter.

A ``move`` slot must be present at the top-level (i.e. outside of nodes). It will cause
the motor to move to the coordinated specified by the ``targetPosition`` of each node.


..
  TODO

  Trigger
  =======

  Camera
  ======

  Processor
  =========

  DeviceInstantiator
  ==================


.. rubric:: Footnotes

.. [#omitted_move] If the motor does not move after a 'move' command, e.g. because it is
already on target or because the requested displacement falls whithin the deadband, the
transition to the MOVING state can be omitted. However it should never happen that the
'MOVING' state is reached after the slot returns.

.. [#event_driven] According to the 'event driven' nature of Karabo, parameter values are
expected to be updated only when their values change. The 'epsilon' defined what is a
relevant change for a floating point parameter.