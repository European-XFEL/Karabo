#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on December 8, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from xml.etree.ElementTree import SubElement

from traits.api import String

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_KARABO, NS_SVG
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


class StatefulIconWidgetModel(BaseWidgetObjectData):
    """ A model for StatefulIconWidgetModel objects"""
    icon_name = String()


@register_scene_writer(StatefulIconWidgetModel)
def _statefulwidget_widget_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    write_base_widget_data(model, element, 'StatefulIconWidget')
    element.set(NS_KARABO + 'icon_name', model.icon_name)
    return element


@register_scene_reader('StatefulIconWidget', version=2)
def _statefulwidget_widget_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['icon_name'] = element.get(NS_KARABO + 'icon_name', '')
    return StatefulIconWidgetModel(**traits)
