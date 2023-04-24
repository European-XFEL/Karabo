# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from xml.etree.ElementTree import SubElement

from traits.api import Bool, Int, String

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_KARABO, WIDGET_ELEMENT_TAG
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


class SparklineModel(BaseWidgetObjectData):
    """A model for a Sparkline"""

    # Time span displayed by the sparkline
    time_base = Int(600)
    # If True, show the current value next to the line
    show_value = Bool(False)
    # If True, show the current alarms if present
    alarm_range = Bool(True)
    # String format for the value when it is shown
    show_format = String("0.2f")


@register_scene_reader("DisplaySparkline", version=2)
def _display_sparkline_reader(element):
    traits = read_base_widget_data(element)
    traits["time_base"] = int(element.get(NS_KARABO + "time_base", 600))
    traits["show_format"] = element.get(NS_KARABO + "show_format", "0.2f")
    show_value = element.get(NS_KARABO + "show_value", "false")
    traits["show_value"] = show_value.lower() == "true"
    alarm_range = element.get(NS_KARABO + "alarm_range", "true")
    traits["alarm_range"] = alarm_range.lower() == "true"
    return SparklineModel(**traits)


@register_scene_writer(SparklineModel)
def _display_sparkline_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "DisplaySparkline")
    element.set(NS_KARABO + "time_base", str(model.time_base))
    element.set(NS_KARABO + "show_value", str(model.show_value).lower())
    element.set(NS_KARABO + "alarm_range", str(model.alarm_range).lower())
    element.set(NS_KARABO + "show_format", str(model.show_format))
    return element
