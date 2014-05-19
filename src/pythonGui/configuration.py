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


class Configuration(QObject):
    signalConfigurationNewDescriptor = pyqtSignal(object) # configuration

    def __init__(self, path, type, descriptor=None):
        """
        Create a new Configuration for schema, type should be 'class',
        'projectClass' or 'device'.
        """
        
        super(Configuration, self).__init__()
        self.type = type
        self.path = path
        self.visible = 0
        
        self._box = Box((), descriptor, self)


    def getDescriptor(self):
        return self._box.descriptor


    def setDescriptor(self, descriptor):
        self._box.descriptor = descriptor


    def setSchema(self, schema):
        self._box.descriptor = Schema.parse(schema.name, schema.hash, {})
        print "emit.setSchema", self.path
        self.signalConfigurationNewDescriptor.emit(self)


    @property
    def configuration(self):
        return self._box.value


    def toHash(self):
        return self._box.toHash()


    def merge(self, config):
        self._box.fromHash(config)


    def set(self, parameterKey, value):
        self._box.set(parameterKey, value)


    def setDefault(self):
        """
        This function should be called explicitly whenever a new schema was set
        and the default values are required to be updated.
        """
        self._box.setDefault()


    def setAttribute(self, parameterKey, attributeKey, value):
        if not self.configuration.has(parameterKey):
            self.configuration.set(parameterKey, None)
        self.configuration.setAttribute(parameterKey, attributeKey, value)


    def getBox(self, path):
        box = self._box
        for p in path:
            box = getattr(box.value, p)
        return box


    def fillWidget(self, parameterEditor):
        self.parameterEditor = parameterEditor
        self._box.fillWidget(parameterEditor, (self.type == "class") \
                                           or (self.type == "projectClass"))
        parameterEditor.globalAccessLevelChanged()


    def addVisible(self):
        self.visible += 1
        if self.visible == 1:
            manager.Manager().signalNewVisibleDevice.emit(self.path)


    def removeVisible(self):
        self.visible -= 1
        if self.visible == 0:
            manager.Manager().signalRemoveVisibleDevice.emit(self.path)


    def refresh(self):
        manager.Manager().onRefreshInstance(self)
