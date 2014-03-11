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


from karabo.karathon import Hash, HashMergePolicy


class Configuration(object):


    def __init__(self, schema):
        super(Configuration, self).__init__()

        # The schema which belongs to this configuration
        self.schema = schema
        # The configuration
        self.configuration = Hash()


    def merge(self, config):
        self.configuration.merge(config, HashMergePolicy.MERGE_ATTRIBUTES)


    def set(self, parameterKey, value):
        self.configuration.set(parameterKey, value)


    def setAttribute(self, parameterKey, attributeKey, value):
        if not self.configuration.has(parameterKey):
            self.configuration.set(parameterKey, None)
        self.configuration.setAttribute(parameterKey, attributeKey, value)

