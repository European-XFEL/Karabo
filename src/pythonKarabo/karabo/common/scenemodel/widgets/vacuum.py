from xml.etree.ElementTree import SubElement

from traits.api import Enum

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import WIDGET_ELEMENT_TAG
from karabo.common.scenemodel.io_utils import (
    read_empty_display_editable_widget, write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# !! NOTE: Vacuum widgets are deprecated! This code remains in place to enable
# !! reading and writing of existing scene data. The GUI code no longer gives
# !! the option to create vacuum widgets in scenes.
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

VACUUM_WIDGETS = (
    'AgilentIonWidget', 'CryoCoolerWidget', 'HydraulicValveWidget',
    'LampWidget', 'MaxiGaugeWidget', 'MembranePumpWidget',
    'MotorWidget', 'PressureGaugeWidget', 'PressureSwitchWidget',
    'RightAngleValveWidget', 'ShutOffValveWidget', 'TemperatureProbeWidget',
    'TurboPumpWidget', 'ValveWidget'
)


class VacuumWidgetModel(BaseWidgetObjectData):
    """A model for VacuumWidget objects."""
    # The actual type of the widget
    klass = Enum(*VACUUM_WIDGETS)


@register_scene_writer(VacuumWidgetModel)
def _vacuum_widget_writer(write_func, model, parent):
    element = SubElement(parent, WIDGET_ELEMENT_TAG)
    write_base_widget_data(model, element, model.klass)
    return element


def _build_vacuum_widget_readers():
    """ Build readers for all the possible vacuum widgets.
    """
    def _reader(read_func, element):
        traits = read_empty_display_editable_widget(element)
        return VacuumWidgetModel(**traits)

    for widget in VACUUM_WIDGETS:
        register_scene_reader(widget, version=1)(_reader)


# Call the builder to register all the vacuum widget readers and writers
_build_vacuum_widget_readers()
