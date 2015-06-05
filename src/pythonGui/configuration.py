#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 7, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents configurations for classes and
devices.
"""

__all__ = ["Configuration"]


from schema import Schema, Box
import manager
from network import Network

from PyQt4.QtCore import pyqtSignal


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
        self.configuration.bulk_list = { }


class Configuration(Box):
    signalStatusChanged = pyqtSignal(object, str, bool)
    signalInitReply = pyqtSignal(bool, str)


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
                Network().onStartMonitoringDevice(self.id)
                index = manager.Manager().systemTopology.findIndex(self.id)
                if index is not None and index.isValid():
                    assert not index.internalPointer().monitoring
                    index.internalPointer().monitoring = True
                    manager.Manager().systemTopology.dataChanged.emit(
                        index, index)
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


    def checkClassSchema(self):
        pass


    def updateStatus(self):
        """ determine the status from the system topology """
        if manager.Manager().systemHash is None:
            self.status = "offline"
            return
        
        for k in ("device", "macro", "server"):
            try:
                attrs = manager.Manager().systemHash[k][self.id, ...]
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
            Network().onGetDeviceSchema(self.id)
            self.status = "requested"
        elif self.status not in ("requested", "schema", "alive", "monitoring"):
            self.status = "online"
        else:
            self.signalStatusChanged.emit(self, self.status, self.error)

    def onClassDescriptor(self, box):
        """this is connected to the (supposed) class of an offline device.
        Set the class descriptor only if we don't already have one."""
        if self.descriptor is None:
            self.descriptor = box.descriptor
        if self.descriptor is not None:
            box.signalNewDescriptor.disconnect(self.onClassDescriptor)

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


    def addVisible(self):
        self.visible += 1
        if self.visible == 1 and self.status not in ("offline", "requested"):
            if self.status == "online":
                Network().onGetDeviceSchema(self.id)
                self.status = "requested"
            else:
                Network().onStartMonitoringDevice(self.id)
                idx = manager.Manager().systemTopology.findIndex(self.id)
                if idx is not None and idx.isValid():
                    assert not idx.internalPointer().monitoring
                    idx.internalPointer().monitoring = True
                    manager.Manager().systemTopology.dataChanged.emit(idx, idx)


    __enter__ = addVisible

    def removeVisible(self):
        self.visible -= 1
        if self.visible == 0 and self.status not in ("offline", "requested"):
            Network().onStopMonitoringDevice(self.id)
            index = manager.Manager().systemTopology.findIndex(self.id)
            if index is not None and index.isValid():
                assert index.internalPointer().monitoring
                index.internalPointer().monitoring = False
                manager.Manager().systemTopology.dataChanged.emit(index, index)
            if self.status == "monitoring":
                self.status = "alive"


    def __exit__(self, a, b, c):
        self.removeVisible()


    def refresh(self):
        Network().onGetDeviceConfiguration(self)


    def shutdown(self):
        manager.Manager().shutdownDevice(self.id)
