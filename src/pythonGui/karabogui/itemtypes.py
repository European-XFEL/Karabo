#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


class NavigationItemTypes(object):
    UNDEFINED = -1
    HOST = 0
    SERVER = 1
    CLASS = 2
    DEVICE = 3


class ProjectItemTypes(object):
    UNDEFINED = "Undefined"
    PROJECT = "Project"
    PROJECT_GROUP = "ProjectGroup"
    MACRO = "Macro"
    MACRO_INSTANCE = "MacroInstance"
    SCENE = "Scene"
    SERVER = "Server"
    DEVICE = "Device"
    CONFIGURATION = "Configuration"
