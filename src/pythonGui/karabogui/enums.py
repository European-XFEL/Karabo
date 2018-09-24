#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from enum import Enum


class NavigationItemTypes(object):
    UNDEFINED = -1
    HOST = 0
    SERVER = 1
    CLASS = 2
    DEVICE = 3


class KaraboSettings(Enum):
    CONFIG_DIR = "LastUsedFolder/ConfigurationDialog"
    MACRO_DIR = "LastUsedFolder/MacroDialog"
    SCENE_DIR = "LastUsedFolder/SceneDialog"
    PROJECT_DOMAIN = "Project/LastUsedDbDomain"
    MACRO_SERVER = "Server/LastUsedMacroServer"
    USERNAME = "Login/LastUsername"
    GUI_SERVERS = "Login/LastConnectionAddresses"
    MAX_GUI_SERVERS = "Login/MaxConnectionAddresses"
    BROKER_TOPIC = "Login/Topic"
