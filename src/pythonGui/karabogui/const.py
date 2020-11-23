#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 7, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from platform import system
from xml.etree import ElementTree

from karabo.common.scenemodel.api import SCENE_DEFAULT_DPI, SCENE_MAC_DPI


ElementTree.register_namespace("xlink", "http://www.w3.org/1999/xlink")

# GUI PROPERTIES
PANEL_ICON_SIZE = 26

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
