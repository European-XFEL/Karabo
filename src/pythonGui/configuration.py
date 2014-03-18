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


from hash import Hash, HashMergePolicy
from schemareader import SchemaReader
import weakref
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


    def setSchema(self, schema):
        r = SchemaReader()
        self.schema = r.readSchema(schema)
        self._configuration = self.schema.Box("", self.schema, self)
        self._configuration.value = self.schema.getClass()("", self)


    @property
    def configuration(self):
        return self._configuration.value


    def merge(self, config):
        self._configuration.set(config)


    def set(self, parameterKey, value):
        self._configuration.set(parameterKey, value)


    def setAttribute(self, parameterKey, attributeKey, value):
        if not self.configuration.has(parameterKey):
            self.configuration.set(parameterKey, None)
        self.configuration.setAttribute(parameterKey, attributeKey, value)


    def fillWidget(self, parameterEditor):
        self.parameterEditor = parameterEditor
        self.schema.fillWidget(parameterEditor, self._configuration,
                               self.type == "class")


    def addVisible(self):
        self.visible += 1
        if self.visible == 1:
            manager.Manager().signalNewVisibleDevice.emit(self.path)


    def removeVisible(self):
        self.visible -= 1
        if self.visible == 0:
            manager.Manager().signalRemoveVisibleDevice.emit(self.path)
