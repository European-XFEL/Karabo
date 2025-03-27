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
from collections import OrderedDict
from enum import Enum

from karabo.native import AccessLevel

ACCESS_LEVELS = OrderedDict()
ACCESS_LEVELS['Admin'] = AccessLevel.ADMIN
ACCESS_LEVELS['Expert'] = AccessLevel.EXPERT
ACCESS_LEVELS['Operator'] = AccessLevel.OPERATOR
ACCESS_LEVELS['User'] = AccessLevel.USER
ACCESS_LEVELS['Observer'] = AccessLevel.OBSERVER

USERNAMES = [level.lower() for level in ACCESS_LEVELS.keys()]

# The globally defined access level variable which is verified and set by the
# GUI server eventually
GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER
HIGHEST_ACCESS_LEVEL = AccessLevel.OBSERVER
ONE_TIME_TOKEN = None

AUTHENTICATION_SERVER = None

TEMPORARY_SESSION_USER = None
TEMPORARY_SESSION_WARNING = False

ACCESS_LEVEL_MAP = {
    "observer": 0,
    "user": 1,
    "operator": 2,
    "expert": 3,
    "admin": 4,
    "god": 5}


class AccessRole(Enum):
    """The AccessRole Enum describes eventually validated action in the
    karabo GUI"""
    SCENE_EDIT = 'scene_edit'
    MACRO_EDIT = 'macro_edit'
    PROJECT_EDIT = 'project_edit'
    SERVICE_EDIT = 'service_edit'
    CONSOLE_EDIT = 'console_edit'
    INSTANCE_CONTROL = 'instance_control'
    APPLY_SCENE_EDIT = 'apply_edit'
    CONFIGURATION_DELETE = 'delete_configuration'


ACCESS_LEVEL_ROLES = {
    AccessRole.SCENE_EDIT: AccessLevel.EXPERT,
    AccessRole.MACRO_EDIT: AccessLevel.OPERATOR,
    AccessRole.PROJECT_EDIT: AccessLevel.OPERATOR,
    AccessRole.SERVICE_EDIT: AccessLevel.OPERATOR,
    AccessRole.CONSOLE_EDIT: AccessLevel.EXPERT,
    AccessRole.INSTANCE_CONTROL: AccessLevel.OPERATOR,
    AccessRole.CONFIGURATION_DELETE: AccessLevel.EXPERT,
    AccessRole.APPLY_SCENE_EDIT: AccessLevel.OPERATOR}


def access_role_allowed(role):
    """Return on runtime if action in the GUI are allowed"""
    return GLOBAL_ACCESS_LEVEL >= ACCESS_LEVEL_ROLES[role]


def is_authenticated():
    """one time token is set for only User-Authenticated login"""
    return ONE_TIME_TOKEN is not None


def is_temporary_session() -> bool:
    """Check if session is temporary"""
    return TEMPORARY_SESSION_USER is not None


def reset_login():
    """Reset the global login information"""
    global ONE_TIME_TOKEN, TEMPORARY_SESSION_WARNING, TEMPORARY_SESSION_USER
    ONE_TIME_TOKEN = None
    TEMPORARY_SESSION_USER = None
    TEMPORARY_SESSION_WARNING = False


def get_access_level_for_role(role: AccessRole) -> str:
    """Get the access level needed for the AccessRole. """
    return ACCESS_LEVEL_ROLES[role].name
