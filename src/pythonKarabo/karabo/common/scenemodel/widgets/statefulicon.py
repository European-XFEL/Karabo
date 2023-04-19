#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on December 8, 2016
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################

from xml.etree.ElementTree import SubElement

from traits.api import String

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_KARABO, WIDGET_ELEMENT_TAG
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


class StatefulIconWidgetModel(BaseWidgetObjectData):
    """A model for StatefulIconWidgetModel objects"""

    icon_name = String()


@register_scene_writer(StatefulIconWidgetModel)
def _statefulwidget_widget_writer(model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, "StatefulIconWidget")
    element.set(NS_KARABO + "icon_name", model.icon_name)
    return element


@register_scene_reader("StatefulIconWidget", version=2)
def _statefulwidget_widget_reader(element):
    traits = read_base_widget_data(element)
    traits["icon_name"] = element.get(NS_KARABO + "icon_name", "")
    return StatefulIconWidgetModel(**traits)
