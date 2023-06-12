#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 21, 2012
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
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
