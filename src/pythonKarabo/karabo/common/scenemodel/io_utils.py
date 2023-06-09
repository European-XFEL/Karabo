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
import re
from collections.abc import Sequence

from traits.api import Undefined

from .const import (
    DEFAULT_DECIMALS, DEFAULT_FORMAT, NS_KARABO, SCENE_FONT_SIZE,
    SCENE_FONT_WEIGHT)
from .exceptions import SceneWriterException

SVG_DEF_REGEX = re.compile(r"url\(\#(.*?)\)")


def get_numbers(names, element):
    """Read a list of float values from an `Element` instance."""
    return {name: float(element.get(name)) for name in names}


def set_numbers(names, model, element, xmlnames=None):
    """Copy a list of integer attribute values to an `Element` instance.

    This is the inverse of `get_numbers`.
    """
    EPSILON = 1e-2
    if xmlnames is None:
        xmlnames = names
    elif not isinstance(xmlnames, Sequence) or len(names) != len(xmlnames):
        msg = "XML names must match up with object names"
        raise SceneWriterException(msg)

    def _convert(num):
        if abs(num - int(num)) > EPSILON:
            return str(num)
        return str(int(num))

    for modelname, xmlname in zip(names, xmlnames):
        element.set(xmlname, _convert(getattr(model, modelname)))


def read_base_widget_data(element):
    """Read the attributes common to all "widget" elements"""
    traits = get_numbers(("x", "y", "width", "height"), element)
    traits["parent_component"] = element.get(NS_KARABO + "class")
    keys = element.get(NS_KARABO + "keys", "")
    if len(keys) > 0:
        traits["keys"] = keys.split(",")
    return traits


def read_empty_display_editable_widget(element):
    """Read the attributes common to all widgets with Display/Editable forms.
    """
    traits = read_base_widget_data(element)
    traits["klass"] = element.get(NS_KARABO + "widget")
    return traits


def read_unknown_display_editable_widget(element):
    """Read the attributes from an unknown data model

    This can be either a controller ``widget`` or a scene ``class`` widget
    """
    traits = read_base_widget_data(element)
    klass = element.get(NS_KARABO + "widget")
    if klass is None:
        klass = element.get(NS_KARABO + "class")
    traits["klass"] = klass
    return traits


def read_font_format_data(element):
    """Read the font format attributes (font_size and font_weight"""
    traits = {}
    font_size = int(element.get(NS_KARABO + "font_size", SCENE_FONT_SIZE))
    traits["font_size"] = font_size
    font_weight = element.get(NS_KARABO + "font_weight", SCENE_FONT_WEIGHT)
    traits["font_weight"] = font_weight
    return traits


def write_base_widget_data(model, element, widget_class_name):
    """Write out the attributes common to all "widget" elements"""
    if len(model.parent_component) == 0:
        msg = f"Widget {widget_class_name} has no parent component!"
        raise SceneWriterException(msg)
    element.set(NS_KARABO + "class", model.parent_component)
    element.set(NS_KARABO + "widget", widget_class_name)
    element.set(NS_KARABO + "keys", ",".join(model.keys))
    set_numbers(("x", "y", "width", "height"), model, element)


def write_font_format_data(model, element):
    # Write only modified display label formatting
    if model.font_size != SCENE_FONT_SIZE:
        element.set(NS_KARABO + "font_size", str(model.font_size))
    if model.font_weight != SCENE_FONT_WEIGHT:
        element.set(NS_KARABO + "font_weight", model.font_weight)


def read_alarm_data(element):
    """Read the alarm attributes"""
    traits = {}
    for key in ("warnLow", "warnHigh", "alarmLow", "alarmHigh"):
        value = element.get(NS_KARABO + key, "")
        traits[key] = float(value) if value else Undefined
    return traits


def write_alarm_data(model, element):
    # Write only existing alarm information
    for key in ("warnLow", "warnHigh", "alarmLow", "alarmHigh"):
        value = getattr(model, key)
        if value is Undefined:
            continue
        element.set(NS_KARABO + key, str(value))


def read_value_format_data(element):
    """Read the formatting data of the models"""
    traits = {}
    traits["fmt"] = element.get(NS_KARABO + "fmt", DEFAULT_FORMAT)
    traits["decimals"] = element.get(NS_KARABO + "decimals", DEFAULT_DECIMALS)
    return traits


def write_value_format_data(model, element):
    """Write the value formatting to the element"""
    element.set(NS_KARABO + "fmt", model.fmt)
    element.set(NS_KARABO + "decimals", model.decimals)


def get_defs_id(value):
    match = SVG_DEF_REGEX.match(value)
    return match.group(1) if match else None


def is_empty(value):
    return value is None or value == ""


def convert_number_or_string(value):
    try:
        return float(value)
    except ValueError:
        # Value is not a number, we consider the string as-is instead.
        return value
