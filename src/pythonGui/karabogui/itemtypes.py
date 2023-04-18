#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 21, 2012
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################


class NavigationItemTypes:
    UNDEFINED = -1
    HOST = 0
    SERVER = 1
    CLASS = 2
    DEVICE = 3


class ConfiguratorItemType:
    LEAF = "Leaf"
    NODE = "Node"


class ProjectItemTypes:
    UNDEFINED = "Undefined"
    PROJECT = "Project"
    PROJECT_GROUP = "ProjectGroup"
    MACRO = "Macro"
    MACRO_INSTANCE = "MacroInstance"
    SCENE = "Scene"
    SERVER = "Server"
    DEVICE = "Device"
    CONFIGURATION = "Configuration"
