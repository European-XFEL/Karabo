..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _alarm_system:

************
Alarm System
************

The Karabo alarm system is separate from the distributed logging system. It
works by raising alarm conditions which are displayed in the GUI, can be
queried from the CLI and are maintained by a centralized alarm service.

In Karabo there are four alarm states, signified by color, symbol and
level-number. They are summarized in the following table:

.. |alarm-interlock| image:: images/interlock_alarm.png

.. |alarm-critical| image:: images/critical_alarm.png

.. |alarm-warning| image:: images/warning.png


.. table:: Alarm and warning conditions in Karabo and their graphical representation
           for to-be-acknowledged alarms (left symbol) and non-acknowledgeable
           alarms (right symbol)

    ================ ================= =============================================================
    Alarm Conditions     Symbol           Comment
    ---------------- ----------------- -------------------------------------------------------------
    INTERLOCK        |alarm-interlock| The property triggered an interlock
    CRITICAL         |alarm-critical|  The property is in its critical value range
    WARNING          |alarm-warning|   The property is in a value range where it should be monitored
    NONE             None              The property is in its normal value range
    ================ ================= =============================================================


Alarm conditions are usually indicated on a per-property basis and set either through
automatic evaluation of the bounds specified for a property or through explicit
calls.

Additionally, each device has a property ``globalAlarmCondition``, defined in the
device base class. It is automatically set to the most significant property
alarm state on the device, where ``INTERLOCK`` is more significant than ``CRITICAL``
is more significant than ``WARNING`` is more significant than ``NONE``.

This should be reserved for when an alarm cannot be attributed to having
resulted from device properties. In all cases you *should* provide a short
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
