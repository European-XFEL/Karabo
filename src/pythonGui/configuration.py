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


class Configuration(object):


    def __init__(self, schema, path, type):
        """Create a new Configuration for schema,
        type should be 'class' or 'device'."""
        super(Configuration, self).__init__()

        r = SchemaReader()
        self.schema = r.readSchema(schema)
        self.configuration = self.schema.getClass()('', self)
        self.type = type
        self.path = path


    def merge(self, config):
        self.schema.__set__(self.configuration, config)


    def set(self, parameterKey, value):
        self.configuration.set(parameterKey, value)


    def setAttribute(self, parameterKey, attributeKey, value):
        if not self.configuration.has(parameterKey):
            self.configuration.set(parameterKey, None)
        self.configuration.setAttribute(parameterKey, attributeKey, value)


    def fillWidget(self, parameterEditor):
        self.parameterEditor = parameterEditor
        self.schema.fillWidget(parameterEditor, self.configuration,
                               self.type == "class")
