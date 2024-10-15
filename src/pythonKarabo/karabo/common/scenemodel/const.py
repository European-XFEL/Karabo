# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import enum
from xml.etree.ElementTree import register_namespace

# The current version of the scene file format.
# New files will always be written with the current version.
SCENE_FILE_VERSION = 2

# Scene default dimensions
SCENE_MIN_WIDTH = 1024
SCENE_MIN_HEIGHT = 768

# Scene display constants
SCENE_DEFAULT_DPI = 96
SCENE_MAC_DPI = 72

# Scene default font formatting
SCENE_FONT_FAMILY = "Source Sans Pro"
SCENE_FONT_SIZE = 10
SCENE_FONT_WEIGHT = "normal"

SCENE_FONT_SIZES = [
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    14,
    16,
    18,
    21,
    24,
    30,
    36,
    48,
    60,
    72,
]  # noqa
SCENE_FONT_WEIGHTS = ["normal", "bold"]

DISPLAY_COMPONENT = "DisplayComponent"

EDITABLE_COMPONENT = "EditableApplyLaterComponent"


@enum.unique
class SceneTargetWindow(enum.Enum):
    MainWindow = "mainwin"  # The default; a tab in the main window
    Dialog = "dialog"  # An undocked window

    def __repr__(self):
        return self.__str__()


# Define some XML namespaces that we might encounter
NS_INKSCAPE = "{http://www.inkscape.org/namespaces/inkscape}"
NS_KARABO = "{http://karabo.eu/scene}"
NS_SVG = "{http://www.w3.org/2000/svg}"
NS_XLINK = "{http://www.w3.org/1999/xlink}"

# Images
IMAGE_ELEMENT_KEY = f"{NS_XLINK}href"

# For convenience (and initialization of ElementTree), put them into a dict too
XML_NAMESPACES = {
    "inkscape": NS_INKSCAPE,
    "krb": NS_KARABO,
    "svg": NS_SVG,
    "xlink": NS_XLINK,
}

for prefix, ns in XML_NAMESPACES.items():
    register_namespace(prefix, ns.strip("{}"))

# Every widget has the same tag and is differentiated with attributes
WIDGET_ELEMENT_TAG = NS_SVG + "rect"
IMAGE_ELEMENT_TAG = NS_SVG + "image"
UNKNOWN_WIDGET_CLASS = "UnknownWidget"
SVG_GROUP_TAG = NS_SVG + "g"
# Minimum width or height of the arrow.
ARROW_MIN_SIZE = 10

# Float formatting constants
DEFAULT_FORMAT = "g"
DEFAULT_DECIMALS = "8"
