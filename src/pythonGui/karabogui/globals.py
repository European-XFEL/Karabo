#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 23, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from os import environ, getpid, path
import socket
from sys import platform

from karabogui.enums import AccessRole
from karabo.common.packaging import utils
from karabo.native import AccessLevel

try:
    from karabogui import _version

    version = _version.version
    if hasattr(_version, 'full_version'):
        full_version = getattr(_version, 'full_version')
    else:
        full_version = version

    GUI_VERSION = utils.extract_base_version(version)
    GUI_VERSION_LONG = utils.extract_full_version(version)
except (ImportError, AttributeError):
    print('Version file not found! Please generate the _version.py file.')
    exit(1)

# The globally defined access level variable which is verified and set by the
# GUI server eventually

GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER

ACCESS_LEVEL_ROLES = {
    AccessRole.SCENE_EDIT: AccessLevel.EXPERT,
    AccessRole.MACRO_EDIT: AccessLevel.OPERATOR,
    AccessRole.PROJECT_EDIT: AccessLevel.OPERATOR,
    AccessRole.SERVICE_EDIT: AccessLevel.OPERATOR,
    AccessRole.CONSOLE_EDIT: AccessLevel.EXPERT}


def access_role_allowed(role):
    """Return on runtime if action in the GUI are allowed"""
    return GLOBAL_ACCESS_LEVEL >= ACCESS_LEVEL_ROLES[role]


# Hidden karabo folder which includes certain karabo related files
if platform.startswith('win'):
    HIDDEN_KARABO_FOLDER = path.join(environ['APPDATA'], 'karabo')
else:
    HIDDEN_KARABO_FOLDER = path.join(environ['HOME'], '.karabo')
# Project folder
KARABO_PROJECT_FOLDER = path.join(HIDDEN_KARABO_FOLDER, 'projects')
KARABO_CLIENT_ID = f"{socket.gethostname()}-{getpid()}"


MAX_UINT8 = (2 ** 8) - 1
MIN_UINT8 = (2 ** 8)
MAX_UINT16 = (2 ** 16) - 1
MIN_UINT16 = (2 ** 16)
MAX_UINT32 = (2 ** 32) - 1  # 0xffffffff
MIN_UINT32 = (2 ** 32)
MAX_UINT64 = (2 ** 64) - 1
MIN_UINT64 = (2 ** 64)
MAX_INT8 = (2 ** 7) - 1
MIN_INT8 = -(2 ** 7)
MAX_INT16 = (2 ** 15) - 1
MIN_INT16 = -(2 ** 15)
MAX_INT32 = (2 ** 31) - 1  # 0x7fffffff
MIN_INT32 = -(2 ** 31)  # -0x80000000
MAX_INT64 = (2 ** 63) - 1
MIN_INT64 = -(2 ** 63)
