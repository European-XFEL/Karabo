# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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


class AccessRole(Enum):
    """The AccessRole Enum describes eventually validated action in the
    karabo GUI"""
    SCENE_EDIT = 'scene_edit'
    MACRO_EDIT = 'macro_edit'
    PROJECT_EDIT = 'project_edit'
    SERVICE_EDIT = 'service_edit'
    CONSOLE_EDIT = 'console_edit'


ACCESS_LEVEL_ROLES = {
    AccessRole.SCENE_EDIT: AccessLevel.EXPERT,
    AccessRole.MACRO_EDIT: AccessLevel.OPERATOR,
    AccessRole.PROJECT_EDIT: AccessLevel.OPERATOR,
    AccessRole.SERVICE_EDIT: AccessLevel.OPERATOR,
    AccessRole.CONSOLE_EDIT: AccessLevel.EXPERT}


def access_role_allowed(role):
    """Return on runtime if action in the GUI are allowed"""
    return GLOBAL_ACCESS_LEVEL >= ACCESS_LEVEL_ROLES[role]
