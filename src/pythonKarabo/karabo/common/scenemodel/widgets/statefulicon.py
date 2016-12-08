import os

from xml.etree.ElementTree import SubElement

from traits.api import Enum

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_SVG
from karabo.common.scenemodel.io_utils import (
    read_empty_display_editable_widget, write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)
from karabo.common.scenemodel.color_change_icon import get_color_change_icons


STATEFUL_ICON_WIDGETS = []
stateful_icon_path = os.path.join(os.path.dirname(__file__), "..", "iconset")
stateful_icons = get_color_change_icons(stateful_icon_path)

for key, icon in stateful_icons.items():
    STATEFUL_ICON_WIDGETS.append(key)

class StatefulIconWidgetModel(BaseWidgetObjectData):
    """ A model for StatefulIconWidgetModel objects"""
    klass = Enum(*STATEFUL_ICON_WIDGETS)


@register_scene_writer(StatefulIconWidgetModel)
def _vacuum_widget_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'g')
    write_base_widget_data(model, element, model.klass)
    return element


def _build_stateful_icon_widget_readers():
    """ Build readers for all the possible stateful icon widgets.
    """
    def _reader(read_func, element):
        traits = read_empty_display_editable_widget(element)
        return StatefulIconWidgetModel(**traits)

    for widget in STATEFUL_ICON_WIDGETS:
        register_scene_reader(widget, version=1)(_reader)

# Call the builder to register all the stateful icon widget readers and writers
_build_stateful_icon_widget_readers()
