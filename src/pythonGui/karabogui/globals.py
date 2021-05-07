#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 23, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import socket
from os import environ, getpid, path
from sys import platform

from karabo.common.packaging import utils

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

# Hidden karabo folder which includes certain karabo related files
if platform.startswith('win'):
    HIDDEN_KARABO_FOLDER = path.join(environ['APPDATA'], 'karabo')
else:
    HIDDEN_KARABO_FOLDER = path.join(environ['HOME'], '.karabo')

# Project folder
KARABO_PROJECT_FOLDER = path.join(HIDDEN_KARABO_FOLDER, 'projects')
# Karabo GUI Client ID
KARABO_CLIENT_ID = f"{socket.gethostname()}-{getpid()}"
