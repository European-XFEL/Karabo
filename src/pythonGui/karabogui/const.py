#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 7, 2011
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
import socket
from os import environ, getpid, path
from platform import system

from karabo.common.packaging import utils
from karabo.common.scenemodel.api import SCENE_DEFAULT_DPI, SCENE_MAC_DPI

# APPLICATION
APPLICATION_MODE = False

# WIDGET PROPERTIES
WIDGET_MIN_WIDTH = 40
WIDGET_MIN_HEIGHT = 18

# SERVER COMMUNICATION PROPERTIES
REQUEST_REPLY_TIMEOUT = 5  # in seconds

# Presentation
GUI_DPI_FACTOR = (
    SCENE_DEFAULT_DPI / SCENE_MAC_DPI if system() == "Darwin" else 1)

# Operating System relevant
OS_SYSTEM = system()
IS_MAC_SYSTEM = OS_SYSTEM == "Darwin"
IS_WINDOWS_SYSTEM = OS_SYSTEM == "Windows"
IS_LINUX_SYSTEM = OS_SYSTEM == "Linux"

# Hidden karabo folder which includes certain karabo related files
if IS_WINDOWS_SYSTEM:
    HIDDEN_KARABO_FOLDER = path.join(environ['APPDATA'], 'karabo')
else:
    HIDDEN_KARABO_FOLDER = path.join(environ['HOME'], '.karabo')

# Project folder
KARABO_PROJECT_FOLDER = path.join(HIDDEN_KARABO_FOLDER, 'projects')
# Karabo GUI Client ID
KARABO_CLIENT_ID = f"{socket.gethostname()}-{getpid()}"
CLIENT_HOST = socket.gethostname()

# Tooltips
TOOLTIP_RUNTIME_CONFIG = (
    '<table width="300"><tr><td>'
    'A runtime configuration can be retrieved from the Datalogger System '
    'and may include read-only or dynamic properties. The lookup is '
    'based on a timestamp.</td></tr></table>'
)

TOOLTIP_INIT_CONFIG = (
    '<table width="300"><tr><td>'
    'An init configuration can be retrieved from the Configuration Manager '
    'and includes only writable properties set at instantiation that differ '
    'from the device\'s default settings. The lookup is performed '
    'using a name.</td></tr></table>'
)

try:
    from karabogui import _version

    version = _version.version
    if hasattr(_version, 'full_version'):
        full_version = getattr(_version, 'full_version')
    else:
        full_version = version

    GUI_VERSION = utils.extract_base_version(version)
    GUI_VERSION_LONG = utils.extract_full_version(version)
    GUI_VERSION_DETAILED = version
except (ImportError, AttributeError):
    print('Version file not found! Please generate the _version.py file.')
    exit(1)
