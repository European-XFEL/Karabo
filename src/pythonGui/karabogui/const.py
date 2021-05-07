#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 7, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import socket
from os import environ, getpid, path
from platform import system
from sys import platform
from xml.etree import ElementTree

from karabo.common.packaging import utils
from karabo.common.scenemodel.api import SCENE_DEFAULT_DPI, SCENE_MAC_DPI

ElementTree.register_namespace("xlink", "http://www.w3.org/1999/xlink")

# WIDGET PROPERTIES
WIDGET_MIN_WIDTH = 40
WIDGET_MIN_HEIGHT = 18

# PLOT PROPERTIES
MAXNUMPOINTS = 1000
MAX_NUMBER_LIMIT = 1e30

# COMMUNICATION
REQUEST_REPLY_TIMEOUT = 5  # in seconds

GUI_DPI_FACTOR = (
    SCENE_DEFAULT_DPI / SCENE_MAC_DPI if system() == "Darwin" else 1)

IS_MAC_SYSTEM = system() == "Darwin"

# Hidden karabo folder which includes certain karabo related files
if platform.startswith('win'):
    HIDDEN_KARABO_FOLDER = path.join(environ['APPDATA'], 'karabo')
else:
    HIDDEN_KARABO_FOLDER = path.join(environ['HOME'], '.karabo')

# Project folder
KARABO_PROJECT_FOLDER = path.join(HIDDEN_KARABO_FOLDER, 'projects')
# Karabo GUI Client ID
KARABO_CLIENT_ID = f"{socket.gethostname()}-{getpid()}"

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
