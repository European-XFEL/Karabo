#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 7, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import weakref

from PyQt4.QtCore import pyqtSignal

from karabo.common.api import DeviceStatus
from karabo_gui.events import KaraboEventSender, broadcast_event
from karabo_gui.schema import Schema, Box
from karabo_gui.singletons.api import get_manager, get_network, get_topology


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
    signalStatusChanged = pyqtSignal(object, object, bool)
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
        self._status = DeviceStatus.OFFLINE
        self.error = False
        self.parameterEditor = None
        self.bulk_changes = False
        self.bulk_list = {}
        self._topology_node_ref = None

        self.serverId = None
        self.classId = None
        self.index = None

    def setSchema(self, schema):
        if self.descriptor is not None:
            self.redummy()
        self.descriptor = Schema.parse(schema.name, schema.hash, {})
        if self.status is DeviceStatus.REQUESTED:
            if self.visible > 0:
                _start_device_monitoring(self.id)
            self.status = DeviceStatus.SCHEMA

    @property
    def topology_node(self):
        if self._topology_node_ref:
            return self._topology_node_ref()
        return self._topology_node_ref

    @topology_node.setter
    def topology_node(self, topo_node):
        self._topology_node_ref = weakref.ref(topo_node)

    @property
    def status(self):
        return self._status

    @status.setter
    def status(self, value):
        assert isinstance(value, DeviceStatus)
        if value != self._status:
            self._status = value
        self.signalStatusChanged.emit(self, value, self.error)

    def isOnline(self):
        offline_statuses = (DeviceStatus.OFFLINE, DeviceStatus.NOPLUGIN,
                            DeviceStatus.NOSERVER, DeviceStatus.INCOMPATIBLE)
        return self.status not in offline_statuses

    def updateStatus(self):
        """ determine the status from the system topology """
        topology = get_topology()
        if not topology.online:
            self.status = DeviceStatus.OFFLINE
            return

        for k in ("device", "macro", "server"):
            path = '{}.{}'.format(k, self.id)
            attrs = topology.get_attributes(path)
            if not attrs:
                continue
            break
        else:
            self.error = False
            self.status = DeviceStatus.OFFLINE
            return

        self.classId = attrs.get("classId")
        self.serverId = attrs.get("serverId")
        error = attrs.get("status") == "error"
        self.error = error

        special_statuses = (DeviceStatus.REQUESTED, DeviceStatus.SCHEMA,
                            DeviceStatus.ALIVE, DeviceStatus.MONITORING)
        if self.status is DeviceStatus.OFFLINE and self.visible > 0:
            get_network().onGetDeviceSchema(self.id)
            self.status = DeviceStatus.REQUESTED
        elif self.status not in special_statuses:
            self.status = DeviceStatus.ONLINE
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

    def addVisible(self):
        ignored_statuses = (DeviceStatus.OFFLINE, DeviceStatus.REQUESTED)
        self.visible += 1
        if self.visible == 1 and self.status not in ignored_statuses:
            network = get_network()
            if self.status is DeviceStatus.ONLINE:
                network.onGetDeviceSchema(self.id)
                self.status = DeviceStatus.REQUESTED
            else:
                _start_device_monitoring(self.id)

    __enter__ = addVisible

    def removeVisible(self):
        ignored_statuses = (DeviceStatus.OFFLINE, DeviceStatus.REQUESTED)
        self.visible -= 1
        if self.visible == 0 and self.status not in ignored_statuses:
            _stop_device_monitoring(self.id)
            if self.status is DeviceStatus.MONITORING:
                self.status = DeviceStatus.ALIVE

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
    broadcast_event(KaraboEventSender.StartMonitoringDevice,
                    {'device_id': device_id})


def _stop_device_monitoring(device_id):
    """Cease device monitoring
    """
    get_network().onStopMonitoringDevice(device_id)
    broadcast_event(KaraboEventSender.StopMonitoringDevice,
                    {'device_id': device_id})
