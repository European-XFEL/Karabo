..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

*************
Alarm Service
*************

The AlarmService Device
=======================

The ``AlarmService`` device keeps track of the alarming status of all the devices
known to the distributed system (currently, this is a synonym for "every device in
the same karabo topic of the ``AlarmService`` instance").
The ``AlarmService`` is considered to be a singleton, meaning that there is only
one instance of this class in the same topic installation.

The ``AlarmService`` performs a set of actions upon initialization in order to
fullfil its responsibility. At its ``initialize`` method, it self registers as a
monitor for ``InstanceNew`` and ``InstanceGone`` events.

In response to ``InstanceNew`` events, the ``AlarmService`` connects its
``slotUpdateAlarm`` to the new device's ``signalAlarmUpdate``.  A device is
registered with the ``AlarmService`` after a successful connection to its
``signalAlarmUpdate``. It is possible for a "new" device to be already registered
with the ``AlarmService`` - this happens, for instance, if the device crashed and
then got reinstantiated before an ``InstanceGone`` could be generated due to a
heartbeat timeout interval. For such "new" devices, the ``AlarmService`` requests
an alarming status from the device by callings its ``slotReSubmitAlarms``
asynchronously. The reply from ``slotReSubmitAlarms`` is handled by
``slotUpdateAlarm``.

``slotUpdateAlarm`` maintains a ``Hash``, ``m_updateHash``, with the
updates for alarming status of the devices. The ``AlarmService`` periodically
emits ``signalAlarmServiceUpdate`` if its ``m_updateHash`` is not empty.

In response to ``InstanceGone`` events, the ``AlarmService`` updates all pending
alarms related to the device that is gone to be acknowledgeable.

``AlarmService`` periodically persists the alarming status for devices that it
keeps internally to a file - ``$KARABO/var/data/Karabo_AlarmService.xml``.
The alarming status of devices stored in this file are used by
``AlarmService`` at start time to set its initial alarming status information.
As part of its initialization, the ``AlarmService`` also sends this restored
alarming status data through a ``signalAlarmServiceUpdate``.

Interactions with the GUI Server Device
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``GuiServerDevice`` connects to the ``AlarmService::signalAlarmServiceUpdate`` at
its method ``connectPotentialAlarmService``.

``GuiServerDevice::connectPotentialAlarmService`` searches for the ``AlarmService`` in
all the topology currently known to the ``GuiServerDevice``. As there may be
no known ``AlarmService`` at the moment the ``GuiServerDevice`` is started,
the ``GuiServerDevice`` calls this method both at initialization time and as
part of the handling of any event of type ``InstanceNew`` event - just in
case the new device is an ``AlarmService`` instance.

A ``signalAlarmServiceUpdate`` is handled by ``GuiServer::slotAlarmSignalsUpdate``.
The connection between the signal and the slot is performed by
``GuiServerDevice::connectPotentialAlarmService`` when its ``topologyEntry`` parameter
is an ``AlarmService`` instance. Once a successful connection to the ``AlarmService``
signal is established, the ``GuiServerDevice::onRequestAlarms`` is called. This method
will send an async request to ``AlarmService::slotRequestAlarmDump``. The reply for
the async request is handled by ``GuiServerDevice::onRequestedAlarmsReply``.
The handling of the reply consists of sending a message of type ``alarmInit`` with the
updated alarms data to all the connected GUI clients. After this initial alarm snapshot,
any changes in alarming conditions of devices instantiated in the topic will be updated
by ``GuiServer::slotAlarmSignalsUpdate``.

    Further information about the Alarm Service can be found at **Karabo** > **Outdated Concepts** > **Alarm System**.
    The pieces of information in that page that are not outdated will be merged soon into this page.



