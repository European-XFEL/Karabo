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

from PyQt4.QtCore import QObject, pyqtSignal


class Configuration(Box):
    statusChanged = pyqtSignal(object, str)


    def __init__(self, key, type, descriptor=None):
        """
        Create a new Configuration for schema, type should be 'class',
        'projectClass' or 'device'.
        """

        super(Configuration, self).__init__((), descriptor, self)
        self.type = type
        self.key = key
        self.visible = 0
        self._status = "dead"


    def setSchema(self, schema):
        self.descriptor = Schema.parse(schema.name, schema.hash, {})
        if self.status != "alive":
            self.status = "schema"


    @property
    def status(self):
        """Each device can be in one of three states:

        "dead": nothing is known about the device
        "requested": a schema is requested, but didnt arrive yet
        "schema": the device has a schema, but no value yet
        "alive": everything is up-and-running """
        return self._status


    @status.setter
    def status(self, value):
        if value != self._status:
            self._status = value
            self.statusChanged.emit(self, value)


    def _set(self, value, timestamp):
        Box._set(self, value, timestamp)
        self.status = "alive"


    def getBox(self, path):
        box = self
        for p in path:
            box = getattr(box.value, p)
        return box


    def fillWidget(self, parameterEditor):
        self.parameterEditor = parameterEditor
        Box.fillWidget(self, parameterEditor,
                       self.type in ("class", "projectClass"))
        parameterEditor.globalAccessLevelChanged()


    def addVisible(self):
        self.visible += 1
        if self.visible == 1:
            manager.Manager().signalNewVisibleDevice.emit(self.key)


    def removeVisible(self):
        self.visible -= 1
        if self.visible == 0:
            manager.Manager().signalRemoveVisibleDevice.emit(self.key)


    def refresh(self):
        manager.Manager().onRefreshInstance(self)
