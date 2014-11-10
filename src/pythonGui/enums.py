#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains classes which define different enumerators."""

__all__ = ["NavigationItemTypes", "AccessMode"]

class NavigationItemTypes(object):
    UNDEFINED = -1
    HOST = 0
    SERVER = 1
    CLASS = 2
    DEVICE = 3


class CompositionMode(object):
    UNDEFINED = -1
    ONLINE = 0
    OFFLINE = 1

