from traits.api import Enum
from xml.etree.ElementTree import SubElement

from karabo.common.icons.color_change_icon import icons
from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_KARABO
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


STATEFUL_ICON_WIDGETS = []

for key, icon in icons.items():
    STATEFUL_ICON_WIDGETS.append(key)


class StatefulIconWidgetModel(BaseWidgetObjectData):
    """ A model for StatefulIconWidgetModel objects"""
    icon_name = Enum(*STATEFUL_ICON_WIDGETS)
    klass = 'StatefulIconWidget'


@register_scene_writer(StatefulIconWidgetModel)
def _statefulwidget_widget_writer(write_func, model, parent):
    element = SubElement(parent, NS_KARABO + 'statefulicon')
    element.set(NS_KARABO + 'icon_name', model.icon_name)
    write_base_widget_data(model, element, model.klass)
    return element


@register_scene_reader('StatefulIconWidget', version=2)
def _statefulwidget_widget_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['icon_name'] = element.get(NS_KARABO + 'icon_name', '')
    return StatefulIconWidgetModel(**traits)

register_scene_reader('statefulicon', version=2)(_statefulwidget_widget_reader)
