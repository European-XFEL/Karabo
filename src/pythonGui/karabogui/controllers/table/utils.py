#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 6, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from urllib.parse import parse_qs

from qtpy.QtGui import QBrush, QColor

from karabo.common.api import KARABO_SCHEMA_DISPLAY_TYPE_STATE
from karabo.native import AccessMode


def is_writable_binding(binding, binding_type=None):
    """Return if the `binding` is a writable string binding"""
    writable = binding.access_mode is AccessMode.RECONFIGURABLE
    if binding_type is None:
        return writable
    return isinstance(binding_type, binding_type) and writable


def is_state_display_type(binding):
    """Return if the display type belongs to a state element"""
    return binding.display_type == KARABO_SCHEMA_DISPLAY_TYPE_STATE


def string2list(value: str) -> list:
    """Convert a list of chars to a list of strings with delimiter ','.

    This function strips white spaces and returns an empty list if an
    empty string is passed.
    """
    return [v.strip() for v in value.split(",")] if value else []


def list2string(value: list) -> str:
    """Convert a list to a string representation with delimiter ','"""
    value = [] if value is None else value
    return ",".join(str(v) for v in value)


def create_brushes(display_string):
    """Parse a displayType string protocol for brushes

    e.g. a display string with text1=color1&text2=color2

    Returns a default brush and a dictionary of brushes
    """

    def create_brush(color):
        """Protective function to create a QBrush from a color"""
        try:
            return QBrush(QColor(color))
        except Exception:
            return None  # No brush!

    query_colors = display_string.split("|")
    if not len(query_colors) == 2:
        return None, {}

    parsed = parse_qs(query_colors[1], keep_blank_values=True)
    color_map = {name: color[0] for name, color in parsed.items()}
    default = color_map.pop("default", None)
    default_brush = create_brush(default) if default is not None else None

    brushes = {text: create_brush(color) for text, color
               in color_map.items()}

    return default_brush, brushes


def has_confirmation(display_string):
    attributes = get_button_attributes(display_string)
    return attributes.get("confirmation") == "1"


def get_button_attributes(display_string):
    """Retrieve the button attributes from a table display string"""
    splitted = display_string.split("|")
    if not len(splitted) == 2:
        return {}

    parsed = parse_qs(splitted[1], keep_blank_values=False)
    options = {name: opt[0] for name, opt in parsed.items()}

    return options


def parse_table_link(string):
    """Parse a table link for a table string button

    Valid schemes are for now `url` and `deviceScene`
    """
    splitted = string.split("|", 1)
    if not len(splitted) == 2:
        return None, {}

    action_type = splitted[0]
    if action_type == "deviceScene":
        parsed = parse_qs(splitted[1], keep_blank_values=False)
        options = {name: opt[0] for name, opt in parsed.items()}
    elif action_type == "url":
        options = splitted[1]
    else:
        options = {}
    return action_type, options
