#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on December 11, 2016
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from os import path

from .color_change_icon import get_color_change_icons

_icon_path = path.join(path.dirname(__file__),  "iconset")
ICONS = get_color_change_icons(_icon_path)
