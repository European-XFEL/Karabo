#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 7, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import numbers
import numpy
import weakref

from PyQt4.QtCore import pyqtSignal, QTimer

from karabo.common.api import DeviceStatus
from karabo_gui import messagebox
from karabo_gui.events import KaraboEventSender, broadcast_event
from karabo_gui.schema import Box, ChoiceOfNodes, Schema
from karabo_gui.singletons.api import get_manager, get_network, get_topology


def _build_array_cmp(dtype):
    """Builds a comparison function for numpy arrays"""
    coerce = dtype.type
    if coerce is numpy.bool_:
        coerce = int

    def _array_cmp(a, b):
        try:
            return coerce(a) == coerce(b)
        except ValueError:
            return False

    return _array_cmp


def _cmp(a, b):
    """Normal comparison"""
    return a == b


def box_has_changes(descriptor, old_value, new_value):
    """Check whether the given `old_value` differs from the given `new_value`
    """
    if old_value is None:
        has_changes = True
    elif (isinstance(old_value, (numbers.Complex, numpy.inexact))
            and not isinstance(old_value, numbers.Integral)):
        diff = abs(old_value - new_value)
        absErr = descriptor.absoluteError
        relErr = descriptor.relativeError
        if absErr is not None:
            has_changes = diff >= absErr
        elif relErr is not None:
            has_changes = diff >= abs(old_value * relErr)
        else:
            has_changes = diff >= 1e-4
    elif isinstance(old_value, (list, numpy.ndarray)):
        if len(old_value) != len(new_value):
            has_changes = True
        else:
            has_changes = False
            cmp = _cmp
            if isinstance(old_value, numpy.ndarray):
                cmp = _build_array_cmp(old_value.dtype)
            for i in range(len(old_value)):
                if not cmp(old_value[i], new_value[i]):
                    has_changes = True
                    break
    else:
        has_changes = (str(old_value) != str(new_value))
    return has_changes


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
        """Create a new Configuration for schema.

        `type` should be 'class', 'projectClass' or 'device'.
        """

        super().__init__((), descriptor, None)
        assert type in ('class', 'projectClass', 'device')
        self.type = type
        self.id = id
        self._status = DeviceStatus.OFFLINE
        self.error = False
        self.parameterEditor = None
        self.bulk_changes = False
        self.bulk_list = {}
        self._topology_node_ref = None

        self._user_values = {}
        self._pending_keys = set()
        self.__busy_timer = QTimer(self)
        self.__busy_timer.setSingleShot(True)
        self.__busy_timer.timeout.connect(_on_timeout)

        self.serverId = None
        self.classId = None
        self.index = None

    def clearUserValue(self, box):
        """Clear a user-entered value for the child Box `box`
        """
        # Use `pop` with a default value so this can be called blindly
        self._user_values.pop(box.key(), None)

    def clearUserValues(self):
        """Clear all user-entered values for child Boxes
        """
        self._user_values = {}

    def getUserValue(self, box):
        """Returns the user-entered value for a Box
        """
        return self._user_values.get(box.key(), box.value)

    def hasAnyUserValues(self):
        """Returns true if a user has entered any values for this Configuration
        """
        return bool(self._user_values)

    def hasUserValue(self, box):
        """Returns true if a user has entered a value for this Box
        """
        return box.key() in self._user_values

    def sendAllUserValues(self):
        """Sends all user-entered values to the GUI server
        """
        changes = []
        for key, value in self._user_values.items():
            if key in self._pending_keys:
                continue
            path = key.split('.', 1)[-1].split('.')
            changes.append((self.getBox(path), value))
            self._pending_keys.add(key)

        if changes:
            get_network().onReconfigure(changes)
            self.__busy_timer.start(5000)

    def sendUserValue(self, box):
        """Sends a single user-entered value to the GUI server
        """
        key = box.key()
        value = self._user_values[key]
        get_network().onReconfigure([(box, value)])
        self._pending_keys.add(key)
        self.__busy_timer.start(5000)

    def setUserValue(self, box, value):
        """Store a user-entered value for the child Box `box`
        """
        self._user_values[box.key()] = value

    def setSchema(self, schema):
        if self.descriptor is not None:
            self.clearUserValues()
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
        """The remote device changed its value
        """
        # Clear pending status for the box
        key = box.key()
        if key in self._pending_keys:
            box.has_conflict = False
            self._pending_keys.remove(key)
            self._user_values.pop(key, None)
            if not self._pending_keys:
                self.__busy_timer.stop()

        if key in self._user_values:
            old_value = self._user_values[key]
            if box_has_changes(box.descriptor, old_value, value):
                box.has_conflict = True
            else:
                box.has_conflict = False
                self._user_values.pop(key, None)

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

    def shutdown(self):
        manager = get_manager()
        manager.shutdownDevice(self.id)


def _on_timeout():
    msg = "The property couldn't be set in the current state"
    messagebox.show_warning(msg)


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
