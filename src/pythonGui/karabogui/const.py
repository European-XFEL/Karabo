#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 7, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from xml.etree import ElementTree


ElementTree.register_namespace("xlink", "http://www.w3.org/1999/xlink")

# WIDGET PROPERTIES
WIDGET_MIN_WIDTH = 40
WIDGET_MIN_HEIGHT = 18

# PLOT PROPERTIES
MAXNUMPOINTS = 1000
MAX_NUMBER_LIMIT = 1e30

# COMMUNICATION
REQUEST_REPLY_TIMEOUT = 5  # in seconds
