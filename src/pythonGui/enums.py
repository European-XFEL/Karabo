#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains classes which define different enumerators."""

__all__ = ["NavigationItemTypes", "AccessTypes", "ConfigChangeTypes"]


class NavigationItemTypes(object):
    UNDEFINED = -1
    NODE = 0
    DEVICE_SERVER_INSTANCE = 1
    DEVICE_CLASS = 2
    DEVICE_INSTANCE = 3


class AccessTypes(object): # TODO use INTs?
    INIT = "1"
    READONLY = "2"
    RECONFIG = "4"


class ConfigChangeTypes(object):
    DEVICE_CLASS_CONFIG_CHANGED = 0 # new configuration for DEVICE_CLASS
    DEVICE_INSTRANCE_CONFIG_CHANGED = 1 # new configuration for DEVICE_INSTANCE
    DEVICE_INSTANCE_CURRENT_VALUES_CHANGED = 2 # current values for DEVICE_INSTANCE changed

