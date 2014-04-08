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


class Configuration(object):


    def __init__(self, path, type):
        """Create a new Configuration for schema,
        type should be 'class' or 'device'."""
        super(Configuration, self).__init__()
        self.type = type
        self.path = path
        self.schema = None
        self.visible = 0
        self._configuration = Box((), None, self)


    def setSchema(self, schema):
        self.schema = Schema.parse(schema.name, schema.hash, {})
        self._configuration.descriptor = self.schema


    @property
    def configuration(self):
        return self._configuration.value


    def toHash(self):
        return self._configuration.toHash()


    def merge(self, config):
        self._configuration.fromHash(config)


    def set(self, parameterKey, value):
        self._configuration.set(parameterKey, value)


    def setDefault(self):
        self._configuration.setDefault()


    def setAttribute(self, parameterKey, attributeKey, value):
        if not self.configuration.has(parameterKey):
            self.configuration.set(parameterKey, None)
        self.configuration.setAttribute(parameterKey, attributeKey, value)


    def getBox(self, path):
        box = self._configuration
        for p in path:
            box = getattr(box.value, p)
        return box


    def fillWidget(self, parameterEditor):
        self.parameterEditor = parameterEditor
        self.schema.fillWidget(parameterEditor, self._configuration,
                               self.type == "class")
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
