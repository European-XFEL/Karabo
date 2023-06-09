#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on December 8, 2016
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
