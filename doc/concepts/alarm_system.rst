

***************
Message Logging
***************


Karabo features a distributed message logging system classifying messages into four
categories: info, warning, error and debug. The message categories should be used as
there names indicate::

   self.log.INFO("Received new configuration for device")
   self.log.WARN("Failed to load parameter file, switching to defaults")
   self.log.ERROR("Hardware error on valve: no communication!")
   self.log.DEBUG("The analogue value is: {}".format(value))

It is important to understand the difference between warning and error
messages: a warning should be issued if some a state has been reached that is not
considered normal, but logic has been implemented to recover from it. It may also be
issued if there are known indications that the device is running into an error state.

Conversely, an error message should be issued if an unforeseen scenario has happened, i.e.
there is no logic to recover from this.


.. warning::

	Make yourself aware of the conceptual difference between warnings and errors and
	emit error notifications only in the case of actual errors.

	Experience from other facilities shows that *error-spamming* leads to users accepting
	errors and associated notifications as a normal operation state - they are not!
	Error notifications should be so rare that they trigger a human examination of the
	problem!

	The Karabo alarm system is separate from the distributed message log system. The message
	system may thus not be used to indicate actual alarms.

.. _alarm_system:

************
Alarm System
************

The Karabo alarm system is separate from the distributed logging system. It
works by raising alarm conditions which are displayed in the GUI, can be
queried from the CLI and are maintained by a centralized alarm service.

In Karabo there are four alarm states, signified by color, symbol and
level-number. They are summarized in the following table:

.. |alarm-interloc| image:: images/interlock_alarm.png

.. |alarm-critical| image:: images/critical_alarm.png

.. |alarm-warning| image:: images/warning.png


.. table:: Alarm and warning conditions in Karabo and their graphical representation
           for to-be-acknowledged alarms (left symbol) and non-acknowledgeable
           alarms (right symbol)

    ================ ================= =============================================================
    Alarm Conditions     Symbol           Comment
    ---------------- ----------------- -------------------------------------------------------------
    INTERLOCK        |alarm-interloc|  The property triggered an interlock
    CRITICAL         |alarm-critical|  The property is in its critical value range
    WARNING          |alarm-warning|   The property is in a value range where it should be monitored
    NONE             None              The property is in its normal value range
    ================ ================= =============================================================


Alarm conditions are usually indicated on a per-property basis and set either through
automatic evaluation of the bounds specified for a property or through explicit
calls.


Additionally, each device has a property ``alarmCondition``, defined in the
device base class. It is automatically set to the most significant property
alarm state on the device, where ``INTERLOCK`` is more significant than ``CRITICAL``
is more significant than ``WARNING`` is more significant than ``NONE``.

You may also set this state directly, by calling

.. code-block:: Python

    self.setAlarmCondition(alarmConditions.CRITICAL,
                           acknowledge = True,
                           description = "Why I set this condition")


This should be reserved for when an alarm cannot be attributed to having
resulted from device properties. In all cases you **must** provide a short
``description`` of why the alarm condition was triggered. Like property alarms,
interlock and critical conditions must be acknowledged. Acknowledgement of
warnings can be controlled using the ``acknowledge`` parameter, which defaults
to false.

.. warning::

    Alarms should always be indicated on the property which triggered the
    alarm if it is known.

    Alarm conditions are independent of device state. A device may be working
    perfectly, but its read-back values indicate that hardware is behaving
    abnormally. For example, a valve indicates a quick rise in pressure which is abnormal,
    the valve and valve device are working normally but the value indicates an
    alarm condition.


.. ifconfig:: includeDevInfo is True

    Any alarm condition triggered which is above none will make a device emit
    a signal ``signalAlarm`` to be interpreted by the alarm service device.

    .. function:: signalAlarm(senderInstance, senderClass, property, condition,
            type, acknowledge, description)

        where ``senderInstance` is the instance id of the device in the alarm
        condition, ``property`` is the property by which the alarm was raised or
        empty in case the alarm was set to the device directly, ``condition``
        is of type ``alarmCondition``, ``type`` indicates an alarmType as
        set by the property attributes, and ``acknowledge`` designates if the
        alarm needs to be acknowledged. Finally, ``description`` gives the
        optional description of the alarm.

    The alarm service device acts on this signal by registering the alarm in
    its list of managed alarms. If an alarm for the same property on the same
    instance already exists with a different severity exists, the alarm
    severity is adjusted to the new severity, while pending acknowledgments
    persist and are enabled if the new severity is lowered.

    Additionally, the service device registers itself to monitor
    the heart-beats of the device in alarm.

    The counterpart of ``signalAlarm`` is

    .. function:: signalAlarmEnd(senderInstance, senderClass, property, condition,
                    type,)

        which signals that the condition which triggered a given alarm has
        passed. Acknowledgeable alarms for this property are now cleared
        for acknowledgement.

    The alarm service additionally allows acknowledgment of any alarms pending,
    if the heartbeat signal from the device has not been received for more
    than 30 seconds. In this case it indicates that acknowledgment has been
    cleared by time-out and not by a cleared alarm condition.






Setting Alarm Ranges on Properties
==================================

Alarm ranges for numeric properties can be specified in the expected parameters
section of a device. These are automatically evaluated upon property updates
on the device and trigger the relevant property-specific alarms. The following
modifiers exist:

.. function:: .warnLow(value)

    Lower inclusive bound for a warning condition to be triggered.

.. function:: .warnHigh(value)

    Upper inclusive bound for a warning condition to be triggered.

.. function:: .alarmLow(value)

    Lower inclusive bound for a critical condition to be triggered.

.. function:: .alarmHigh(value)

    Upper inclusive bound for a critical condition to be triggered.

.. function:: warnVarianceLow(value)

    A variance above this value will trigger a warning condition. Must be
    preceded by a ``.enableRollingStats()`` command and closed of by
    ``evaluationInterval(n)``, where n is the rolling window size.

.. function:: .alarmVarianceHigh(value)

    A variance above this value will trigger a critical condition. Must be
    preceded by a ``.enableRollingStats()`` command and closed of by
    ``evaluationInterval(n)``, where n is the rolling window size.


Each of these attributes has to be followed by

.. function:: .needsAcknowledging(True | False)

    which specifies if this alarm has to be acknowledged to disappear.

and then optionally indicators of a short description.

.. function:: .description(message)

    gives a more detailed meaning of the alarm. Should be at maximum ca.
    80 characters.


.. note::

    It is good practice to give descriptions which include resolution strategies
    when known.

The alarm conditions may be configured in the device code (hard-coded) or
reconfigured for each device instance, including whether an alarm is to be
acknowledged or not. See the GUI Section :ref:`configuring_alarms` for details.


Acknowledging Alarms
++++++++++++++++++++

Alarms may be acknowledge in two ways: by using `The Alarm Service Device`_ as
described below.


The Alarm Service Device
========================

The alarm service device is the central collection point for all alarms in
a Karabo installation. It receives alarms from all other devices and allows
acknowledgement of these and filtering by alarm condition and type, as well
as instance id and device class. Mainly it is to be interacted with from the
GUI as described in :ref:`gui_alarm_service`.
