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


class Configuration(Box):
    statusChanged = pyqtSignal(object, str, bool)


    def __init__(self, id, type, descriptor=None):
        """
        Create a new Configuration for schema, type should be 'class',
        'projectClass' or 'device'.
        """

        super(Configuration, self).__init__((), descriptor, self)
        assert type in ('class', 'projectClass', 'device', 'deviceGroup')
        self.type = type
        self.id = id
        self.visible = 0
        self._status = "offline"
        self.error = False
        self.parameterEditor = None

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

        "noserver", "noplugin", and "incompatible" only make sense
        if we actually know the (future) server, so only for a device
        in a project. Actual devices are just "offline". """
        return self._status


    @status.setter
    def status(self, value):
        assert value in ('offline', 'noserver', 'noplugin', 'online',
                         'incompatible', 'requested', 'schema', 'alive')
        if value != self._status:
            self._status = value
        self.statusChanged.emit(self, value, self.error)


    def updateStatus(self):
        """ determine the status from the system topology """
        if manager.Manager().systemHash is None:
            self.status = "offline"
            return

        try:
            attrs = manager.Manager().systemHash["device"][self.id, ...]
        except KeyError as e:
            self.error = False
            self.status = "offline"
        else:
            self.classId = attrs.get("classId")
            self.serverId = attrs.get("serverId")
            error = attrs.get("status") == "error"
            self.error = error
            if self.status == "offline" and self.visible > 0:
                Network().onGetDeviceSchema(self.id)
                self.status = "requested"
            elif self.status not in ("requested", "schema", "alive"):
                self.status = "online"
            else:
                self.statusChanged.emit(self, self.status, self.error)


    def getBox(self, path):
        box = self
        for p in path:
            box = getattr(box.boxvalue, p)
        return box


    def fillWidget(self, parameterEditor):
        self.parameterEditor = parameterEditor
        Box.fillWidget(self, parameterEditor,
                       self.type in ("class", "projectClass"))
        parameterEditor.globalAccessLevelChanged()


    def addVisible(self):
        self.visible += 1
        if self.visible == 1 and self.status not in ("offline", "requested"):
            Network().onStartMonitoringDevice(self.id)


    def removeVisible(self):
        self.visible -= 1
        if self.visible == 0 and self.status != "offline":
            Network().onStopMonitoringDevice(self.id)


    def refresh(self):
        Network().onGetDeviceConfiguration(self)
