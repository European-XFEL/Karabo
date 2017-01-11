#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 7, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import pyqtSignal

from karabo_gui.events import (KaraboBroadcastEvent, KaraboEventSender,
                               broadcast_event)
from karabo_gui.schema import Schema, Box
from karabo_gui.singletons.api import get_manager, get_network


class BulkNotifications(object):
    """Make the configuration not notify every change to the listeners,
    but only after a set of changes was installed."""
    def __init__(self, conf):
        self.configuration = conf

    def __enter__(self):
        self.configuration.bulk_changes = True

    def __exit__(self, a, b, c):
        self.configuration.bulk_changes = False
        for box, (value, timestamp) in self.configuration.bulk_list.items():
            box.signalUpdateComponent.emit(box, value, timestamp)
        self.configuration.bulk_list = {}


class Configuration(Box):
    signalStatusChanged = pyqtSignal(object, str, bool)
    signalBoxChanged = pyqtSignal()

    def __init__(self, id, type, descriptor=None):
        """
        Create a new Configuration for schema, type should be 'class',
        'projectClass' or 'device'.
        """

        super().__init__((), descriptor, None)
        assert type in ('class', 'projectClass', 'device', 'deviceGroupClass', 'deviceGroup')
        self.type = type
        self.id = id
        self._status = "offline"
        self.error = False
        self.parameterEditor = None
        self.bulk_changes = False
        self.bulk_list = { }

        #if type == "device":
        self.serverId = None
        self.classId = None
        self.index = None

    def setSchema(self, schema):
        if self.descriptor is not None:
            self.redummy()
        self.descriptor = Schema.parse(schema.name, schema.hash, {})
        if self.status == "requested":
            if self.visible > 0:
                _start_device_monitoring(self.id)
            self.status = "schema"

    @property
    def status(self):
        """Each device can be in one of the following states:

        "noserver": device server not available
        "noplugin": class plugin not available
        "incompatible": device running, but of different type
        "offline": device could, but is not started
        "online": the device is online but doesn't have a schema yet
        "requested": a schema is requested, but didnt arrive yet
        "schema": the device has a schema, but no value yet
        "alive": everything is up-and-running
        "monitoring": we are registered to monitor this device

        "noserver", "noplugin", and "incompatible" only make sense
        if we actually know the (future) server, so only for a device
        in a project. Actual devices are just "offline". """
        return self._status

    @status.setter
    def status(self, value):
        assert value in ('offline', 'noserver', 'noplugin', 'online',
                         'incompatible', 'requested', 'schema', 'alive',
                         'monitoring')
        if value != self._status:
            self._status = value
        self.signalStatusChanged.emit(self, value, self.error)

    def isOnline(self):
        return self.status not in ("offline", "noplugin", "noserver",
                                   "incompatible")

    def updateStatus(self):
        """ determine the status from the system topology """
        manager = get_manager()
        if manager.systemHash is None:
            self.status = "offline"
            return

        for k in ("device", "macro", "server"):
            try:
                attrs = manager.systemHash[k][self.id, ...]
            except KeyError:
                continue
            if len(attrs) < 1:
                continue
            break
        else:
            self.error = False
            self.status = "offline"
            return

        self.classId = attrs.get("classId")
        self.serverId = attrs.get("serverId")
        error = attrs.get("status") == "error"
        self.error = error
        if self.status == "offline" and self.visible > 0:
            get_network().onGetDeviceSchema(self.id)
            self.status = "requested"
        elif self.status not in ("requested", "schema", "alive", "monitoring"):
            self.status = "online"
        else:
            self.signalStatusChanged.emit(self, self.status, self.error)

    def getBox(self, path):
        box = self
        for p in path:
            box = getattr(box.boxvalue, p)
        return box

    def hasBox(self, path):
        for p in path:
            if hasattr(self.boxvalue, p):
                return True
        return False

    def boxChanged(self, box, value, timestamp):
        if self.bulk_changes:
            self.bulk_list[box] = value, timestamp
        else:
            box.signalUpdateComponent.emit(box, value, timestamp)

    def fillWidget(self, parameterEditor):
        self.parameterEditor = parameterEditor
        Box.fillWidget(self, parameterEditor,
                       self.type in ("class", "projectClass", "deviceGroupClass"))
        parameterEditor.globalAccessLevelChanged()
        parameterEditor.ensureMiddleColumnWidth()

    def addVisible(self):
        self.visible += 1
        if self.visible == 1 and self.status not in ("offline", "requested"):
            network = get_network()
            if self.status == "online":
                network.onGetDeviceSchema(self.id)
                self.status = "requested"
            else:
                _start_device_monitoring(self.id)

    __enter__ = addVisible

    def removeVisible(self):
        self.visible -= 1
        if self.visible == 0 and self.status not in ("offline", "requested"):
            _stop_device_monitoring(self.id)
            if self.status == "monitoring":
                self.status = "alive"

    def __exit__(self, a, b, c):
        self.removeVisible()

    def refresh(self):
        network = get_network()
        network.onGetDeviceConfiguration(self)

    def shutdown(self):
        manager = get_manager()
        manager.shutdownDevice(self.id)


def _start_device_monitoring(device_id):
    """Initiate device monitoring
    """
    get_network().onStartMonitoringDevice(device_id)
    broadcast_event(KaraboBroadcastEvent(
        KaraboEventSender.StartMonitoringDevice,
        data={'device_id': device_id})
    )


def _stop_device_monitoring(device_id):
    """Cease device monitoring
    """
    get_network().onStopMonitoringDevice(device_id)
    broadcast_event(KaraboBroadcastEvent(
        KaraboEventSender.StopMonitoringDevice,
        data={'device_id': device_id})
    )
