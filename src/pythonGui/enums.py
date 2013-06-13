#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains classes which define different enumerators."""

__all__ = ["NavigationItemTypes", "AccessMode", "ConfigChangeTypes"]


class NavigationItemTypes(object):
    UNDEFINED = -1
    HOST = 0
    SERVER = 1
    CLASS = 2
    DEVICE = 3


class AccessMode(object): # TODO use INTs?
    INIT = "1"
    READONLY = "2"
    RECONFIG = "4"


class ConfigChangeTypes(object):
    DEVICE_CLASS_CONFIG_CHANGED = 0 # new configuration for DEVICE_CLASS
    DEVICE_INSTANCE_CONFIG_CHANGED = 1 # new configuration for DEVICE_INSTANCE
    DEVICE_INSTANCE_CURRENT_VALUES_CHANGED = 2 # current values for DEVICE_INSTANCE changed

